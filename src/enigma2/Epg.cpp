/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Epg.h"

#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"
#include "utilities/XMLUtils.h"

#include <chrono>
#include <cmath>
#include <regex>

#include <kodi/tools/StringUtils.h>
#include <nlohmann/json.hpp>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using namespace kodi::tools;
using json = nlohmann::json;

Epg::Epg(IConnectionListener& connectionListener, enigma2::Channels& channels, enigma2::extract::EpgEntryExtractor& entryExtractor, std::shared_ptr<enigma2::InstanceSettings>& settings, int epgMaxPastDays, int epgMaxFutureDays)
      : m_connectionListener(connectionListener), m_channels(channels), m_entryExtractor(entryExtractor), m_settings(settings), m_epgMaxPastDays(epgMaxPastDays),
        m_epgMaxFutureDays(epgMaxFutureDays)
{
  m_channelsMap = channels.GetChannelsServiceReferenceMap();
}

Epg::Epg(const Epg& epg) : m_connectionListener(epg.m_connectionListener), m_channels(epg.m_channels), m_entryExtractor(epg.m_entryExtractor), m_settings(epg.m_settings) {}

bool Epg::Initialise(enigma2::Channels& channels, enigma2::ChannelGroups& channelGroups)
{
  SetEPGMaxPastDays(m_epgMaxPastDays);
  SetEPGMaxFutureDays(m_epgMaxFutureDays);

  return true;
}

PVR_ERROR Epg::GetEPGForChannel(const std::string& serviceReference, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results)
{
  std::shared_ptr<data::Channel> channel = m_channels.GetChannel(serviceReference);

  if (channel)
  {
    Logger::Log(LEVEL_DEBUG, "%s Getting EPG for channel '%s'", __func__, channel->GetChannelName().c_str());

    const std::string url = StringUtils::Format("%s%s%s", m_settings->GetConnectionURL().c_str(),
                                                "web/epgservice?sRef=", WebUtils::URLEncodeInline(serviceReference).c_str());

    const std::string strXML = WebUtils::GetHttpXML(url);

    int iNumEPG = 0;

    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return PVR_ERROR_SERVER_ERROR;
    }

    TiXmlHandle hDoc(&xmlDoc);

    TiXmlElement* pElem = hDoc.FirstChildElement("e2eventlist").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_WARNING, "%s could not find <e2eventlist> element for channel: %s", __func__, channel->GetChannelName().c_str());
      // Return "NO_ERROR" as the EPG could be empty for this channel
      return PVR_ERROR_NO_ERROR;
    }

    TiXmlHandle hRoot = TiXmlHandle(pElem);

    TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

    if (!pNode)
    {
      Logger::Log(LEVEL_WARNING, "%s Could not find <e2event> element for channel: %s", __func__, channel->GetChannelName().c_str());
      // RETURN "NO_ERROR" as the EPG could be empty for this channel
      return PVR_ERROR_NO_ERROR;
    }

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2event"))
    {
      EpgEntry entry{m_settings};

      if (!entry.UpdateFrom(pNode, channel, start, end))
        continue;

      if (m_entryExtractor.IsEnabled())
        m_entryExtractor.ExtractFromEntry(entry);

      kodi::addon::PVREPGTag broadcast;

      entry.UpdateTo(broadcast);

      results.Add(broadcast);

      iNumEPG++;

      Logger::Log(LEVEL_TRACE, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __func__, broadcast.GetUniqueBroadcastId(), broadcast.GetTitle().c_str(), entry.GetChannelId(), entry.GetStartTime(), entry.GetEndTime());
    }

    iNumEPG += TransferTimerBasedEntries(results, channel->GetUniqueId());

    Logger::Log(LEVEL_DEBUG, "%s Loaded %u EPG Entries for channel '%s'", __func__, iNumEPG, channel->GetChannelName().c_str());
  }
  else
  {
    Logger::Log(LEVEL_DEBUG, "%s EPG requested for unknown channel reference: '%s'", __func__, serviceReference.c_str());
  }

  return PVR_ERROR_NO_ERROR;
}

void Epg::SetEPGMaxPastDays(int epgMaxPastDays)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_epgMaxPastDays = epgMaxPastDays;

  if (m_epgMaxPastDays > EPG_TIMEFRAME_UNLIMITED)
    m_epgMaxPastDaysSeconds = m_epgMaxPastDays * 24 * 60 * 60;
  else
    m_epgMaxPastDaysSeconds = DEFAULT_EPG_MAX_DAYS * 24 * 60 * 60;
}

void Epg::SetEPGMaxFutureDays(int epgMaxFutureDays)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_epgMaxFutureDays = epgMaxFutureDays;

  if (m_epgMaxFutureDays > EPG_TIMEFRAME_UNLIMITED)
    m_epgMaxFutureDaysSeconds = m_epgMaxFutureDays * 24 * 60 * 60;
  else
    m_epgMaxFutureDaysSeconds = DEFAULT_EPG_MAX_DAYS * 24 * 60 * 60;
}

std::string Epg::LoadEPGEntryShortDescription(const std::string& serviceReference, unsigned int epgUid)
{
  std::string shortDescription;

  const std::string jsonUrl = StringUtils::Format("%sapi/event?sref=%s&idev=%u", m_settings->GetConnectionURL().c_str(),
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
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event short description for sref: %s, epgId: %u - '%s'", __func__, serviceReference.c_str(), epgUid, element.value().get<std::string>().c_str());
          shortDescription = element.value().get<std::string>();
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load short descrption from OpenWebIf for sref: %s, epgId: %u - JSON parse error - message: %s, exception id: %d", __func__, serviceReference.c_str(), epgUid, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return shortDescription;
}

EpgPartialEntry Epg::LoadEPGEntryPartialDetails(const std::string& serviceReference, unsigned int epgUid)
{
  EpgPartialEntry partialEntry;

  Logger::Log(LEVEL_DEBUG, "%s Looking for EPG event partial details for sref: %s, epgUid: %u", __func__, serviceReference.c_str(), epgUid);

  const std::string jsonUrl = StringUtils::Format("%sapi/event?sref=%s&idev=%u", m_settings->GetConnectionURL().c_str(),
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
        Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event partial details for sref: %s, epgId: %u - title: %s - '%s'", __func__, serviceReference.c_str(), epgUid, partialEntry.GetTitle().c_str(), partialEntry.GetPlotOutline().c_str());
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot event details from OpenWebIf for sref: %s, epgId: %u - JSON parse error - message: %s, exception id: %d", __func__, serviceReference.c_str(), epgUid, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return partialEntry;
}

EpgPartialEntry Epg::LoadEPGEntryPartialDetails(const std::string& serviceReference, time_t startTime)
{
  EpgPartialEntry partialEntry;

  Logger::Log(LEVEL_DEBUG, "%s Looking for EPG event partial details for sref: %s, time: %lld", __func__, serviceReference.c_str(), static_cast<long long>(startTime));

  const std::string jsonUrl = StringUtils::Format("%sapi/epgservice?sRef=%s&time=%lld&endTime=1", m_settings->GetConnectionURL().c_str(), WebUtils::URLEncodeInline(serviceReference).c_str(), static_cast<long long>(startTime));

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
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event partial details for sref: %s, time: %lld - title: %s, epgId: %u - '%s'", __func__, serviceReference.c_str(), static_cast<long long>(startTime), partialEntry.GetTitle().c_str(), partialEntry.GetEpgUid(), partialEntry.GetPlotOutline().c_str());

        break; //We only want first event
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot event details from OpenWebIf for sref: %s, time: %lld - JSON parse error - message: %s, exception id: %d", __func__, serviceReference.c_str(), static_cast<long long>(startTime), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return partialEntry;
}

std::string Epg::FindServiceReference(const std::string& title, int epgUid, time_t startTime, time_t endTime) const
{
  std::string serviceReference;

  const auto started = std::chrono::high_resolution_clock::now();

  const std::string jsonUrl = StringUtils::Format("%sapi/epgsearch?search=%s&endtime=%lld", m_settings->GetConnectionURL().c_str(), WebUtils::URLEncodeInline(title).c_str(), static_cast<long long>(endTime));

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
          serviceReference = Channel::NormaliseServiceReference(event.value()["sref"].get<std::string>(), m_settings->UseStandardServiceReference());

          break; //We only want first event
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot retrieve service reference from OpenWebIf for: %s, epgUid: %d, start time: %lld, end time: %lld  - JSON parse error - message: %s, exception id: %d", __func__, title.c_str(), epgUid, static_cast<long long>(startTime), static_cast<long long>(endTime), e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_DEBUG, "%s Service reference search time - %d (ms)", __func__, milliseconds);

  return serviceReference;
}

void Epg::UpdateTimerEPGFallbackEntries(const std::vector<enigma2::data::EpgEntry>& timerBasedEntries)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  time_t now = std::time(nullptr);
  time_t before = now - m_epgMaxPastDaysSeconds;
  time_t until = now + m_epgMaxFutureDaysSeconds;

  m_timerBasedEntries.clear();

  for (auto& timerBasedEntry : timerBasedEntries)
  {
    if (timerBasedEntry.GetEndTime() < before || timerBasedEntry.GetEndTime() > until)
      m_timerBasedEntries.emplace_back(timerBasedEntry);
  }
}

int Epg::TransferTimerBasedEntries(kodi::addon::PVREPGTagsResultSet& results, int channelId)
{
  int numTransferred = 0;

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto& timerBasedEntry : m_timerBasedEntries)
  {
    if (channelId == timerBasedEntry.GetChannelId())
    {
      kodi::addon::PVREPGTag broadcast;

      timerBasedEntry.UpdateTo(broadcast);

      results.Add(broadcast);

      numTransferred++;
    }
  }

  return numTransferred;
}
