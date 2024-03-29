/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Timer.h"

#include "../utilities/XMLUtils.h"

#include <cinttypes>
#include <regex>

#include <kodi/tools/StringUtils.h>
#include <kodi/General.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using namespace kodi::tools;

bool Timer::Like(const Timer& right) const
{
  bool isLike = (m_startTime == right.m_startTime);
  isLike &= (m_endTime == right.m_endTime);
  isLike &= (m_channelId == right.m_channelId);
  isLike &= (m_weekdays == right.m_weekdays);
  isLike &= (m_epgId == right.m_epgId);

  return isLike;
}

bool Timer::operator==(const Timer& right) const
{
  bool isEqual = (m_startTime == right.m_startTime);
  isEqual &= (m_endTime == right.m_endTime);
  isEqual &= (m_channelId == right.m_channelId);
  isEqual &= (m_weekdays == right.m_weekdays);
  isEqual &= (m_epgId == right.m_epgId);
  isEqual &= (m_paddingStartMins == right.m_paddingStartMins);
  isEqual &= (m_paddingEndMins == right.m_paddingEndMins);
  isEqual &= (m_state == right.m_state);
  isEqual &= (m_title == right.m_title);
  isEqual &= (m_plot == right.m_plot);
  isEqual &= (m_tags == right.m_tags);
  isEqual &= (m_plotOutline == right.m_plotOutline);
  isEqual &= (m_plot == right.m_plot);
  isEqual &= (m_genreType == right.m_genreType);
  isEqual &= (m_genreSubType == right.m_genreSubType);
  isEqual &= (m_genreDescription == right.m_genreDescription);
  isEqual &= (m_episodeNumber == right.m_episodeNumber);
  isEqual &= (m_episodePartNumber == right.m_episodePartNumber);
  isEqual &= (m_seasonNumber == right.m_seasonNumber);
  isEqual &= (m_year == right.m_year);

  return isEqual;
}

void Timer::UpdateFrom(const Timer& right)
{
  m_title = right.m_title;
  m_plot = right.m_plot;
  m_channelId = right.m_channelId;
  m_channelName = right.m_channelName;
  m_startTime = right.m_startTime;
  m_endTime = right.m_endTime;
  m_weekdays = right.m_weekdays;
  m_epgId = right.m_epgId;
  m_tags = right.m_tags;
  m_state = right.m_state;
  m_paddingStartMins = right.m_paddingStartMins;
  m_paddingEndMins = right.m_paddingEndMins;
  m_plotOutline = right.m_plotOutline;
  m_plot = right.m_plot;
  m_genreType = right.m_genreType;
  m_genreSubType = right.m_genreSubType;
  m_genreDescription = right.m_genreDescription;
  m_episodeNumber = right.m_episodeNumber;
  m_episodePartNumber = right.m_episodePartNumber;
  m_seasonNumber = right.m_seasonNumber;
  m_year = right.m_year;
}

void Timer::UpdateTo(kodi::addon::PVRTimer& left) const
{
  left.SetTitle(m_title);
  left.SetDirectory("/"); // unused
  left.SetSummary(m_plot);
  left.SetTimerType(m_type);
  left.SetClientChannelUid(m_channelId);
  left.SetStartTime(m_startTime);
  left.SetEndTime(m_endTime);
  left.SetState(m_state);
  left.SetPriority(0); // unused
  left.SetLifetime(0); // unused
  left.SetFirstDay(0); // unused
  left.SetWeekdays(m_weekdays);
  left.SetEPGUid(m_epgId);
  left.SetMarginStart(m_paddingStartMins);
  left.SetMarginEnd(m_paddingEndMins);
  left.SetGenreType(0); // unused
  left.SetGenreSubType(0); // unused
  left.SetClientIndex(m_clientIndex);
  left.SetParentClientIndex(m_parentClientIndex);
}

bool Timer::IsScheduled() const
{
  return m_state == PVR_TIMER_STATE_SCHEDULED || m_state == PVR_TIMER_STATE_RECORDING;
}

bool Timer::IsRunning(std::time_t* now, std::string* channelName, std::time_t startTime) const
{
  if (!IsScheduled())
    return false;
  if (now && !(GetRealStartTime() <= *now && *now <= GetRealEndTime()))
    return false;
  if (channelName && m_channelName != *channelName)
    return false;
  if (GetRealStartTime() != startTime)
    return false;
  return true;
}

bool Timer::IsChildOfParent(const Timer& parent) const
{
  time_t time;
  std::tm timeinfo;
  int weekday = 0;

  time = m_startTime;
  timeinfo = *std::localtime(&time);
  const std::string childStartTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  int tmDayOfWeek = timeinfo.tm_wday - 1;
  if (tmDayOfWeek < 0)
    tmDayOfWeek = 6;
  weekday = (1 << tmDayOfWeek);

  time = m_endTime;
  timeinfo = *std::localtime(&time);
  const std::string childEndTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  time = parent.m_startTime;
  timeinfo = *std::localtime(&time);
  const std::string parentStartTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  time = parent.m_endTime;
  timeinfo = *std::localtime(&time);
  const std::string parentEndTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  bool isChild = true;

  isChild = isChild && (m_title == parent.m_title);
  isChild = isChild && (childStartTime == parentStartTime);
  isChild = isChild && (childEndTime == parentEndTime);
  isChild = isChild && (m_paddingStartMins == parent.m_paddingStartMins);
  isChild = isChild && (m_paddingEndMins == parent.m_paddingEndMins);
  isChild = isChild && (m_channelId == parent.m_channelId);
  isChild = isChild && (weekday & parent.m_weekdays);

  return isChild;
}

bool Timer::UpdateFrom(TiXmlElement* timerNode, Channels& channels)
{
  std::string strTmp;

  int iTmp;
  bool bTmp;
  int iDisabled;

  if (xml::GetString(timerNode, "e2name", strTmp))
    Logger::Log(LEVEL_DEBUG, "%s Processing timer '%s'", __func__, strTmp.c_str());

  if (!xml::GetInt(timerNode, "e2state", iTmp))
    return false;

  if (!xml::GetInt(timerNode, "e2disabled", iDisabled))
    return false;

  m_title = strTmp;

  if (xml::GetString(timerNode, "e2servicereference", strTmp))
  {
    m_serviceReference = strTmp;
    m_channelId = channels.GetChannelUniqueId(Channel::NormaliseServiceReference(strTmp.c_str(), m_settings->UseStandardServiceReference()));
  }

  // Skip timers for channels we don't know about, such as when the addon only uses one bouquet or an old channel referene that doesn't exist
  if (m_channelId == PVR_CHANNEL_INVALID_UID)
  {
    m_channelName = kodi::addon::GetLocalizedString(30520); // Invalid Channel
  }
  else
  {
    m_channelName = channels.GetChannel(m_channelId)->GetChannelName();
  }


  if (!xml::GetInt(timerNode, "e2timebegin", iTmp))
    return false;

  m_startTime = iTmp;

  if (!xml::GetInt(timerNode, "e2timeend", iTmp))
    return false;

  m_endTime = iTmp;

  if (xml::GetString(timerNode, "e2descriptionextended", strTmp))
    m_plot = strTmp;

  if (xml::GetString(timerNode, "e2description", strTmp))
    m_plotOutline = strTmp;

  if (m_plot == "N/A")
    m_plot.clear();

  // Some providers only use PlotOutline (e.g. freesat) and Kodi does not display it, if this is the case swap them
  if (m_plot.empty())
  {
    m_plot = m_plotOutline;
    m_plotOutline.clear();
  }
  else if (m_settings->GetPrependOutline() == PrependOutline::ALWAYS &&
           m_plot != m_plotOutline && !m_plotOutline.empty() && m_plotOutline != "N/A")
  {
    m_plot.insert(0, m_plotOutline + "\n");
    m_plotOutline.clear();
  }

  if (xml::GetInt(timerNode, "e2repeated", iTmp))
    m_weekdays = iTmp;
  else
    m_weekdays = 0;

  if (xml::GetInt(timerNode, "e2eit", iTmp))
    m_epgId = iTmp;
  else
    m_epgId = 0;

  m_state = PVR_TIMER_STATE_NEW;

  if (!xml::GetInt(timerNode, "e2state", iTmp))
    return false;

  Logger::Log(LEVEL_DEBUG, "%s e2state is: %d ", __func__, iTmp);

  if (iTmp == 0)
  {
    m_state = PVR_TIMER_STATE_SCHEDULED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: SCHEDULED", __func__);
  }

  if (iTmp == 2)
  {
    m_state = PVR_TIMER_STATE_RECORDING;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: RECORDING", __func__);
  }

  if (iTmp == 3 && iDisabled == 0)
  {
    m_state = PVR_TIMER_STATE_COMPLETED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: COMPLETED", __func__);
  }

  if (xml::GetBoolean(timerNode, "e2cancled", bTmp))
  {
    if (bTmp)
    {
      // If a timer is cancelled by the backend mark it as an error so it will show as such in the UI
      // We don't use CANCELLED or ABORTED as they are synonymous with DISABLED and we won't use them at all
      // Note there is no user/API action to change to cancelled
      m_state = PVR_TIMER_STATE_ERROR;
      Logger::Log(LEVEL_DEBUG, "%s Timer state is: ERROR", __func__);
    }
  }

  if (iDisabled == 1)
  {
    m_state = PVR_TIMER_STATE_DISABLED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: Disabled", __func__);
  }

  if (m_state == PVR_TIMER_STATE_NEW)
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: NEW", __func__);

  if (m_channelId == PVR_CHANNEL_INVALID_UID)
  {
    m_state = PVR_TIMER_STATE_ERROR;
    Logger::Log(LEVEL_DEBUG, "%s Overriding Timer as channel not found, state is: ERROR", __func__);
  }

  m_tags.clear();
  if (xml::GetString(timerNode, "e2tags", strTmp))
    m_tags = strTmp;

  if (ContainsTag(TAG_FOR_MANUAL_TIMER))
  {
    //We create a Manual tag on Manual timers created from Kodi, this allows us to set the Timer Type correctly
    if (m_weekdays != PVR_WEEKDAY_NONE)
    {
      m_type = Timer::MANUAL_REPEATING;
    }
    else
    {
      m_type = Timer::MANUAL_ONCE;
    }
  }
  else
  { //Default to EPG for all other standard timers
    if (m_weekdays != PVR_WEEKDAY_NONE)
    {
      m_type = Timer::EPG_REPEATING;
    }
    else
    {
      if (ContainsTag(TAG_FOR_AUTOTIMER))
      {
        m_type = Timer::EPG_AUTO_ONCE;

        if (!ContainsTag(TAG_FOR_PADDING))
        {
          //We need to add this as these timers are created by the backend so won't have a padding to read
          m_tags.append(StringUtils::Format(" Padding=%u,%u",
            m_settings->GetDeviceSettings()->GetGlobalRecordingStartMargin(),
            m_settings->GetDeviceSettings()->GetGlobalRecordingEndMargin()));
        }
      }
      else
      {
        m_type = Timer::EPG_ONCE;
      }
    }
  }

  if (ContainsTag(TAG_FOR_PADDING))
  {
    if (std::sscanf(ReadTagValue(TAG_FOR_PADDING).c_str(), "%u,%u", &m_paddingStartMins, &m_paddingEndMins) != 2)
    {
      m_paddingStartMins = 0;
      m_paddingEndMins = 0;
    }
  }

  if (m_paddingStartMins > 0)
    m_startTime += m_paddingStartMins * 60;

  if (m_paddingEndMins > 0)
    m_endTime -= m_paddingEndMins * 60;

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
