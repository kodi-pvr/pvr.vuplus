/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AutoTimer.h"

#include "../utilities/LocalizedString.h"

#include <cinttypes>
#include <cstdlib>

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool AutoTimer::Like(const AutoTimer& right) const
{
  return m_backendId == right.m_backendId;
}

bool AutoTimer::operator==(const AutoTimer& right) const
{
  bool isEqual = (!m_title.compare(right.m_title));
  isEqual &= (m_startTime == right.m_startTime);
  isEqual &= (m_endTime == right.m_endTime);
  isEqual &= (m_channelId == right.m_channelId);
  isEqual &= (m_weekdays == right.m_weekdays);

  isEqual &= (m_searchPhrase == right.m_searchPhrase);
  isEqual &= (m_searchType == right.m_searchType);
  isEqual &= (m_searchCase == right.m_searchCase);
  isEqual &= (m_state == right.m_state);
  isEqual &= (m_searchFulltext == right.m_searchFulltext);
  isEqual &= (m_startAnyTime == right.m_startAnyTime);
  isEqual &= (m_endAnyTime == right.m_endAnyTime);
  isEqual &= (m_anyChannel == right.m_anyChannel);
  isEqual &= (m_deDup == right.m_deDup);
  isEqual &= (m_tags == right.m_tags);

  return isEqual;
}

void AutoTimer::UpdateFrom(const AutoTimer& right)
{
  Timer::UpdateFrom(right);

  m_searchPhrase = right.m_searchPhrase;
  m_encoding = right.m_encoding;
  m_searchCase = right.m_searchCase;
  m_searchType = right.m_searchType;
  m_searchFulltext = right.m_searchFulltext;
  m_startAnyTime = right.m_startAnyTime;
  m_endAnyTime = right.m_endAnyTime;
  m_anyChannel = right.m_anyChannel;
  m_deDup = right.m_deDup;
  m_tags = right.m_tags;
}

void AutoTimer::UpdateTo(PVR_TIMER& left) const
{
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle) - 1);
  strncpy(left.strEpgSearchString, m_searchPhrase.c_str(), sizeof(left.strEpgSearchString) - 1);
  left.iTimerType = m_type;
  if (m_anyChannel)
    left.iClientChannelUid = PVR_TIMER_ANY_CHANNEL;
  else
    left.iClientChannelUid = m_channelId;
  left.startTime = m_startTime;
  left.endTime = m_endTime;
  left.state = m_state;
  left.iPriority = 0; // unused
  left.iLifetime = 0; // unused
  left.firstDay = 0; // unused
  left.iWeekdays = m_weekdays;
  left.iMarginStart = 0; // unused
  left.iMarginEnd = 0; // unused
  left.iGenreType = 0; // unused
  left.iGenreSubType = 0; // unused
  left.iClientIndex = m_clientIndex;
  left.bStartAnyTime = m_startAnyTime;
  left.bEndAnyTime = m_endAnyTime;
  left.bFullTextEpgSearch = m_searchFulltext;
  left.iPreventDuplicateEpisodes = m_deDup;
}

bool AutoTimer::UpdateFrom(TiXmlElement* autoTimerNode, Channels& channels)
{
  std::string strTmp;
  int iTmp;

  m_type = Timer::EPG_AUTO_SEARCH;

  //this is an auto timer so the state is always scheduled unless it's disabled
  m_state = PVR_TIMER_STATE_SCHEDULED;

  m_tags.clear();
  if (XMLUtils::GetString(autoTimerNode, "e2tags", strTmp))
    m_tags = strTmp;

  if (autoTimerNode->QueryStringAttribute("name", &strTmp) == TIXML_SUCCESS)
    m_title = strTmp;

  if (autoTimerNode->QueryStringAttribute("match", &strTmp) == TIXML_SUCCESS)
    m_searchPhrase = strTmp;

  if (autoTimerNode->QueryStringAttribute("enabled", &strTmp) == TIXML_SUCCESS)
  {
    if (strTmp == AUTOTIMER_ENABLED_NO)
    {
      m_state = PVR_TIMER_STATE_DISABLED;
    }
  }

  if (autoTimerNode->QueryIntAttribute("id", &iTmp) == TIXML_SUCCESS)
    m_backendId = iTmp;

  std::string from;
  std::string to;
  std::string avoidDuplicateDescription;
  std::string searchForDuplicateDescription;
  autoTimerNode->QueryStringAttribute("from", &from);
  autoTimerNode->QueryStringAttribute("to", &to);
  autoTimerNode->QueryStringAttribute("avoidDuplicateDescription", &avoidDuplicateDescription);
  autoTimerNode->QueryStringAttribute("searchForDuplicateDescription", &searchForDuplicateDescription);

  if (avoidDuplicateDescription != AUTOTIMER_AVOID_DUPLICATE_DISABLED)
  {
    if (searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE)
      m_deDup = AutoTimer::DeDup::CHECK_TITLE;
    else if (searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC)
      m_deDup = AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC;
    else if (searchForDuplicateDescription.empty() || searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS) //Even though this value should be 2 it is sent as ommitted for this attribute, we'll allow 2 anyway incase it changes in the future
      m_deDup = AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS;
  }

  if (autoTimerNode->QueryStringAttribute("encoding", &strTmp) == TIXML_SUCCESS)
    m_encoding = strTmp;

  if (autoTimerNode->QueryStringAttribute("searchType", &strTmp) == TIXML_SUCCESS)
  {
    m_searchType = strTmp;
    if (strTmp == AUTOTIMER_SEARCH_TYPE_DESCRIPTION)
      m_searchFulltext = true;
  }

  if (autoTimerNode->QueryStringAttribute("searchCase", &strTmp) == TIXML_SUCCESS)
    m_searchCase = strTmp;

  TiXmlElement* serviceNode = autoTimerNode->FirstChildElement("e2service");

  if (serviceNode)
  {
    const TiXmlElement* nextServiceNode = serviceNode->NextSiblingElement("e2service");

    if (!nextServiceNode)
    {
      //If we only have one channel
      if (XMLUtils::GetString(serviceNode, "e2servicereference", strTmp))
      {
        m_channelId = channels.GetChannelUniqueId(Channel::NormaliseServiceReference(strTmp.c_str()));

        // For autotimers for channels we don't know about, such as when the addon only uses one bouquet or an old channel referene that doesn't exist
        // we'll default to any channel (as that is what kodi PVR does) and leave in ERROR state
        if (m_channelId == PVR_CHANNEL_INVALID_UID)
        {
          m_state = PVR_TIMER_STATE_ERROR;
          Logger::Log(LEVEL_DEBUG, "%s Overriding AutoTimer state as channel not found, state is: ERROR", __FUNCTION__);
          m_channelName = LocalizedString(30520); // Invalid Channel
          m_channelId = PVR_TIMER_ANY_CHANNEL;
          m_anyChannel = true;
        }
        else
        {
          m_channelName = channels.GetChannel(m_channelId)->GetChannelName();
        }
      }
    }
    else //otherwise set to any channel
    {
      m_channelId = PVR_TIMER_ANY_CHANNEL;
      m_anyChannel = true;
    }
  }
  else //otherwise set to any channel
  {
    m_channelId = PVR_TIMER_ANY_CHANNEL;
    m_anyChannel = true;
  }

  m_weekdays = 0;

  TiXmlElement* includeNode = autoTimerNode->FirstChildElement("include");

  if (includeNode)
  {
    for (; includeNode != nullptr; includeNode = includeNode->NextSiblingElement("include"))
    {
      std::string includeVal = includeNode->GetText();

      std::string where;
      if (includeNode->QueryStringAttribute("where", &where) == TIXML_SUCCESS)
      {
        if (where == "dayofweek")
        {
          m_weekdays = m_weekdays |= (1 << std::atoi(includeVal.c_str()));
        }
      }
    }
  }

  if (m_weekdays != PVR_WEEKDAY_NONE)
  {
    std::time_t t = std::time(nullptr);
    std::tm timeinfo = *std::localtime(&t);
    timeinfo.tm_sec = 0;
    m_startTime = 0;
    if (!from.empty())
    {
      ParseTime(from, timeinfo);
      m_startTime = std::mktime(&timeinfo);
    }

    timeinfo = *std::localtime(&t);
    timeinfo.tm_sec = 0;
    m_endTime = 0;
    if (!to.empty())
    {
      ParseTime(to, timeinfo);
      m_endTime = std::mktime(&timeinfo);
    }
  }
  else
  {
    for (int i = 0; i < DAYS_IN_WEEK; i++)
    {
      m_weekdays = m_weekdays |= (1 << i);
    }
    m_startAnyTime = true;
    m_endAnyTime = true;
  }

  if (ContainsTag(TAG_FOR_GENRE_ID))
  {
    int genreId = 0;
    if (std::sscanf(ReadTagValue(TAG_FOR_GENRE_ID).c_str(), "0x%02X", &genreId) == 1)
    {
      m_genreType = genreId & 0xF0;
      m_genreSubType = genreId & 0x0F;
    }
    else
    {
      m_genreType = 0;
      m_genreSubType = 0;
    }
  }

  return true;
}

void AutoTimer::ParseTime(const std::string& time, std::tm& timeinfo) const
{
  std::sscanf(time.c_str(), "%02d:%02d", &timeinfo.tm_hour, &timeinfo.tm_min);
}