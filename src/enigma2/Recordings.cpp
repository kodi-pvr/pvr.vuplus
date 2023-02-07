/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Recordings.h"

#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"
#include "utilities/XMLUtils.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>

#include <kodi/tools/StringUtils.h>
#include <nlohmann/json.hpp>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using namespace kodi::tools;
using json = nlohmann::json;

const std::string Recordings::FILE_NOT_FOUND_RESPONSE_SUFFIX = "not found";

Recordings::Recordings(IConnectionListener& connectionListener, std::shared_ptr<InstanceSettings>& settings, Channels& channels, Providers& providers, enigma2::extract::EpgEntryExtractor& entryExtractor)
  : m_connectionListener(connectionListener), m_settings(settings), m_channels(channels), m_providers(providers), m_entryExtractor(entryExtractor)
{
  std::random_device randomDevice; //Will be used to obtain a seed for the random number engine
  m_randomGenerator = std::mt19937(randomDevice()); //Standard mersenne_twister_engine seeded with randomDevice()
  m_randomDistribution = std::uniform_int_distribution<>(E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MIN, E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MAX);
}

void Recordings::GetRecordings(std::vector<kodi::addon::PVRRecording>& kodiRecordings, bool deleted)
{
  auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  for (auto& recording : recordings)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer recording '%s', Recording Id '%s'", __func__, recording.GetTitle().c_str(), recording.GetRecordingId().c_str());
    kodi::addon::PVRRecording kodiRecording;

    recording.UpdateTo(kodiRecording, m_channels, IsInVirtualRecordingFolder(recording, deleted));

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

void Recordings::GetRecordingEdl(const std::string& recordingId, std::vector<kodi::addon::PVREDLEntry>& edlEntries) const
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
          Logger::Log(LEVEL_INFO, "%s Unable to parse EDL entry for recording '%s' at line %d. Skipping.", __func__,
                      recordingEntry.GetTitle().c_str(), lineNumber);
          continue;
        }

        start += static_cast<float>(m_settings->GetEDLStartTimePadding()) / 1000.0f;
        stop += static_cast<float>(m_settings->GetEDLStopTimePadding()) / 1000.0f;

        start = std::max(start, 0.0f);
        stop = std::max(stop, 0.0f);
        start = std::min(start, stop);
        stop = std::max(start, stop);

        Logger::Log(LEVEL_INFO, "%s EDL for '%s', line %d -  start: %f stop: %f type: %d", __func__, recordingEntry.GetTitle().c_str(), lineNumber, start, stop, type);

        kodi::addon::PVREDLEntry edlEntry;
        edlEntry.SetStart(static_cast<int64_t>(start * 1000.0f));
        edlEntry.SetEnd(static_cast<int64_t>(stop * 1000.0f));
        edlEntry.SetType(static_cast<PVR_EDL_TYPE>(type));

        edlEntries.emplace_back(edlEntry);
      }
    }
  }
}

RecordingEntry Recordings::GetRecording(const std::string& recordingId) const
{
  RecordingEntry entry{m_settings};

  auto recordingPair = m_recordingsIdMap.find(recordingId);
  if (recordingPair != m_recordingsIdMap.end())
  {
    entry = recordingPair->second;
  }

  return entry;
}

bool Recordings::IsInVirtualRecordingFolder(const RecordingEntry& recordingToCheck, bool deleted) const
{
  if (m_settings->GetKeepRecordingsFolders() && !recordingToCheck.InLocationRoot())
    return false;

  const std::string& recordingFolderToCheck = recordingToCheck.GetTitle();
  const auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

  int iMatches = 0;
  for (const auto& recording : recordings)
  {
    if (m_settings->GetKeepRecordingsFolders() && !recording.InLocationRoot())
      continue;

    if (recordingFolderToCheck == recording.GetTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found Recording title '%s' in recordings vector!", __func__, recordingFolderToCheck.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __func__, recordingFolderToCheck.c_str());
        return true;
      }
    }
  }

  return false;
}

PVR_ERROR Recordings::RenameRecording(const kodi::addon::PVRRecording& recording)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());

  if (!recordingEntry.GetRecordingId().empty())
  {
    Logger::Log(LEVEL_DEBUG, "%s Sending rename command for recording '%s' to '%s'", __func__, recordingEntry.GetTitle().c_str(), recording.GetTitle().c_str());
    const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&title=%s", m_settings->GetConnectionURL().c_str(),
                                                    WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                                                    WebUtils::URLEncodeInline(recording.GetTitle()).c_str());
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, m_settings->GetConnectionURL(), strResult))
    {
      m_connectionListener.TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  m_connectionListener.TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Recordings::SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());

  if (!recordingEntry.GetRecordingId().empty())
  {
    if (recording.GetPlayCount() == count)
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

    Logger::Log(LEVEL_DEBUG, "%s Setting playcount for recording '%s' to '%d'", __func__, recordingEntry.GetTitle().c_str(), count);
    const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                                m_settings->GetConnectionURL().c_str(),
                                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                                WebUtils::URLEncodeInline(addTagsArg).c_str());
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, m_settings->GetConnectionURL(), strResult))
    {
      m_connectionListener.TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  m_connectionListener.TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Recordings::SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int lastPlayedPosition)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());

  if (!recordingEntry.GetRecordingId().empty())
  {
    if (recording.GetLastPlayedPosition() == lastPlayedPosition)
      return PVR_ERROR_NO_ERROR;

    std::vector<std::pair<int, int64_t>> cuts;
    std::vector<std::string> oldTags;

    bool readExtraCutsInfo = ReadExtaRecordingCutsInfo(recordingEntry, cuts, oldTags);
    std::string cutsArg;
    bool cutsLastPlayedSet = false;
    if (readExtraCutsInfo && m_settings->GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_AND_E2_INSTANCES)
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

    Logger::Log(LEVEL_DEBUG, "%s Setting last played position for recording '%s' to '%d'", __func__, recordingEntry.GetTitle().c_str(), lastPlayedPosition);

    std::string jsonUrl;
    if (m_settings->GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_INSTANCES || !cutsLastPlayedSet)
    {
      jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                m_settings->GetConnectionURL().c_str(),
                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                WebUtils::URLEncodeInline(addTagsArg).c_str());
    }
    else
    {
      jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s&cuts=%s",
                m_settings->GetConnectionURL().c_str(),
                WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                WebUtils::URLEncodeInline(addTagsArg).c_str(),
                WebUtils::URLEncodeInline(cutsArg).c_str());
    }
    std::string strResult;

    if (WebUtils::SendSimpleJsonCommand(jsonUrl, m_settings->GetConnectionURL(), strResult))
    {
      m_connectionListener.TriggerRecordingUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  m_connectionListener.TriggerRecordingUpdate();

  return PVR_ERROR_SERVER_ERROR;
}

int Recordings::GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());

  time_t now = std::time(nullptr);
  time_t newNextSyncTime = now + m_randomDistribution(m_randomGenerator);

  Logger::Log(LEVEL_DEBUG, "%s Recording: %s - Checking if Next Sync Time: %lld < Now: %lld ", __func__, recordingEntry.GetTitle().c_str(), static_cast<long long>(recordingEntry.GetNextSyncTime()), static_cast<long long>(now));

  if (m_settings->GetRecordingLastPlayedMode() == RecordingLastPlayedMode::ACROSS_KODI_AND_E2_INSTANCES &&
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

      Logger::Log(LEVEL_DEBUG, "%s Setting last played position from E2 cuts file to tags for recording '%s' to '%d'", __func__, recordingEntry.GetTitle().c_str(), lastPlayedPosition);

      std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s&deltag=%s&addtag=%s",
                            m_settings->GetConnectionURL().c_str(),
                            WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                            WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                            WebUtils::URLEncodeInline(addTagsArg).c_str());
      std::string strResult;

      if (WebUtils::SendSimpleJsonCommand(jsonUrl, m_settings->GetConnectionURL(), strResult))
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

PVR_ERROR Recordings::GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& sizeInBytes)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());
  UpdateRecordingSizeFromMovieDetails(recordingEntry);

  Logger::Log(LEVEL_DEBUG, "%s In progress recording size is %lld for sRef: %s", __func__, static_cast<long long>(recordingEntry.GetSizeInBytes()), recording.GetRecordingId().c_str());

  sizeInBytes = recordingEntry.GetSizeInBytes();

  return PVR_ERROR_NO_ERROR;
}

bool Recordings::UpdateRecordingSizeFromMovieDetails(RecordingEntry& recordingEntry)
{
  const std::string jsonUrl = StringUtils::Format("%sapi/moviedetails?sref=%s", m_settings->GetConnectionURL().c_str(),
                                                  WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str());

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (jsonDoc["result"].empty() || !jsonDoc["result"].get<bool>())
      return false;

    if (!jsonDoc["movie"].empty())
    {
      for (const auto& element : jsonDoc["movie"].items())
      {
        if (element.key() == "filesize")
        {
          int64_t sizeInBytes = element.value().get<int64_t>();
          if (sizeInBytes != 0)
            recordingEntry.SetSizeInBytes(sizeInBytes);
          break;
        }
      }
    }

    return true;
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot find extra recording cuts info from OpenWebIf for recording: %s, ID: %s - JSON parse error - message: %s, exception id: %d", __func__, recordingEntry.GetTitle().c_str(), recordingEntry.GetRecordingId().c_str(), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return false;
}

void Recordings::SetRecordingNextSyncTime(RecordingEntry& recordingEntry, time_t nextSyncTime, std::vector<std::string>& oldTags)
{
  Logger::Log(LEVEL_DEBUG, "%s Setting next sync time in tags for recording '%s' to '%lld'", __func__, recordingEntry.GetTitle().c_str(), static_cast<long long>(nextSyncTime));

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
                              m_settings->GetConnectionURL().c_str(),
                              WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(),
                              WebUtils::URLEncodeInline(deleteTagsArg).c_str(),
                              WebUtils::URLEncodeInline(addTagsArg).c_str());
  std::string strResult;

  if (!WebUtils::SendSimpleJsonCommand(jsonUrl, m_settings->GetConnectionURL(), strResult))
  {
    recordingEntry.SetNextSyncTime(nextSyncTime);
    Logger::Log(LEVEL_ERROR, "%s Error setting next sync time for recording '%s' to '%lld'", __func__, recordingEntry.GetTitle().c_str(), static_cast<long long>(nextSyncTime));
  }
}

PVR_ERROR Recordings::DeleteRecording(const kodi::addon::PVRRecording& recinfo)
{
  const std::string strTmp = StringUtils::Format("web/moviedelete?sRef=%s", WebUtils::URLEncodeInline(recinfo.GetRecordingId()).c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, m_settings->GetConnectionURL(), strResult))
    return PVR_ERROR_FAILED;

  // No need to call m_connectionListener.TriggerRecordingUpdate() as it is handled by kodi PVR.
  // In fact when multiple recordings are removed at once, calling it here can cause hanging issues

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Recordings::UndeleteRecording(const kodi::addon::PVRRecording& recording)
{
  auto recordingEntry = GetRecording(recording.GetRecordingId());

  static const std::regex regex(TRASH_FOLDER);

  const std::string newRecordingDirectory = std::regex_replace(recordingEntry.GetDirectory(), regex, "");

  const std::string strTmp = StringUtils::Format("web/moviemove?sRef=%s&dirname=%s", WebUtils::URLEncodeInline(recordingEntry.GetRecordingId()).c_str(), WebUtils::URLEncodeInline(newRecordingDirectory).c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, m_settings->GetConnectionURL(), strResult))
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
    WebUtils::SendSimpleCommand(strTmp, m_settings->GetConnectionURL(), strResult, true);
  }

  return PVR_ERROR_NO_ERROR;
}

bool Recordings::HasRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording)
{
  return GetRecording(recording.GetRecordingId()).HasStreamProgramNumber();
}

int Recordings::GetRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording)
{
  return GetRecording(recording.GetRecordingId()).GetStreamProgramNumber();
}

const std::string Recordings::GetRecordingURL(const kodi::addon::PVRRecording& recinfo)
{
  auto recordingEntry = GetRecording(recinfo.GetRecordingId());

  if (!recordingEntry.GetRecordingId().empty())
    return recordingEntry.GetStreamURL();

  return "";
}

bool Recordings::ReadExtaRecordingCutsInfo(const data::RecordingEntry& recordingEntry, std::vector<std::pair<int, int64_t>>& cuts, std::vector<std::string>& tags)
{
  const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s", m_settings->GetConnectionURL().c_str(),
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
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot find extra recording cuts info from OpenWebIf for recording: %s, ID: %s - JSON parse error - message: %s, exception id: %d", __func__, recordingEntry.GetTitle().c_str(), recordingEntry.GetRecordingId().c_str(), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return false;
}

bool Recordings::ReadExtraRecordingPlayCountInfo(const data::RecordingEntry& recordingEntry, std::vector<std::string>& tags)
{
  const std::string jsonUrl = StringUtils::Format("%sapi/movieinfo?sref=%s", m_settings->GetConnectionURL().c_str(),
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
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot find extra recording play count info from OpenWebIf for recording: %s, ID: %s - JSON parse error - message: %s, exception id: %d", __func__, recordingEntry.GetTitle().c_str(), recordingEntry.GetRecordingId().c_str(), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
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
  if (m_settings->GetRecordingsFromCurrentLocationOnly())
    url = StringUtils::Format("%s%s", m_settings->GetConnectionURL().c_str(), "web/getcurrlocation");
  else
    url = StringUtils::Format("%s%s", m_settings->GetConnectionURL().c_str(), "web/getlocations");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2locations> element", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2location> element", __func__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2location"))
  {
    const std::string strTmp = pNode->GetText();

    m_locations.emplace_back(strTmp);

    Logger::Log(LEVEL_DEBUG, "%s Added '%s' as a recording location", __func__, strTmp.c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded '%d' recording locations", __func__, m_locations.size());

  return true;
}

void Recordings::LoadRecordings(bool deleted)
{
  std::vector<RecordingEntry> newRecordingsList;
  std::unordered_map<std::string, enigma2::data::RecordingEntry> newRecordingsIdMap;
  bool loadError = false;

  auto started = std::chrono::high_resolution_clock::now();
  Logger::Log(LEVEL_INFO, "%s Recordings Load Start: %s", __func__, deleted ? "deleted items" : "recordings");

  for (std::string location : m_locations)
  {
    if (deleted)
      location += TRASH_FOLDER;

    if (!GetRecordingsFromLocation(location, deleted, newRecordingsList, newRecordingsIdMap))
    {
      loadError = true;
      Logger::Log(LEVEL_ERROR, "%s Error fetching lists for folder: '%s'", __func__, location.c_str());
    }
  }

  if (!loadError || !newRecordingsList.empty()) //We allow once any recordings are loaded as some bad locations are possible
  {
    ClearRecordings(deleted);
    auto& recordings = (!deleted) ? m_recordings : m_deletedRecordings;

    std::move(newRecordingsList.begin(), newRecordingsList.end(), std::back_inserter(recordings));
    for (auto& pair : newRecordingsIdMap)
      m_recordingsIdMap.insert(pair);
  }

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
  Logger::Log(LEVEL_INFO, "%s Recordings Load: %s - %d (ms)", __func__, deleted ? "deleted items" : "recordings", milliseconds);
}

namespace
{

std::string GetRecordingsParams(const std::string recordingLocation, bool deleted, bool getRecordingsRecursively, bool supportsMovieListRecursive, bool supportsMovieListOWFInternal)
{
  std::string recordingsParams;

  if (!deleted && getRecordingsRecursively && supportsMovieListRecursive)
  {
    if (recordingLocation == "default")
      recordingsParams = "?recursive=1";
    else
      recordingsParams = "&recursive=1";

    // &internal=true requests that openwebif uses it own OWFMovieList instead of the E2 MovieList
    // becuase the E2 MovieList causes memory leaks
    if (supportsMovieListOWFInternal)
      recordingsParams += "&internal=1";
  }
  else
  {
    // &internal=true requests that openwebif uses it own OWFMovieList instead of the E2 MovieList
    // becuase the E2 MovieList causes memory leaks
    if (supportsMovieListOWFInternal)
    {
      if (recordingLocation == "default")
        recordingsParams += "?internal=1";
      else
        recordingsParams += "&internal=1";
    }
  }

  return recordingsParams;
}

} // unnamed namespace

bool Recordings::GetRecordingsFromLocation(const std::string recordingLocation, bool deleted, std::vector<RecordingEntry>& recordings, std::unordered_map<std::string, enigma2::data::RecordingEntry>& recordingsIdMap)
{
  std::string url;
  std::string directory;

  std::string recordingsParams = GetRecordingsParams(recordingLocation, deleted, m_settings->GetRecordingsRecursively(), m_settings->SupportsMovieListRecursive(), m_settings->SupportsMovieListOWFInternal());

  if (recordingLocation == "default")
  {
    url = StringUtils::Format("%s%s%s", m_settings->GetConnectionURL().c_str(), "web/movielist", recordingsParams.c_str());
    directory = StringUtils::Format("/");
  }
  else
  {
    url = StringUtils::Format("%s%s?dirname=%s%s", m_settings->GetConnectionURL().c_str(), "web/movielist",
                              WebUtils::URLEncodeInline(recordingLocation).c_str(), recordingsParams.c_str());
    directory = recordingLocation;
  }

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2movielist> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  int iNumRecordings = 0;

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2movie> element, no movies at location: %s", __func__, directory.c_str());
  }
  else
  {
    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2movie"))
    {
      RecordingEntry recordingEntry{m_settings};

      if (!recordingEntry.UpdateFrom(pNode, directory, deleted, m_channels))
        continue;

      if (m_entryExtractor.IsEnabled())
        m_entryExtractor.ExtractFromEntry(recordingEntry);

      iNumRecordings++;

      recordings.emplace_back(recordingEntry);
      recordingsIdMap.insert({recordingEntry.GetRecordingId(), recordingEntry});

      Logger::Log(LEVEL_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __func__, recordingEntry.GetTitle().c_str(), recordingEntry.GetStartTime(), recordingEntry.GetDuration());
    }

    Logger::Log(LEVEL_INFO, "%s Loaded %u Recording Entries from folder '%s'", __func__, iNumRecordings, recordingLocation.c_str());
  }
  return true;
}
