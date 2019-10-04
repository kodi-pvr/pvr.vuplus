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

#include "Epg.h"

#include "../Enigma2.h"
#include "../client.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <chrono>
#include <cmath>
#include <regex>

#include <kodi/util/XMLUtils.h>
#include <nlohmann/json.hpp>
#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using json = nlohmann::json;

Epg::Epg(enigma2::extract::EpgEntryExtractor& entryExtractor, int epgMaxDays)
      : m_entryExtractor(entryExtractor), m_epgMaxDays(epgMaxDays) {}

Epg::Epg(const Epg& epg) : m_entryExtractor(epg.m_entryExtractor) {}

bool Epg::Initialise(enigma2::Channels& channels, enigma2::ChannelGroups& channelGroups)
{
  m_epgMaxDaysSeconds = m_epgMaxDays * 24 * 60 * 60;

  auto started = std::chrono::high_resolution_clock::now();
  Logger::Log(LEVEL_DEBUG, "%s Initial EPG Load Start", __FUNCTION__);

  //clear current data structures
  m_epgChannels.clear();
  m_epgChannelsMap.clear();
  m_readInitialEpgChannelsMap.clear();
  m_needsInitialEpgChannelsMap.clear();
  m_initialEpgReady = false;

  //add an initial EPG data per channel uId, sref and initial EPG
  for (auto& channel : channels.GetChannelsList())
  {
    std::shared_ptr<data::EpgChannel> newEpgChannel = std::make_shared<EpgChannel>();

    newEpgChannel->SetRadio(channel->IsRadio());
    newEpgChannel->SetUniqueId(channel->GetUniqueId());
    newEpgChannel->SetChannelName(channel->GetChannelName());
    newEpgChannel->SetServiceReference(channel->GetServiceReference());

    m_epgChannels.emplace_back(newEpgChannel);

    m_epgChannelsMap.insert({newEpgChannel->GetServiceReference(), newEpgChannel});
    m_readInitialEpgChannelsMap.insert({newEpgChannel->GetServiceReference(), newEpgChannel});
    m_needsInitialEpgChannelsMap.insert({newEpgChannel->GetServiceReference(), newEpgChannel});
  }

  int lastScannedIgnoreSuccessCount = std::round((1 - LAST_SCANNED_INITIAL_EPG_SUCCESS_PERCENT) * m_epgChannels.size());

  std::vector<std::shared_ptr<ChannelGroup>> groupList;

  std::shared_ptr<ChannelGroup> newChannelGroup = std::make_shared<ChannelGroup>();
  newChannelGroup->SetRadio(false);
  newChannelGroup->SetGroupName("Last Scanned"); //Name not important
  newChannelGroup->SetServiceReference("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.LastScanned.tv\" ORDER BY bouquet");
  newChannelGroup->SetLastScannedGroup(true);

  groupList.emplace_back(newChannelGroup);
  for (auto& group : channelGroups.GetChannelGroupsList())
  {
    if (!group->IsLastScannedGroup())
      groupList.emplace_back(group);
  }

  //load each group and if we don't already have it's intial EPG then load those entries
  for (auto& group : groupList)
  {
    LoadInitialEPGForGroup(group);

    //Remove channels that now have an initial EPG
    for (auto& epgChannel : m_epgChannels)
    {
      if (epgChannel->GetInitialEPG().size() > 0)
        InitialEpgLoadedForChannel(epgChannel->GetServiceReference());
    }

    Logger::Log(LEVEL_DEBUG, "%s Initial EPG Progress - Remaining channels %d, Min Channels for completion %d", __FUNCTION__, m_needsInitialEpgChannelsMap.size(), lastScannedIgnoreSuccessCount);

    for (auto pair : m_needsInitialEpgChannelsMap)
      Logger::Log(LEVEL_DEBUG, "%s - Initial EPG Progress - Remaining channel: %s - sref: %s", __FUNCTION__, pair.second->GetChannelName().c_str(), pair.first.c_str());

    if (group->IsLastScannedGroup() && m_needsInitialEpgChannelsMap.size() <= lastScannedIgnoreSuccessCount)
      break;
  }

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_NOTICE, "%s Initial EPG Loaded - %d (ms)", __FUNCTION__, milliseconds);

  m_initialEpgReady = true;

  return true;
}

std::shared_ptr<data::EpgChannel> Epg::GetEpgChannel(const std::string& serviceReference)
{
  std::shared_ptr<data::EpgChannel> epgChannel = std::make_shared<data::EpgChannel>();

  auto epgChannelSearch = m_epgChannelsMap.find(serviceReference);
  if (epgChannelSearch != m_epgChannelsMap.end())
    epgChannel = epgChannelSearch->second;

  return epgChannel;
}

std::shared_ptr<data::EpgChannel> Epg::GetEpgChannelNeedingInitialEpg(const std::string& serviceReference)
{
  std::shared_ptr<data::EpgChannel> epgChannel = std::make_shared<data::EpgChannel>();

  auto initialEpgChannelSearch = m_needsInitialEpgChannelsMap.find(serviceReference);
  if (initialEpgChannelSearch != m_needsInitialEpgChannelsMap.end())
    epgChannel = initialEpgChannelSearch->second;

  return epgChannel;
}

bool Epg::ChannelNeedsInitialEpg(const std::string& serviceReference)
{
  auto needsInitialEpgSearch = m_needsInitialEpgChannelsMap.find(serviceReference);

  return needsInitialEpgSearch != m_needsInitialEpgChannelsMap.end();
}

bool Epg::InitialEpgLoadedForChannel(const std::string& serviceReference)
{
  return m_needsInitialEpgChannelsMap.erase(serviceReference) == 1;
}

bool Epg::IsInitialEpgCompleted()
{
  Logger::Log(LEVEL_DEBUG, "%s Waiting to Get Initial EPG for %d remaining channels", __FUNCTION__, m_readInitialEpgChannelsMap.size());

  return m_readInitialEpgChannelsMap.size() == 0;
}

void Epg::TriggerEpgUpdatesForChannels()
{
  for (auto& epgChannel : m_epgChannels)
  {
    //We want to trigger full updates only so let's make sure it's not an initialEpg
    if (epgChannel->RequiresInitialEpg())
    {
      epgChannel->SetRequiresInitialEpg(false);
      epgChannel->GetInitialEPG().clear();
      m_readInitialEpgChannelsMap.erase(epgChannel->GetServiceReference());
    }

    Logger::Log(LEVEL_DEBUG, "%s - Trigger EPG update for channel: %s (%d)", __FUNCTION__, epgChannel->GetChannelName().c_str(), epgChannel->GetUniqueId());
    PVR->TriggerEpgUpdate(epgChannel->GetUniqueId());
  }
}

void Epg::MarkChannelAsInitialEpgRead(const std::string& serviceReference)
{
  std::shared_ptr<data::EpgChannel> epgChannel = GetEpgChannel(serviceReference);

  if (epgChannel->RequiresInitialEpg())
  {
    epgChannel->SetRequiresInitialEpg(false);
    epgChannel->GetInitialEPG().clear();
    m_readInitialEpgChannelsMap.erase(epgChannel->GetServiceReference());
  }
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, const std::string& serviceReference, time_t iStart, time_t iEnd)
{
  std::shared_ptr<data::EpgChannel> epgChannel = GetEpgChannel(serviceReference);

  if (epgChannel)
  {
    Logger::Log(LEVEL_DEBUG, "%s Getting EPG for channel '%s'", __FUNCTION__, epgChannel->GetChannelName().c_str());

    if (epgChannel->RequiresInitialEpg())
    {
      epgChannel->SetRequiresInitialEpg(false);

      return TransferInitialEPGForChannel(handle, epgChannel, iStart, iEnd);
    }

    const std::string url = StringUtils::Format("%s%s%s", Settings::GetInstance().GetConnectionURL().c_str(),
                                                "web/epgservice?sRef=", WebUtils::URLEncodeInline(serviceReference).c_str());

    const std::string strXML = WebUtils::GetHttpXML(url);

    int iNumEPG = 0;

    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return PVR_ERROR_SERVER_ERROR;
    }

    TiXmlHandle hDoc(&xmlDoc);

    TiXmlElement* pElem = hDoc.FirstChildElement("e2eventlist").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_NOTICE, "%s could not find <e2eventlist> element!", __FUNCTION__);
      // Return "NO_ERROR" as the EPG could be empty for this channel
      return PVR_ERROR_NO_ERROR;
    }

    TiXmlHandle hRoot = TiXmlHandle(pElem);

    TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

    if (!pNode)
    {
      Logger::Log(LEVEL_NOTICE, "%s Could not find <e2event> element", __FUNCTION__);
      // RETURN "NO_ERROR" as the EPG could be empty for this channel
      return PVR_ERROR_NO_ERROR;
    }

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2event"))
    {
      EpgEntry entry;

      if (!entry.UpdateFrom(pNode, epgChannel, iStart, iEnd))
        continue;

      if (m_entryExtractor.IsEnabled())
        m_entryExtractor.ExtractFromEntry(entry);

      EPG_TAG broadcast = {0};

      entry.UpdateTo(broadcast);

      PVR->TransferEpgEntry(handle, &broadcast);

      iNumEPG++;

      Logger::Log(LEVEL_TRACE, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.GetChannelId(), entry.GetStartTime(), entry.GetEndTime());
    }

    iNumEPG += TransferTimerBasedEntries(handle, epgChannel->GetUniqueId());

    Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, epgChannel->GetChannelName().c_str());
  }
  else
  {
    Logger::Log(LEVEL_NOTICE, "%s EPG requested for unknown channel reference: '%s'", __FUNCTION__, serviceReference.c_str());
  }


  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Epg::TransferInitialEPGForChannel(ADDON_HANDLE handle, const std::shared_ptr<EpgChannel>& epgChannel, time_t iStart, time_t iEnd)
{
  for (const auto& entry : epgChannel->GetInitialEPG())
  {
    EPG_TAG broadcast = {0};

    entry.UpdateTo(broadcast);

    PVR->TransferEpgEntry(handle, &broadcast);
  }

  epgChannel->GetInitialEPG().clear();
  m_readInitialEpgChannelsMap.erase(epgChannel->GetServiceReference());

  TransferTimerBasedEntries(handle, epgChannel->GetUniqueId());

  return PVR_ERROR_NO_ERROR;
}

std::string Epg::LoadEPGEntryShortDescription(const std::string& serviceReference, unsigned int epgUid)
{
  std::string shortDescription;

  const std::string jsonUrl = StringUtils::Format("%sapi/event?sref=%s&idev=%u", Settings::GetInstance().GetConnectionURL().c_str(),
                                                  WebUtils::URLEncodeInline(serviceReference).c_str(), epgUid);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["event"].empty())
    {
      for (const auto& element : jsonDoc["event"].items())
      {
        if (element.key() == "shortdesc")
        {
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event short description for sref: %s, epgId: %u - '%s'", __FUNCTION__, serviceReference.c_str(), epgUid, element.value().get<std::string>().c_str());
          shortDescription = element.value().get<std::string>();
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load short descrption from OpenWebIf for sref: %s, epgId: %u - JSON parse error - message: %s, exception id: %d", __FUNCTION__, serviceReference.c_str(), epgUid, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return shortDescription;
}

EpgPartialEntry Epg::LoadEPGEntryPartialDetails(const std::string& serviceReference, unsigned int epgUid)
{
  EpgPartialEntry partialEntry;

  const std::string jsonUrl = StringUtils::Format("%sapi/event?sref=%s&idev=%u", Settings::GetInstance().GetConnectionURL().c_str(),
                                                  WebUtils::URLEncodeInline(serviceReference).c_str(), epgUid);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["event"].empty())
    {
      for (const auto& element : jsonDoc["event"].items())
      {
        if (element.key() == "shortdesc")
          partialEntry.SetPlotOutline(element.value().get<std::string>());
        if (element.key() == "longdesc")
          partialEntry.SetPlot(element.value().get<std::string>());
        else if (element.key() == "title")
          partialEntry.SetTitle(element.value().get<std::string>());
        else if (element.key() == "id")
          partialEntry.SetEpgUid(element.value().get<unsigned int>());
        else if (element.key() == "genreid")
        {
          int genreId = element.value().get<int>();
          partialEntry.SetGenreType(genreId & 0xF0);
          partialEntry.SetGenreSubType(genreId & 0x0F);
        }
      }

      if (partialEntry.EntryFound())
      {
        Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event partial details for sref: %s, epgId: %u - title: %s - '%s'", __FUNCTION__, serviceReference.c_str(), epgUid, partialEntry.GetTitle().c_str(), partialEntry.GetPlotOutline().c_str());
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot event details from OpenWebIf for sref: %s, epgId: %u - JSON parse error - message: %s, exception id: %d", __FUNCTION__, serviceReference.c_str(), epgUid, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return partialEntry;
}

EpgPartialEntry Epg::LoadEPGEntryPartialDetails(const std::string& serviceReference, time_t startTime)
{
  EpgPartialEntry partialEntry;

  const std::string jsonUrl = StringUtils::Format("%sapi/epgservice?sRef=%s&time=%ld&endTime=1", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(serviceReference).c_str(), startTime);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["events"].empty())
    {
      for (const auto& event : jsonDoc["events"].items())
      {
        for (const auto& element : event.value().items())
        {
          if (element.key() == "shortdesc")
            partialEntry.SetPlotOutline(element.value().get<std::string>());
          if (element.key() == "longdesc")
            partialEntry.SetPlot(element.value().get<std::string>());
          else if (element.key() == "title")
            partialEntry.SetTitle(element.value().get<std::string>());
          else if (element.key() == "id")
            partialEntry.SetEpgUid(element.value().get<unsigned int>());
          else if (element.key() == "genreid")
          {
            int genreId = element.value().get<int>();
            partialEntry.SetGenreType(genreId & 0xF0);
            partialEntry.SetGenreSubType(genreId & 0x0F);
          }
        }

        if (partialEntry.EntryFound())
        {
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event partial details for sref: %s, time: %ld - title: %s, epgId: %u - '%s'", __FUNCTION__, serviceReference.c_str(), startTime, partialEntry.GetTitle().c_str(), partialEntry.GetEpgUid(), partialEntry.GetPlotOutline().c_str());
        }

        break; //We only want first event
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot event details from OpenWebIf for sref: %s, time: %ld - JSON parse error - message: %s, exception id: %d", __FUNCTION__, serviceReference.c_str(), startTime, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return partialEntry;
}

std::string Epg::FindServiceReference(const std::string& title, int epgUid, time_t startTime, time_t endTime) const
{
  std::string serviceReference;

  const auto started = std::chrono::high_resolution_clock::now();

  const std::string jsonUrl = StringUtils::Format("%sapi/epgsearch?search=%s&endtime=%ld", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(title).c_str(), endTime);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["events"].empty())
    {
      for (const auto& event : jsonDoc["events"].items())
      {
        if (event.value()["title"].get<std::string>() == title &&
            event.value()["id"].get<int>() == epgUid &&
            event.value()["begin_timestamp"].get<time_t>() == startTime &&
            event.value()["duration_sec"].get<int>() == (endTime - startTime))
        {
          serviceReference = Channel::NormaliseServiceReference(event.value()["sref"].get<std::string>());

          break; //We only want first event
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot service reference from OpenWebIf for: %s, epgUid: %d, start time: %ld, end time: %ld  - JSON parse error - message: %s, exception id: %d", __FUNCTION__, title.c_str(), epgUid, startTime, endTime, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_DEBUG, "%s Service reference search time - %d (ms)", __FUNCTION__, milliseconds);

  return serviceReference;
}

bool Epg::LoadInitialEPGForGroup(const std::shared_ptr<ChannelGroup> group)
{
  const std::string url = StringUtils::Format("%s%s%s", Settings::GetInstance().GetConnectionURL().c_str(),
                                              "web/epgnownext?bRef=", WebUtils::URLEncodeInline(group->GetServiceReference()).c_str());

  const std::string strXML = WebUtils::GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2eventlist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_NOTICE, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2event> element", __FUNCTION__);
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2event"))
  {
    EpgEntry entry;

    if (!entry.UpdateFrom(pNode, m_needsInitialEpgChannelsMap))
      continue;

    std::shared_ptr<data::EpgChannel> epgChannel = GetEpgChannelNeedingInitialEpg(entry.GetServiceReference());

    if (m_entryExtractor.IsEnabled())
      m_entryExtractor.ExtractFromEntry(entry);

    iNumEPG++;

    epgChannel->GetInitialEPG().emplace_back(entry);
    Logger::Log(LEVEL_TRACE, "%s Added Initial EPG Entry for: %s, %d, %s", __FUNCTION__, epgChannel->GetChannelName().c_str(), epgChannel->GetUniqueId(), epgChannel->GetServiceReference().c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group->GetGroupName().c_str());
  return true;
}

void Epg::UpdateTimerEPGFallbackEntries(const std::vector<enigma2::data::EpgEntry>& timerBasedEntries)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  time_t now = std::time(nullptr);
  time_t until = now + m_epgMaxDaysSeconds;

  m_timerBasedEntries.clear();

  for (auto& timerBasedEntry : timerBasedEntries)
  {
    if (timerBasedEntry.GetEndTime() < now || timerBasedEntry.GetEndTime() > until)
      m_timerBasedEntries.emplace_back(timerBasedEntry);
  }
}

int Epg::TransferTimerBasedEntries(ADDON_HANDLE handle, int epgChannelId)
{
  int numTransferred = 0;

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto& timerBasedEntry : m_timerBasedEntries)
  {
    if (epgChannelId == timerBasedEntry.GetChannelId())
    {
      EPG_TAG broadcast = {0};

      timerBasedEntry.UpdateTo(broadcast);

      PVR->TransferEpgEntry(handle, &broadcast);

      numTransferred++;
    }
  }

  return numTransferred;
}
