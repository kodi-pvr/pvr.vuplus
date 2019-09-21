/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Recordings.h"

#include "../Enigma2.h"
#include "../client.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>

#include <kodi/util/XMLUtils.h>
#include <nlohmann/json.hpp>
#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using json = nlohmann::json;

const std::string Recordings::FILE_NOT_FOUND_RESPONSE_SUFFIX = "not found";

Recordings::Recordings(Channels& channels, enigma2::extract::EpgEntryExtractor& entryExtractor)
      : m_channels(channels), m_entryExtractor(entryExtractor)
{
  std::random_device randomDevice; //Will be used to obtain a seed for the random number engine
  m_randomGenerator = std::mt19937(randomDevice()); //Standard mersenne_twister_engine seeded with randomDevice()
  m_randomDistribution = std::uniform_int_distribution<>(E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MIN, E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MAX);
}

void Recordings::GetRecordings(std::vector<PVR_RECORDING>& kodiRecordings, bool deleted)
{
  auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  for (auto& recording : recordings)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer recording '%s', Recording Id '%s'", __FUNCTION__, recording.GetTitle().c_str(), recording.GetRecordingId().c_str());
    PVR_RECORDING kodiRecording = {0};

    recording.UpdateTo(kodiRecording, m_channels, IsInRecordingFolder(recording.GetTitle(), deleted));

    kodiRecordings.emplace_back(kodiRecording);
  }
}

int Recordings::GetNumRecordings(bool deleted) const
{
  const auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  return recordings.size();
}

void Recordings::ClearRecordings(bool deleted)
{
  auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  recordings.clear();

  for (auto it = m_recordingsIdMap.begin(); it != m_recordingsIdMap.end();)
  {
    if (it->second.IsDeleted() == deleted)
      it = m_recordingsIdMap.erase(it);
    else
      ++it;
  }
}

void Recordings::GetRecordingEdl(const std::string& recordingId, std::vector<PVR_EDL_ENTRY>& edlEntries) const
{
  const RecordingEntry recordingEntry = GetRecording(recordingId);

  if (!recordingEntry.GetEdlURL().empty())
  {
    const std::string edlFile = WebUtils::GetHttp(recordingEntry.GetEdlURL());

    if (!StringUtils::EndsWith(edlFile, FILE_NOT_FOUND_RESPONSE_SUFFIX))
    {
      std::istringstream stream(edlFile);
      std::string line;
      int lineNumber = 0;
      while (std::getline(stream, line))
      {
        float start = 0.0f, stop = 0.0f;
        unsigned int type = PVR_EDL_TYPE_CUT;
        lineNumber++;
        if (std::sscanf(line.c_str(), "%f %f %u", &start, &stop, &type) < 2 || type > PVR_EDL_TYPE_COMBREAK)
        {
          Logger::Log(LEVEL_NOTICE, "%s Unable to parse EDL entry for recording '%s' at line %d. Skipping.", __FUNCTION__,
                      recordingEntry.GetTitle().c_str(), lineNumber);
          continue;
        }

        start += static_cast<float>(Settings::GetInstance().GetEDLStartTimePadding()) / 1000.0f;
        stop += static_cast<float>(Settings::GetInstance().GetEDLStopTimePadding()) / 1000.0f;

        start = std::max(start, 0.0f);
        stop = std::max(stop, 0.0f);
        start = std::min(start, stop);
        stop = std::max(start, stop);

        Logger::Log(LEVEL_NOTICE, "%s EDL for '%s', line %d -  start: %f stop: %f type: %d", __FUNCTION__, recordingEntry.GetTitle().c_str(), lineNumber, start, stop, type);

        PVR_EDL_ENTRY edlEntry;
        edlEntry.start = static_cast<int64_t>(start * 1000.0f);
        edlEntry.end = static_cast<int64_t>(stop * 1000.0f);
        edlEntry.type = static_cast<PVR_EDL_TYPE>(type);

        edlEntries.emplace_back(edlEntry);
      }
    }
  }
}

RecordingEntry Recordings::GetRecording(const std::string& recordingId) const
{
  RecordingEntry entry;

  auto recordingPair = m_recordingsIdMap.find(recordingId);
  if (recordingPair != m_recordingsIdMap.end())
  {
    entry = recordingPair->second;
  }

  return entry;
}

bool Recordings::IsInRecordingFolder(const std::string& recordingFolder, bool deleted) const
{
  const auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  int iMatches = 0;
  for (const auto& recording : recordings)
  {
    if (recordingFolder == recording.GetTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found Recording title '%s' in recordings vector!", __FUNCTION__, recordingFolder.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __FUNCTION__, recordingFolder.c_str());
        return true;
      }
    }
  }

  return false;
}

PVR_ERROR Recordings::RenameRecording(const PVR_RECORDING& recording)
{
  auto recordingEntry = GetRecording(recording.strRecordingId);

  if (!recordingEntry.GetRecordingId().empty())
  {
    Logger::Log(LEVEL_DEBUG, "%s Sending rename command for recording '%s' to '%s'", __FUNCTION__, recordingEntry.GetTitle().c_str(), recording.strTitle);
    const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&title=%s", Settings::GetInstance().GetConnectionURL().c_str(),
                                                    WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                                                    WebUtils::URLEncodeInline(recording.strTitle).c_str());
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, strResult))
    {
      PVR->TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Recordings::SetRecordingPlayCount(const PVR_RECORDING& recording, int count)
{
  auto recordingEntry = GetRecording(recording.strRecordingId);

  if (!recordingEntry.GetRecordingId().empty())
  {
    if (recording.iPlayCount == count)
      return PVR_ERROR_NO_ERROR;

    std::vector<std::string> oldTags;
    ReadExtraRecordingPlayCountInfo(recordingEntry, oldTags);

    std::string addTagsArg = TAG_FOR_PLAY_COUNT + "=" + std::to_string(count);

    std::string deleteTagsArg;
    for (std::string& oldTag : oldTags)
    {
      if (oldTag != addTagsArg)
      {
        if (!deleteTagsArg.empty())
          deleteTagsArg += ",";

        deleteTagsArg += oldTag;
      }
    }

    Logger::Log(LEVEL_DEBUG, "%s Setting playcount for recording '%s' to '%d'", __FUNCTION__, recordingEntry.GetTitle().c_str(), count);
    const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                                Settings::GetInstance().GetConnectionURL().c_str(),
                                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                                WebUtils::URLEncodeInline(addTagsArg).c_str());
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, strResult))
    {
      PVR->TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Recordings::SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastPlayedPosition)
{
  auto recordingEntry = GetRecording(recording.strRecordingId);

  if (!recordingEntry.GetRecordingId().empty())
  {
    if (recording.iLastPlayedPosition == lastPlayedPosition)
      return PVR_ERROR_NO_ERROR;

    std::vector<std::pair<int, int64_t>> cuts;
    std::vector<std::string> oldTags;

    bool readExtraCutsInfo = ReadExtaRecordingCutsInfo(recordingEntry, cuts, oldTags);
    std::string cutsArg;
    bool cutsLastPlayedSet = false;
    if (readExtraCutsInfo && Settings::GetInstance().GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_AND_E2_INSTANCES)
    {
      for (auto cut : cuts)
      {
        if (!cutsArg.empty())
          cutsArg += ",";

        if (cut.first == CUTS_LAST_PLAYED_TYPE)
        {
          if (!cutsLastPlayedSet)
          {
            cutsArg += std::to_string(CUTS_LAST_PLAYED_TYPE) + ":" + std::to_string(PTS_PER_SECOND * lastPlayedPosition);
            cutsLastPlayedSet = true;
          }
        }
        else
        {
          cutsArg += std::to_string(cut.first) + ":" + std::to_string(cut.second);
        }
      }

      if (!cutsLastPlayedSet)
      {
        if (!cutsArg.empty())
          cutsArg += ",";

        cutsArg += std::to_string(CUTS_LAST_PLAYED_TYPE) + ":" + std::to_string(PTS_PER_SECOND * lastPlayedPosition);
        cutsLastPlayedSet = true;
      }
    }

    std::string addTagsArg = TAG_FOR_LAST_PLAYED + "=" + std::to_string(lastPlayedPosition);

    std::string deleteTagsArg;
    for (std::string& oldTag : oldTags)
    {
      if (oldTag != addTagsArg)
      {
        if (!deleteTagsArg.empty())
          deleteTagsArg += ",";

        deleteTagsArg += oldTag;
      }
    }

    addTagsArg += "," + TAG_FOR_NEXT_SYNC_TIME + "=" + std::to_string(std::time(nullptr) + m_randomDistribution(m_randomGenerator));

    Logger::Log(LEVEL_DEBUG, "%s Setting last played position for recording '%s' to '%d'", __FUNCTION__, recordingEntry.GetTitle().c_str(), lastPlayedPosition);

    std::string jsonUrl;
    if (Settings::GetInstance().GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_INSTANCES || !cutsLastPlayedSet)
    {
      jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                Settings::GetInstance().GetConnectionURL().c_str(),
                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                WebUtils::URLEncodeInline(addTagsArg).c_str());
    }
    else
    {
      jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s&cuts=%s",
                Settings::GetInstance().GetConnectionURL().c_str(),
                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                WebUtils::URLEncodeInline(addTagsArg).c_str(),
                WebUtils::URLEncodeInline(cutsArg).c_str());
    }
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, strResult))
    {
      PVR->TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

int Recordings::GetRecordingLastPlayedPosition(const PVR_RECORDING& recording)
{
  auto recordingEntry = GetRecording(recording.strRecordingId);

  time_t now = std::time(nullptr);
  time_t newNextSyncTime = now + m_randomDistribution(m_randomGenerator);

  Logger::Log(LEVEL_DEBUG, "%s Recording: %s - Checking if Next Sync Time: %ld < Now: %ld ", __FUNCTION__, recordingEntry.GetTitle().c_str(), recordingEntry.GetNextSyncTime(), now);

  if (Settings::GetInstance().GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_AND_E2_INSTANCES &&
      recordingEntry.GetNextSyncTime() < now)
  {
    //We need to get this value separately as it's not returned by the movielist api
    //We don't want to call for it everytime as large movie directories would make a lot of calls.
    //Instead we'll only call out every five to ten mins and store the value so it can be returned by the movielist api.

    std::vector<std::pair<int, int64_t>> cuts;
    std::vector<std::string> oldTags;
    bool readExtraCutsInfo = ReadExtaRecordingCutsInfo(recordingEntry, cuts, oldTags);
    int lastPlayedPosition = -1;
    if (readExtraCutsInfo)
    {
      for (auto cut : cuts)
      {
        if (cut.first == CUTS_LAST_PLAYED_TYPE)
        {
          lastPlayedPosition = cut.second / PTS_PER_SECOND;
          break;
        }
      }
    }

    if (readExtraCutsInfo && lastPlayedPosition >= 0 && lastPlayedPosition != recordingEntry.GetLastPlayedPosition())
    {
      std::string addTagsArg = TAG_FOR_LAST_PLAYED + "=" + std::to_string(lastPlayedPosition);

      //then we update it in the tags using movieinfo
      std::string deleteTagsArg;
      for (std::string& oldTag : oldTags)
      {
        if (oldTag != addTagsArg)
        {
          if (!deleteTagsArg.empty())
            deleteTagsArg += ",";

          deleteTagsArg += oldTag;
        }
      }

      addTagsArg += "," + TAG_FOR_NEXT_SYNC_TIME + "=" + std::to_string(newNextSyncTime);

      Logger::Log(LEVEL_DEBUG, "%s Setting last played position from E2 cuts file to tags for recording '%s' to '%d'", __FUNCTION__, recordingEntry.GetTitle().c_str(), lastPlayedPosition);

      std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                            Settings::GetInstance().GetConnectionURL().c_str(),
                            WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                            WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                            WebUtils::URLEncodeInline(addTagsArg).c_str());
      std::string strResult;

      if (WebUtils::SendSimpleJsonCommand(jsonUrl, strResult))
      {
        recordingEntry.SetLastPlayedPosition(lastPlayedPosition);
        recordingEntry.SetNextSyncTime(newNextSyncTime);
      }
    }
    else
    {
      //just update the tag for next sync.
      SetRecordingNextSyncTime(recordingEntry, newNextSyncTime, oldTags);

      lastPlayedPosition = recordingEntry.GetLastPlayedPosition();
    }

    return lastPlayedPosition;
  }
  else
  {
    return recordingEntry.GetLastPlayedPosition();
  }
}

void Recordings::SetRecordingNextSyncTime(RecordingEntry& recordingEntry, time_t nextSyncTime, std::vector<std::string>& oldTags)
{
  Logger::Log(LEVEL_DEBUG, "%s Setting next sync time in tags for recording '%s' to '%ld'", __FUNCTION__, recordingEntry.GetTitle().c_str(), nextSyncTime);

  std::string addTagsArg = TAG_FOR_NEXT_SYNC_TIME + "=" + std::to_string(nextSyncTime);

  //then we update it in the tags using movieinfo api
  std::string deleteTagsArg;
  for (std::string& oldTag : oldTags)
  {
    if (oldTag != addTagsArg && StringUtils::StartsWith(oldTag, TAG_FOR_NEXT_SYNC_TIME + "="))
    {
      if (!deleteTagsArg.empty())
        deleteTagsArg += ",";

      deleteTagsArg += oldTag;
    }
  }

  const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                              Settings::GetInstance().GetConnectionURL().c_str(),
                              WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                              WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                              WebUtils::URLEncodeInline(addTagsArg).c_str());
  std::string strResult;

  if (!WebUtils::SendSimpleJsonCommand(jsonUrl, strResult))
  {
    recordingEntry.SetNextSyncTime(nextSyncTime);
    Logger::Log(LEVEL_ERROR, "%s Error setting next sync time for recording '%s' to '%ld'", __FUNCTION__, recordingEntry.GetTitle().c_str(), nextSyncTime);
  }
}

PVR_ERROR Recordings::DeleteRecording(const PVR_RECORDING& recinfo)
{
  const std::string strTmp = StringUtils::Format("web/moviedelete?sRef=%s", WebUtils::URLEncodeInline(recinfo.strRecordingId).c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_FAILED;

  // No need to call PVR->TriggerRecordingUpdate() as it is handled by kodi PVR.
  // In fact when multiple recordings are removed at once, calling it here can cause hanging issues

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Recordings::UndeleteRecording(const PVR_RECORDING& recording)
{
  auto recordingEntry = GetRecording(recording.strRecordingId);

  std::regex regex(TRASH_FOLDER);

  const std::string newRecordingDirectory = std::regex_replace(recordingEntry.GetDirectory(), regex, "");

  const std::string strTmp = StringUtils::Format("web/moviemove?sRef=%s&dirname=%s", WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(), WebUtils::URLEncodeInline(newRecordingDirectory).c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_FAILED;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Recordings::DeleteAllRecordingsFromTrash()
{
  for (const auto& deletedRecording : m_deletedRecordings)
  {
    const std::string strTmp =
        StringUtils::Format("web/moviedelete?sRef=%s", WebUtils::URLEncodeInline(deletedRecording.GetRecordingId()).c_str());

    std::string strResult;
    WebUtils::SendSimpleCommand(strTmp, strResult, true);
  }

  return PVR_ERROR_NO_ERROR;
}

bool Recordings::HasRecordingStreamProgramNumber(const PVR_RECORDING& recording)
{
  return GetRecording(recording.strRecordingId).HasStreamProgramNumber();
}

int Recordings::GetRecordingStreamProgramNumber(const PVR_RECORDING& recording)
{
  return GetRecording(recording.strRecordingId).GetStreamProgramNumber();
}

const std::string Recordings::GetRecordingURL(const PVR_RECORDING& recinfo)
{
  for (const auto& recording : m_recordings)
  {
    if (recinfo.strRecordingId == recording.GetRecordingId())
      return recording.GetStreamURL();
  }
  return "";
}

bool Recordings::ReadExtaRecordingCutsInfo(const data::RecordingEntry& recordingEntry, std::vector<std::pair<int, int64_t>>& cuts, std::vector<std::string>& tags)
{
  const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s", Settings::GetInstance().GetConnectionURL().c_str(),
                                                  WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str());

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (jsonDoc["result"].empty() || !jsonDoc["result"].get<bool>())
      return false;

    if (!jsonDoc["cuts"].empty())
    {
      int type;
      uint64_t position;

      for (const auto& cut : jsonDoc["cuts"].items())
      {
        for (const auto& element : cut.value().items())
        {
          if (element.key() == "type")
            type = element.value().get<int>();
          if (element.key() == "pos")
            position = element.value().get<int64_t>();
        }

        cuts.emplace_back(std::make_pair(type, position));
      }
    }

    if (!jsonDoc["tags"].empty())
    {
      for (const auto& tag : jsonDoc["tags"].items())
      {
        std::string tempTag = tag.value().get<std::string>();

        if (StringUtils::StartsWith(tempTag, TAG_FOR_LAST_PLAYED) || StringUtils::StartsWith(tempTag, TAG_FOR_NEXT_SYNC_TIME))
          tags.emplace_back(tempTag);
      }
    }

    return true;
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot find extra recording cuts info from OpenWebIf for recording: %s, ID: %s - JSON parse error - message: %s, exception id: %d", __FUNCTION__, recordingEntry.GetTitle().c_str(), recordingEntry.GetRecordingId().c_str(), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return false;
}

bool Recordings::ReadExtraRecordingPlayCountInfo(const data::RecordingEntry& recordingEntry, std::vector<std::string>& tags)
{
  const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s", Settings::GetInstance().GetConnectionURL().c_str(),
                                                  WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str());

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (jsonDoc["result"].empty() || !jsonDoc["result"].get<bool>())
      return false;

    if (!jsonDoc["tags"].empty())
    {
      for (const auto& tag : jsonDoc["tags"].items())
      {
        std::string tempTag = tag.value().get<std::string>();

        if (StringUtils::StartsWith(tempTag, TAG_FOR_PLAY_COUNT))
          tags.emplace_back(tempTag);
      }
    }

    return true;
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot find extra recording play count info from OpenWebIf for recording: %s, ID: %s - JSON parse error - message: %s, exception id: %d", __FUNCTION__, recordingEntry.GetTitle().c_str(), recordingEntry.GetRecordingId().c_str(), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return false;
}

std::vector<std::string>& Recordings::GetLocations()
{
  return m_locations;
}

void Recordings::ClearLocations()
{
  m_locations.clear();
}

bool Recordings::LoadLocations()
{
  std::string url;
  if (Settings::GetInstance().GetRecordingsFromCurrentLocationOnly())
    url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/getcurrlocation");
  else
    url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/getlocations");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2locations> element", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2location> element", __FUNCTION__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2location"))
  {
    const std::string strTmp = pNode->GetText();

    m_locations.emplace_back(strTmp);

    Logger::Log(LEVEL_DEBUG, "%s Added '%s' as a recording location", __FUNCTION__, strTmp.c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded '%d' recording locations", __FUNCTION__, m_locations.size());

  return true;
}

void Recordings::LoadRecordings(bool deleted)
{
  std::vector<RecordingEntry> newRecordingsList;
  bool loadError = false;
  for (std::string location : m_locations)
  {
    if (deleted)
      location += TRASH_FOLDER;

    if (!GetRecordingsFromLocation(location, deleted, newRecordingsList))
    {
      loadError = true;
      Logger::Log(LEVEL_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, location.c_str());
    }
  }

  if (!loadError || !newRecordingsList.empty()) //We allow once any recordings are loaded as some bad locations are possible
  {
    ClearRecordings(deleted);
    auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

    std::move(newRecordingsList.begin(), newRecordingsList.end(), std::back_inserter(recordings));
  }
}

bool Recordings::GetRecordingsFromLocation(const std::string recordingLocation, bool deleted, std::vector<RecordingEntry>& recordings)
{
  std::string url;
  std::string directory;

  if (recordingLocation == "default")
  {
    url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/movielist");
    directory = StringUtils::Format("/");
  }
  else
  {
    url = StringUtils::Format("%s%s?dirname=%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/movielist",
                                          WebUtils::URLEncodeInline(recordingLocation).c_str());
    directory = recordingLocation;
  }

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2movielist> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  int iNumRecordings = 0;

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2movie> element, no movies at location: %s", directory.c_str(), __FUNCTION__);
  }
  else
  {
    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2movie"))
    {

      RecordingEntry recordingEntry;

      if (!recordingEntry.UpdateFrom(pNode, directory, deleted, m_channels))
        continue;

      if (m_entryExtractor.IsEnabled())
        m_entryExtractor.ExtractFromEntry(recordingEntry);

      iNumRecordings++;

      recordings.emplace_back(recordingEntry);
      m_recordingsIdMap.insert({recordingEntry.GetRecordingId(), recordingEntry});

      Logger::Log(LEVEL_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, recordingEntry.GetTitle().c_str(), recordingEntry.GetStartTime(), recordingEntry.GetDuration());
    }

    Logger::Log(LEVEL_INFO, "%s Loaded %u Recording Entries from folder '%s'", __FUNCTION__, iNumRecordings, recordingLocation.c_str());
  }
  return true;
}