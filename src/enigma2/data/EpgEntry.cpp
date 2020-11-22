/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "EpgEntry.h"
#include "Channel.h"
#include "../utilities/XMLUtils.h"

#include <cinttypes>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void EpgEntry::UpdateTo(kodi::addon::PVREPGTag& left) const
{
  left.SetUniqueBroadcastId(m_epgId);
  left.SetTitle(m_title);
  left.SetUniqueChannelId(m_channelId);
  left.SetStartTime(m_startTime);
  left.SetEndTime(m_endTime);
  left.SetPlotOutline(m_plotOutline);
  left.SetPlot(m_plot);
  left.SetOriginalTitle(""); // unused
  left.SetCast(""); // unused
  left.SetDirector(""); // unused
  left.SetWriter(""); // unused
  left.SetYear(m_year);
  left.SetIMDBNumber(""); // unused
  left.SetIconPath(""); // unused
  left.SetGenreType(m_genreType);
  left.SetGenreSubType(m_genreSubType);
  left.SetGenreDescription(m_genreDescription);
  left.SetFirstAired((m_new || m_live || m_premiere) ? m_startTimeW3CDateString.c_str() : "");
  left.SetParentalRating(0); // unused
  left.SetStarRating(0); // unused
  left.SetSeriesNumber(m_seasonNumber);
  left.SetEpisodeNumber(m_episodeNumber);
  left.SetEpisodePartNumber(m_episodePartNumber);
  left.SetEpisodeName(""); // unused
  unsigned int flags = EPG_TAG_FLAG_UNDEFINED;
  if (m_new)
    flags |= EPG_TAG_FLAG_IS_NEW;
  if (m_premiere)
    flags |= EPG_TAG_FLAG_IS_PREMIERE;
  if (m_finale)
    flags |= EPG_TAG_FLAG_IS_FINALE;
  if (m_live)
    flags |= EPG_TAG_FLAG_IS_LIVE;

  left.SetFlags(flags);
}

bool EpgEntry::UpdateFrom(TiXmlElement* eventNode, std::map<std::string, std::shared_ptr<EpgChannel>>& epgChannelsMap)
{
  if (!xml::GetString(eventNode, "e2eventservicereference", m_serviceReference))
    return false;

  // Check whether the current element is not just a label or that it's not an empty record
  if (m_serviceReference.compare(0, 5, "1:64:") == 0)
    return false;

  m_serviceReference = Channel::NormaliseServiceReference(m_serviceReference);

  std::shared_ptr<data::EpgChannel> epgChannel = std::make_shared<data::EpgChannel>();

  auto epgChannelSearch = epgChannelsMap.find(m_serviceReference);
  if (epgChannelSearch != epgChannelsMap.end())
    epgChannel = epgChannelSearch->second;

  if (!epgChannel)
  {
    Logger::Log(LEVEL_DEBUG, "%s could not find channel so skipping entry", __func__);
    return false;
  }

  m_channelId = epgChannel->GetUniqueId();

  return UpdateFrom(eventNode, epgChannel, 0, 0);
}

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::localtime(&time);
  char buffer[16];
  if (tm)
    std::strftime(buffer, 16, "%Y-%m-%d", tm);
  else // negative time or other invalid time_t value
    std::strcpy(buffer, "1970-01-01");

  return buffer;
}

} // unnamed namespace

bool EpgEntry::UpdateFrom(TiXmlElement* eventNode, const std::shared_ptr<EpgChannel>& epgChannel, time_t iStart, time_t iEnd)
{
  std::string strTmp;

  int iTmpStart;
  int iTmp;

  // check and set event starttime and endtimes
  if (!xml::GetInt(eventNode, "e2eventstart", iTmpStart))
    return false;

  // Skip unneccessary events
  if (iStart > iTmpStart)
    return false;

  if (!xml::GetInt(eventNode, "e2eventduration", iTmp))
    return false;

  if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
    return false;

  m_startTime = iTmpStart;
  m_endTime = iTmpStart + iTmp;
  m_startTimeW3CDateString = ParseAsW3CDateString(m_startTime);

  if (!xml::GetInt(eventNode, "e2eventid", iTmp))
    return false;

  m_epgId = iTmp;
  m_channelId = epgChannel->GetUniqueId();

  if (!xml::GetString(eventNode, "e2eventtitle", strTmp))
    return false;

  m_title = strTmp;

  m_serviceReference = epgChannel->GetServiceReference().c_str();

  // Check that it's not an empty record
  if (m_epgId == 0 && m_title == "None")
    return false;

  if (xml::GetString(eventNode, "e2eventdescriptionextended", strTmp))
    m_plot = strTmp;

  if (xml::GetString(eventNode, "e2eventdescription", strTmp))
    m_plotOutline = strTmp;

  ProcessPrependMode(PrependOutline::IN_EPG);

  if (xml::GetString(eventNode, "e2eventgenre", strTmp))
  {
    m_genreDescription = strTmp;

    TiXmlElement* genreNode = eventNode->FirstChildElement("e2eventgenre");
    if (genreNode)
    {
      int genreId = 0;
      if (genreNode->QueryIntAttribute("id", &genreId) == TIXML_SUCCESS)
      {
        m_genreType = genreId & 0xF0;
        m_genreSubType = genreId & 0x0F;
      }
    }
  }

  return true;
}
