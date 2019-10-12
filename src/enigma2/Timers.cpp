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

#include "Timers.h"

#include "../Enigma2.h"
#include "../client.h"
#include "utilities/LocalizedString.h"
#include "utilities/Logger.h"
#include "utilities/UpdateState.h"
#include "utilities/WebUtils.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <regex>

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

template<typename T>
T* Timers::GetTimer(std::function<bool(const T&)> func, std::vector<T>& timerlist)
{
  for (auto& timer : timerlist)
  {
    if (func(timer))
      return &timer;
  }
  return nullptr;
}

bool Timers::LoadTimers(std::vector<Timer>& timers) const
{
  const std::string url = StringUtils::Format("%s%s", m_settings.GetConnectionURL().c_str(), "web/timerlist");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2timerlist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2timerlist> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2timer").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2timer> element", __FUNCTION__);
    return true; //No timers is valid
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2timer"))
  {
    Timer newTimer;

    if (!newTimer.UpdateFrom(pNode, m_channels))
      continue;

    if (m_entryExtractor.IsEnabled())
      m_entryExtractor.ExtractFromEntry(newTimer);

    timers.emplace_back(newTimer);

    if ((newTimer.GetType() == Timer::MANUAL_REPEATING || newTimer.GetType() == Timer::EPG_REPEATING) &&
        m_settings.GetGenRepeatTimersEnabled() && m_settings.GetNumGenRepeatTimers() > 0)
    {
      GenerateChildManualRepeatingTimers(&timers, &newTimer);
    }

    Logger::Log(LEVEL_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d', start padding mins '%u', end padding mins '%u'",
                __FUNCTION__, newTimer.GetTitle().c_str(), newTimer.GetStartTime(), newTimer.GetEndTime(), newTimer.GetPaddingStartMins(), newTimer.GetPaddingEndMins());
  }

  Logger::Log(LEVEL_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return true;
}

void Timers::GenerateChildManualRepeatingTimers(std::vector<Timer>* timers, Timer* timer) const
{
  int genTimerCount = 0;
  int weekdays = timer->GetWeekdays();
  const time_t ONE_DAY = 24 * 60 * 60;

  if (m_settings.GetNumGenRepeatTimers() && weekdays != PVR_WEEKDAY_NONE)
  {
    time_t nextStartTime = timer->GetStartTime();
    time_t nextEndTime = timer->GetEndTime();

    for (int i = 0; i < m_settings.GetNumGenRepeatTimers(); i++)
    {
      //Even if one day a week the max we can hit is 3 weeks
      for (int i = 0; i < DAYS_IN_WEEK; i++)
      {
        std::tm nextTimeInfo = *std::localtime(&nextStartTime);

        // Get the weekday and convert to PVR day of week
        int pvrWeekday = nextTimeInfo.tm_wday - 1;
        if (pvrWeekday < 0)
          pvrWeekday = 6;

        if (timer->GetWeekdays() & (1 << pvrWeekday))
        {
          //Create a timer
          Timer newTimer;
          newTimer.SetType(Timer::READONLY_REPEATING_ONCE);
          newTimer.SetTitle(timer->GetTitle());
          newTimer.SetChannelId(timer->GetChannelId());
          newTimer.SetChannelName(timer->GetChannelName());
          newTimer.SetStartTime(nextStartTime);
          newTimer.SetEndTime(nextEndTime);
          newTimer.SetPlot(timer->GetPlot());
          newTimer.SetWeekdays(0);
          newTimer.SetState(PVR_TIMER_STATE_NEW);
          newTimer.SetEpgId(timer->GetEpgId());
          newTimer.SetPaddingStartMins(timer->GetPaddingStartMins());
          newTimer.SetPaddingEndMins(timer->GetPaddingEndMins());

          time_t now = std::time(nullptr);
          if (now < nextStartTime)
            newTimer.SetState(PVR_TIMER_STATE_SCHEDULED);
          else if (nextStartTime <= now && now <= nextEndTime)
            newTimer.SetState(PVR_TIMER_STATE_RECORDING);
          else
            newTimer.SetState(PVR_TIMER_STATE_COMPLETED);

          timers->emplace_back(newTimer);

          genTimerCount++;

          if (genTimerCount >= m_settings.GetNumGenRepeatTimers())
            break;
        }

        nextStartTime += ONE_DAY;
        nextEndTime += ONE_DAY;
      }

      if (genTimerCount >= m_settings.GetNumGenRepeatTimers())
        break;
    }
  }
}

std::string Timers::ConvertToAutoTimerTag(std::string tag)
{
  static const std::regex regex(" ");
  std::string replaceWith = "_";

  return std::regex_replace(tag, regex, replaceWith);
}

bool Timers::LoadAutoTimers(std::vector<AutoTimer>& autoTimers) const
{
  const std::string url = StringUtils::Format("%s%s", m_settings.GetConnectionURL().c_str(), "autotimer");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("autotimer").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <autotimer> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("timer").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <timer> element", __FUNCTION__);
    return true; //No timers is valid
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("timer"))
  {
    AutoTimer newAutoTimer;

    if (!newAutoTimer.UpdateFrom(pNode, m_channels))
      continue;

    autoTimers.emplace_back(newAutoTimer);

    Logger::Log(LEVEL_INFO, "%s fetched AutoTimer entry '%s', begin '%d', end '%d'", __FUNCTION__, newAutoTimer.GetTitle().c_str(), newAutoTimer.GetStartTime(), newAutoTimer.GetEndTime());
  }

  Logger::Log(LEVEL_INFO, "%s fetched %u AutoTimer Entries", __FUNCTION__, autoTimers.size());
  return true;
}

bool Timers::IsAutoTimer(const PVR_TIMER& timer) const
{
  return timer.iTimerType == Timer::Type::EPG_AUTO_SEARCH;
}

void Timers::GetTimerTypes(std::vector<PVR_TIMER_TYPE>& types) const
{
  struct TimerType : PVR_TIMER_TYPE
  {
    TimerType(unsigned int id,
              unsigned int attributes,
              const std::string& description = std::string(),
              const std::vector<std::pair<int, std::string>>& groupValues = std::vector<std::pair<int, std::string>>(),
              const std::vector<std::pair<int, std::string>>& deDupValues = std::vector<std::pair<int, std::string>>(),
              int preventDuplicateEpisodesDefault = AutoTimer::DeDup::DISABLED)
    {
      int i;
      memset(this, 0, sizeof(PVR_TIMER_TYPE));

      iId = id;
      iAttributes = attributes;
      strncpy(strDescription, description.c_str(), sizeof(strDescription) - 1);

      if ((iRecordingGroupSize = groupValues.size()))
        iRecordingGroupDefault = groupValues[0].first;
      i = 0;
      for (const auto& group : groupValues)
      {
        recordingGroup[i].iValue = group.first;
        strncpy(recordingGroup[i].strDescription, group.second.c_str(), sizeof(recordingGroup[i].strDescription) - 1);
        i++;
      }

      if ((iPreventDuplicateEpisodesSize = deDupValues.size()))
        iPreventDuplicateEpisodesDefault = preventDuplicateEpisodesDefault;
      i = 0;
      for (const auto& deDup : deDupValues)
      {
        preventDuplicateEpisodes[i].iValue = deDup.first;
        strncpy(preventDuplicateEpisodes[i].strDescription, deDup.second.c_str(), sizeof(preventDuplicateEpisodes[i].strDescription) - 1);
        i++;
      }
    }
  };

  /* PVR_Timer.iRecordingGroup values and presentation.*/
  std::vector<std::pair<int, std::string>> groupValues = {
      {0, LocalizedString(30410)}, //automatic
  };
  for (const auto& recf : m_locations)
    groupValues.emplace_back(groupValues.size(), recf);

  /* One-shot manual (time and channel based) */
  TimerType* t = new TimerType(
    Timer::Type::MANUAL_ONCE,
    PVR_TIMER_TYPE_IS_MANUAL                 |
    PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
    PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
    PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP  |
    PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE,
    LocalizedString(30422), // Once off time/channel based
    groupValues);
  types.emplace_back(*t);
  delete t;

  /* One-shot generated by manual repeating timer - note these are completely read only and cannot be edited */
  t = new TimerType(
    Timer::Type::READONLY_REPEATING_ONCE,
    PVR_TIMER_TYPE_IS_MANUAL                 |
    PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES     |
    PVR_TIMER_TYPE_IS_READONLY               |
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
    PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
    PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP,
    LocalizedString(30421), // Once off timer (set by repeating time/channel based rule)
    groupValues);
  types.emplace_back(*t);
  delete t;

  /* Repeating manual (time and channel based) */
  t = new TimerType(
    Timer::Type::MANUAL_REPEATING,
    PVR_TIMER_TYPE_IS_MANUAL                 |
    PVR_TIMER_TYPE_IS_REPEATING              |
    PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
    PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
    PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
    PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS         |
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP  |
    PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE,
    LocalizedString(30423), // Repeating time/channel based
    groupValues);
  types.emplace_back(*t);
  delete t;

  /* One-shot epg based */
  t = new TimerType(
    Timer::Type::EPG_ONCE,
    PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
    PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
    PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
    PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE,
    LocalizedString(30424)); // One time guide-based
  types.emplace_back(*t);
  delete t;

  if (!Settings::GetInstance().SupportsAutoTimers() || !m_settings.GetAutoTimersEnabled())
  {
    // Allow this type of timer to be created from kodi if autotimers are not available
    // as otherwise there is no way to create a repeating EPG timer rule
    /* Repeating epg based */
    t = new TimerType(
      Timer::Type::EPG_REPEATING,
      PVR_TIMER_TYPE_IS_REPEATING              |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS,
      LocalizedString(30425)); // Repeating guide-based
    types.emplace_back(*t);
    delete t;
  }
  else
  {
    // This type can only be created on the Enigma2 device (i.e. not via kodi PVR) if autotimers are used.
    // In this case this type is superflous and just clutters the UI on creation.
    /* Repeating epg based */
    t = new TimerType(
      Timer::Type::EPG_REPEATING,
      PVR_TIMER_TYPE_IS_REPEATING              |
      PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES     |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS,
      LocalizedString(30425)); // Repeating guide-based
    types.emplace_back(*t);
    delete t;

    /* PVR_Timer.iPreventDuplicateEpisodes values and presentation.*/
    static std::vector<std::pair<int, std::string>> deDupValues =
    {
      { AutoTimer::DeDup::DISABLED,                   LocalizedString(30430) },
      { AutoTimer::DeDup::CHECK_TITLE,                LocalizedString(30431) },
      { AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC, LocalizedString(30432) },
      { AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS,  LocalizedString(30433) },
    };

     /* epg auto search */
    t = new TimerType(
      Timer::Type::EPG_AUTO_SEARCH,
      PVR_TIMER_TYPE_IS_REPEATING                |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE     |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS           |
      PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL        |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME           |
      PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME      |
      PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME        |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS           |
      PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH    |
      PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH |
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP    |
      PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES |
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE,
      LocalizedString(30426), // Auto guide-based
      groupValues, deDupValues, AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS);
    types.emplace_back(*t);
    delete t;

    /* One-shot created by epg auto search */
    t = new TimerType(
      Timer::Type::EPG_AUTO_ONCE,
      PVR_TIMER_TYPE_IS_MANUAL                 |
      PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES     |
      PVR_TIMER_TYPE_IS_READONLY               |
      PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE  |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP |
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE,
      LocalizedString(30420), // Once off timer (set by auto guide-based rule)
      groupValues);
    types.emplace_back(*t);
    delete t;
  }
}

int Timers::GetTimerCount() const
{
  return m_timers.size();
}

int Timers::GetAutoTimerCount() const
{
  return m_autotimers.size();
}

void Timers::GetTimers(std::vector<PVR_TIMER>& timers) const
{
  for (const auto& timer : m_timers)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.GetTitle().c_str(), timer.GetClientIndex());
    PVR_TIMER tag = {0};

    timer.UpdateTo(tag);

    timers.emplace_back(tag);
  }
}

void Timers::GetAutoTimers(std::vector<PVR_TIMER>& timers) const
{
  for (const auto& autoTimer : m_autotimers)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, autoTimer.GetTitle().c_str(),
                autoTimer.GetClientIndex());
    PVR_TIMER tag = {0};

    autoTimer.UpdateTo(tag);

    timers.emplace_back(tag);
  }
}

Timer* Timers::GetTimer(std::function<bool(const Timer&)> func)
{
  return GetTimer<Timer>(func, m_timers);
}

AutoTimer* Timers::GetAutoTimer(std::function<bool(const AutoTimer&)> func)
{
  return GetTimer<AutoTimer>(func, m_autotimers);
}

PVR_ERROR Timers::AddTimer(const PVR_TIMER& timer)
{
  if (IsAutoTimer(timer))
    return AddAutoTimer(timer);

  Logger::Log(LEVEL_DEBUG, "%s - Start", __FUNCTION__);

  const std::string serviceReference = m_channels.GetChannel(timer.iClientChannelUid)->GetServiceReference().c_str();
  Tags tags;

  if (timer.iTimerType == Timer::MANUAL_ONCE || timer.iTimerType == Timer::MANUAL_REPEATING)
    tags.AddTag(TAG_FOR_MANUAL_TIMER);
  else
    tags.AddTag(TAG_FOR_EPG_TIMER);

  if (m_channels.GetChannel(timer.iClientChannelUid)->IsRadio())
    tags.AddTag(TAG_FOR_CHANNEL_TYPE, VALUE_FOR_CHANNEL_TYPE_RADIO);
  else
    tags.AddTag(TAG_FOR_CHANNEL_TYPE, VALUE_FOR_CHANNEL_TYPE_TV);

  tags.AddTag(TAG_FOR_CHANNEL_REFERENCE, serviceReference, true);

  unsigned int startPadding = timer.iMarginStart;
  unsigned int endPadding = timer.iMarginEnd;

  if (startPadding == 0 && endPadding == 0)
  {
    startPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingStartMargin();
    endPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingEndMargin();
  }

  bool alreadyStarted;
  time_t startTime, endTime;
  if ((timer.startTime - (startPadding * 60)) < std::time(nullptr))
  {
    alreadyStarted = true;
    startTime = std::time(nullptr);
    startPadding = 0;
  }
  else
  {
    startTime = timer.startTime - (startPadding * 60);
  }
  endTime = timer.endTime + (endPadding * 60);
  endPadding = endPadding;

  tags.AddTag(TAG_FOR_PADDING, StringUtils::Format("%u,%u", startPadding, endPadding));

  std::string title = timer.strTitle;
  std::string description = timer.strSummary;
  unsigned int epgUid = timer.iEpgUid;
  bool foundEntry = false;

  if (Settings::GetInstance().IsOpenWebIf() && (timer.iTimerType == Timer::EPG_ONCE || timer.iTimerType == Timer::MANUAL_ONCE))
  {
    // We try to find the EPG Entry and use it's details
    EpgPartialEntry partialEntry = m_epg.LoadEPGEntryPartialDetails(serviceReference, alreadyStarted ? startTime : timer.startTime);

    if (partialEntry.EntryFound())
    {
      foundEntry = true;

      /* Note that plot (long desc) is automatically written to a timer entry by the backend
         therefore we only need to send outline as description to preserve both.
         Once a timer completes, long description will be cleared so if description
         is not populated we set it to the value of long description
         */
      title = partialEntry.GetTitle();
      description = partialEntry.GetPlotOutline();
      epgUid = partialEntry.GetEpgUid();

      // Very important for providers that only use the plot field.
      if (description.empty())
        description = partialEntry.GetPlot();

      tags.AddTag(TAG_FOR_GENRE_ID, StringUtils::Format("0x%02X", partialEntry.GetGenreType() | partialEntry.GetGenreSubType()));
    }
  }

  if (!foundEntry)
    tags.AddTag(TAG_FOR_GENRE_ID, StringUtils::Format("0x%02X", timer.iGenreType | timer.iGenreSubType));

  std::string strTmp;
  if (!m_settings.GetRecordingPath().empty())
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&tags=%s&dirname=&s",
              WebUtils::URLEncodeInline(serviceReference).c_str(), timer.iWeekdays, startTime, endTime,
              WebUtils::URLEncodeInline(title).c_str(), WebUtils::URLEncodeInline(description).c_str(), epgUid,
              WebUtils::URLEncodeInline(tags.GetTags()).c_str(), WebUtils::URLEncodeInline(m_settings.GetRecordingPath()).c_str());
  else
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&tags=%s",
              WebUtils::URLEncodeInline(serviceReference).c_str(), timer.iWeekdays, startTime, endTime,
              WebUtils::URLEncodeInline(title).c_str(), WebUtils::URLEncodeInline(description).c_str(), epgUid,
              WebUtils::URLEncodeInline(tags.GetTags()).c_str());

  Logger::Log(LEVEL_DEBUG, "%s - Command: %s", __FUNCTION__, strTmp.c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  Logger::Log(LEVEL_DEBUG, "%s - Updating timers", __FUNCTION__);

  TimerUpdates();

  if (alreadyStarted)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Timer started, triggering recording update", __FUNCTION__);
    PVR->TriggerRecordingUpdate();
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::AddAutoTimer(const PVR_TIMER& timer)
{
  Logger::Log(LEVEL_DEBUG, "%s - Start", __FUNCTION__);

  std::string strTmp = StringUtils::Format("autotimer/edit?");

  strTmp += StringUtils::Format("name=%s", WebUtils::URLEncodeInline(timer.strTitle).c_str());
  strTmp += StringUtils::Format("&match=%s", WebUtils::URLEncodeInline(timer.strEpgSearchString).c_str());

  if (timer.state != PVR_TIMER_STATE_DISABLED)
    strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
  else
    strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());

  if (!timer.bStartAnyTime)
  {
    time_t startTime = timer.startTime;
    std::tm timeinfo = *std::localtime(&startTime);
    strTmp += StringUtils::Format("&timespanFrom=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  if (!timer.bEndAnyTime)
  {
    time_t endTime = timer.endTime;
    std::tm timeinfo = *std::localtime(&endTime);
    strTmp += StringUtils::Format("&timespanTo=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  strTmp += StringUtils::Format("&encoding=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENCODING).c_str());
  strTmp += StringUtils::Format("&searchCase=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());
  if (timer.bFullTextEpgSearch)
    strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());
  else
    strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

  std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.iPreventDuplicateEpisodes);
  if (deDup == AutoTimer::DeDup::DISABLED)
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=0");
  }
  else
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=%s", AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE_OR_RECORDING.c_str());
    switch (deDup)
    {
      case AutoTimer::DeDup::CHECK_TITLE:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE.c_str());
        break;
      case AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC.c_str());
        break;
      case AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS.c_str());
        break;
    }
  }

  if (timer.iClientChannelUid != PVR_TIMER_ANY_CHANNEL)
  {
    const std::string serviceReference = m_channels.GetChannel(timer.iClientChannelUid)->GetServiceReference();

    //single channel
    strTmp += StringUtils::Format("&services=%s", WebUtils::URLEncodeInline(serviceReference).c_str());

    if (m_channels.GetChannel(timer.iClientChannelUid)->IsRadio())
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_RADIO.c_str())).c_str());
    else
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_TV.c_str())).c_str());

    std::string tagValue = serviceReference;
    std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
    strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_REFERENCE.c_str(), tagValue.c_str())).c_str());
  }
  else
  {
    const std::string serviceReference = m_epg.FindServiceReference(timer.strTitle, timer.iEpgUid, timer.startTime, timer.endTime);
    if (!serviceReference.empty())
    {
      const auto channel = m_channels.GetChannel(serviceReference);

      if (channel)
      {
        bool isRadio = channel->IsRadio();

        if (isRadio)
          strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_RADIO.c_str())).c_str());
        else
          strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_TV.c_str())).c_str());

        std::string tagValue = serviceReference;
        std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_REFERENCE.c_str(), tagValue.c_str())).c_str());

        if (!m_settings.UsesLastScannedChannelGroup())
          strTmp += BuildAddUpdateAutoTimerLimitGroupsParams(channel);
      }
    }

    strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s", TAG_FOR_ANY_CHANNEL.c_str())).c_str());
  }

  strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("GenreId=0x%02X", timer.iGenreType | timer.iGenreSubType)).c_str());

  strTmp += Timers::BuildAddUpdateAutoTimerIncludeParams(timer.iWeekdays);

  Logger::Log(LEVEL_DEBUG, "%s - Command: %s", __FUNCTION__, strTmp.c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Timer started, triggering recording update", __FUNCTION__);
    PVR->TriggerRecordingUpdate();
  }

  Logger::Log(LEVEL_DEBUG, "%s - Updating timers", __FUNCTION__);
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::UpdateTimer(const PVR_TIMER& timer)
{
  if (IsAutoTimer(timer))
    return UpdateAutoTimer(timer);

  Logger::Log(LEVEL_DEBUG, "%s timer channelid '%d'", __FUNCTION__, timer.iClientChannelUid);

  const std::string strServiceReference = m_channels.GetChannel(timer.iClientChannelUid)->GetServiceReference().c_str();

  const auto it = std::find_if(m_timers.cbegin(), m_timers.cend(), [&timer](const Timer& myTimer)
  {
    return myTimer.GetClientIndex() == timer.iClientIndex;
  });

  if (it != m_timers.cend())
  {
    Timer oldTimer = *it;

    Logger::Log(LEVEL_DEBUG, "%s old timer channelid '%d'", __FUNCTION__, oldTimer.GetChannelId());

    Tags tags(oldTimer.GetTags());
    tags.AddTag(TAG_FOR_CHANNEL_REFERENCE, strServiceReference, true);

    int iDisabled = 0;
    if (timer.state == PVR_TIMER_STATE_DISABLED)
      iDisabled = 1;

    unsigned int startPadding = timer.iMarginStart;
    unsigned int endPadding = timer.iMarginEnd;

    if (startPadding == 0 && endPadding == 0)
    {
      startPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingStartMargin();
      endPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingEndMargin();
    }

    bool alreadyStarted;
    time_t startTime, endTime;
    if ((timer.startTime - (startPadding * 60)) < std::time(nullptr))
    {
      alreadyStarted = true;
      startTime = std::time(nullptr);
      startPadding = 0;
    }
    else
    {
      startTime = timer.startTime - (startPadding * 60);
    }
    endTime = timer.endTime + (endPadding * 60);
    endPadding = endPadding;

    tags.AddTag(TAG_FOR_PADDING, StringUtils::Format("%u,%u", startPadding, endPadding));

    const std::string strTmp = StringUtils::Format("web/timerchange?sRef=%s&begin=%d&end=%d&name=%s&eventID=&description=%s&tags=%s&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%d&endOld=%d&deleteOldOnSave=1",
                                    WebUtils::URLEncodeInline(strServiceReference).c_str(), startTime, endTime,
                                    WebUtils::URLEncodeInline(timer.strTitle).c_str(), WebUtils::URLEncodeInline(timer.strSummary).c_str(),
                                    WebUtils::URLEncodeInline(tags.GetTags()).c_str(), iDisabled, timer.iWeekdays,
                                    WebUtils::URLEncodeInline(oldTimer.GetServiceReference()).c_str(), oldTimer.GetRealStartTime(),
                                    oldTimer.GetRealEndTime());

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    TimerUpdates();

    if (alreadyStarted)
      PVR->TriggerRecordingUpdate();

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

std::string Timers::RemovePaddingTag(std::string tag)
{
  static const std::regex regex(" Padding=[0-9]+,[0-9]+ *");
  std::string replaceWith = "";

  return std::regex_replace(tag, regex, replaceWith);
}

PVR_ERROR Timers::UpdateAutoTimer(const PVR_TIMER& timer)
{
  const auto it = std::find_if(m_autotimers.cbegin(), m_autotimers.cend(), [&timer](const AutoTimer& autoTimer)
  {
    return autoTimer.GetClientIndex() == timer.iClientIndex;
  });

  if (it != m_autotimers.cend())
  {
    AutoTimer timerToUpdate = *it;

    std::string strTmp = StringUtils::Format("autotimer/edit?id=%d", timerToUpdate.GetBackendId());

    strTmp += StringUtils::Format("&name=%s", WebUtils::URLEncodeInline(timer.strTitle).c_str());
    strTmp += StringUtils::Format("&match=%s", WebUtils::URLEncodeInline(timer.strEpgSearchString).c_str());

    if (timer.state != PVR_TIMER_STATE_DISABLED)
      strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
    else
      strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());

    if (!timer.bStartAnyTime)
    {
      time_t startTime = timer.startTime;
      std::tm timeinfo = *std::localtime(&startTime);
      strTmp += StringUtils::Format("&timespanFrom=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    if (!timer.bEndAnyTime)
    {
      time_t endTime = timer.endTime;
      std::tm timeinfo = *std::localtime(&endTime);
      strTmp += StringUtils::Format("&timespanTo=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    strTmp += StringUtils::Format("&encoding=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENCODING).c_str());

    if (timer.bFullTextEpgSearch)
      strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());
    else
      strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

    if (!timerToUpdate.GetSearchCase().empty())
      strTmp += StringUtils::Format("&searchCase=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());

    std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.iPreventDuplicateEpisodes);
    if (deDup == AutoTimer::DeDup::DISABLED)
    {
      strTmp += StringUtils::Format("&avoidDuplicateDescription=0");
    }
    else
    {
      strTmp += StringUtils::Format("&avoidDuplicateDescription=%s", AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE_OR_RECORDING.c_str());
      switch (deDup)
      {
        case AutoTimer::DeDup::CHECK_TITLE:
          strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE.c_str());
          break;
        case AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC:
          strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC.c_str());
          break;
        case AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS:
          strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS.c_str());
          break;
      }
    }

    if (timer.iClientChannelUid != PVR_TIMER_ANY_CHANNEL &&
        (timerToUpdate.GetAnyChannel() || (!timerToUpdate.GetAnyChannel() && timer.iClientChannelUid != timerToUpdate.GetChannelId())))
    {
      const std::string serviceReference = m_channels.GetChannel(timer.iClientChannelUid)->GetServiceReference();

      //move to single channel
      strTmp += StringUtils::Format("&services=%s", WebUtils::URLEncodeInline(serviceReference).c_str());

      //Update tags
      strTmp += StringUtils::Format("&tag=");

      if (m_channels.GetChannel(timer.iClientChannelUid)->IsRadio())
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_RADIO.c_str())).c_str());
      else
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_TV.c_str())).c_str());

      std::string tagValue = serviceReference;
      std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_REFERENCE.c_str(), tagValue.c_str())).c_str());

      if (timerToUpdate.ContainsTag(TAG_FOR_GENRE_ID))
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_GENRE_ID.c_str(), timerToUpdate.ReadTagValue(TAG_FOR_GENRE_ID).c_str())).c_str());

      //Clear bouquets
      strTmp += "&bouquets=";
    }
    else if (!timerToUpdate.GetAnyChannel() && timer.iClientChannelUid == PVR_TIMER_ANY_CHANNEL)
    {
      const auto& channel = m_channels.GetChannel(timerToUpdate.GetChannelId());

      //Move to any channel
      strTmp += StringUtils::Format("&services=");

      //Update tags
      strTmp += StringUtils::Format("&tag=");

      if (timerToUpdate.ContainsTag(TAG_FOR_CHANNEL_TYPE))
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), timerToUpdate.ReadTagValue(TAG_FOR_CHANNEL_TYPE).c_str())).c_str());

      std::string tagValue = channel->GetServiceReference();
      std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_REFERENCE.c_str(), tagValue.c_str())).c_str());

      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s", TAG_FOR_ANY_CHANNEL.c_str())).c_str());

      if (timerToUpdate.ContainsTag(TAG_FOR_GENRE_ID))
        strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_GENRE_ID.c_str(), timerToUpdate.ReadTagValue(TAG_FOR_GENRE_ID).c_str())).c_str());

      //Limit Channel Groups
      if (!m_settings.UsesLastScannedChannelGroup())
        strTmp += BuildAddUpdateAutoTimerLimitGroupsParams(channel);
    }

    strTmp += Timers::BuildAddUpdateAutoTimerIncludeParams(timer.iWeekdays);

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    if (timer.state == PVR_TIMER_STATE_RECORDING)
      PVR->TriggerRecordingUpdate();

    TimerUpdates();

    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_SERVER_ERROR;
}

std::string Timers::BuildAddUpdateAutoTimerLimitGroupsParams(const std::shared_ptr<Channel>& channel)
{
  std::string limitGroupParams;

  if (m_settings.GetLimitAnyChannelAutoTimers() && channel)
  {
    if (m_settings.GetLimitAnyChannelAutoTimersToChannelGroups())
    {
      for (const auto& group : channel->GetChannelGroupList())
        limitGroupParams += StringUtils::Format("%s,", group->GetServiceReference().c_str());
    }
    else
    {
      for (const auto& group : m_channelGroups.GetChannelGroupsList())
      {
        if (group->IsRadio() == channel->IsRadio())
          limitGroupParams += StringUtils::Format("%s,", group->GetServiceReference().c_str());
      }
    }
  }

  return StringUtils::Format("&bouquets=%s", WebUtils::URLEncodeInline(limitGroupParams).c_str());
}

std::string Timers::BuildAddUpdateAutoTimerIncludeParams(int weekdays)
{
  bool everyday = true;
  std::string includeParams;
  if (weekdays != PVR_WEEKDAY_NONE)
  {
      for (int i = 0; i < DAYS_IN_WEEK; i++)
      {
        if (1 & (weekdays >> i))
        {
          includeParams += StringUtils::Format("&dayofweek=%d", i);
        }
        else
        {
          everyday = false;
        }
      }

      if (everyday)
        includeParams = "&dayofweek=";
  }
  else
  {
    includeParams = "&dayofweek=";
  }

  return includeParams;
}

PVR_ERROR Timers::DeleteTimer(const PVR_TIMER& timer)
{
  if (IsAutoTimer(timer))
    return DeleteAutoTimer(timer);

  const auto it = std::find_if(m_timers.cbegin(), m_timers.cend(), [&timer](const Timer& myTimer)
  {
    return myTimer.GetClientIndex() == timer.iClientIndex;
  });

  if (it != m_timers.cend())
  {
    Timer timerToDelete = *it;

    const std::string strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%d&end=%d",
                                                   WebUtils::URLEncodeInline(timerToDelete.GetServiceReference()).c_str(),
                                                   timerToDelete.GetRealStartTime(), timerToDelete.GetRealEndTime());

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    if (timer.state == PVR_TIMER_STATE_RECORDING)
      PVR->TriggerRecordingUpdate();

    TimerUpdates();

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Timers::DeleteAutoTimer(const PVR_TIMER &timer)
{
  const auto it = std::find_if(m_autotimers.cbegin(), m_autotimers.cend(), [&timer](const AutoTimer& autoTimer)
  {
    return autoTimer.GetClientIndex() == timer.iClientIndex;
  });

  if (it != m_autotimers.cend())
  {
    AutoTimer timerToDelete = *it;

    //remove any child timers
    bool childTimerIsRecording = false;
    for (const auto& childTimer : m_timers)
    {
      if (childTimer.GetParentClientIndex() == timerToDelete.GetClientIndex())
      {
        const std::string strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%d&end=%d",
                                                       WebUtils::URLEncodeInline(childTimer.GetServiceReference()).c_str(),
                                                       childTimer.GetRealStartTime(), childTimer.GetRealEndTime());

        std::string strResult;
        WebUtils::SendSimpleCommand(strTmp, strResult, true);

        if (childTimer.GetState() == PVR_TIMER_STATE_RECORDING)
          childTimerIsRecording = true;
      }
    }

    const std::string strTmp = StringUtils::Format("autotimer/remove?id=%u", timerToDelete.GetBackendId());

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    if (timer.state == PVR_TIMER_STATE_RECORDING || childTimerIsRecording)
      PVR->TriggerRecordingUpdate();

    TimerUpdates();

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

void Timers::ClearTimers()
{
    m_timers.clear();
    m_autotimers.clear();
    m_timerChangeWatchers.clear();
}

void Timers::AddTimerChangeWatcher(std::atomic_bool* watcher)
{
  m_timerChangeWatchers.emplace_back(watcher);
}

bool Timers::TimerUpdates()
{
  bool regularTimersChanged = TimerUpdatesRegular();
  bool autoTimersChanged = false;

  if (Settings::GetInstance().SupportsAutoTimers() && m_settings.GetAutoTimersEnabled())
    autoTimersChanged = TimerUpdatesAuto();

  if (regularTimersChanged || autoTimersChanged)
  {
    Logger::Log(LEVEL_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();

    for (auto watcher : m_timerChangeWatchers)
        watcher->store(true);

    return true;
  }

  return false;
}

bool Timers::TimerUpdatesRegular()
{
  std::vector<Timer> newTimers;

  if (!LoadTimers(newTimers))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to load timers, skipping timer update", __FUNCTION__);
    return false;
  }

  for (auto& timer : m_timers)
  {
    timer.SetUpdateState(UPDATE_STATE_NONE);
  }

  //Update any timers
  unsigned int iUpdated = 0;
  unsigned int iUnchanged = 0;

  for (auto& newTimer : newTimers)
  {
    for (auto& existingTimer : m_timers)
    {
      if (existingTimer.Like(newTimer))
      {
        if (existingTimer == newTimer)
        {
          existingTimer.SetUpdateState(UPDATE_STATE_FOUND);
          newTimer.SetUpdateState(UPDATE_STATE_FOUND);
          iUnchanged++;
        }
        else
        {
          newTimer.SetUpdateState(UPDATE_STATE_UPDATED);
          existingTimer.SetUpdateState(UPDATE_STATE_UPDATED);

          existingTimer.UpdateFrom(newTimer);

          iUpdated++;
        }
      }
    }
  }

  //Remove any timers that are no longer valid
  unsigned int iRemoved = m_timers.size();

  m_timers.erase(
    std::remove_if(m_timers.begin(), m_timers.end(),
      [](const Timer& timer) { return timer.GetUpdateState() == UPDATE_STATE_NONE; }),
    m_timers.end());

  iRemoved -= m_timers.size();

  //Add any new autotimers
  unsigned int iNew = 0;

  for (auto& newTimer : newTimers)
  {
    if (newTimer.GetUpdateState() == UPDATE_STATE_NEW)
    {
      newTimer.SetClientIndex(m_clientIndexCounter);
      Logger::Log(LEVEL_DEBUG, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, newTimer.GetTitle().c_str(), m_clientIndexCounter);
      m_timers.emplace_back(newTimer);
      m_clientIndexCounter++;
      iNew++;
    }
  }

  //Link any Readonly timers to Repeating Timers
  for (const auto& repeatingTimer : m_timers)
  {
    for (auto& readonlyRepeatingOnceTimer : m_timers)
    {
      if ((repeatingTimer.GetType() == Timer::MANUAL_REPEATING || repeatingTimer.GetType() == Timer::EPG_REPEATING) &&
          readonlyRepeatingOnceTimer.GetType() == Timer::READONLY_REPEATING_ONCE &&
          readonlyRepeatingOnceTimer.IsChildOfParent(repeatingTimer))
      {
        readonlyRepeatingOnceTimer.SetParentClientIndex(repeatingTimer.GetClientIndex());
        continue;
      }
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew);

  std::vector<EpgEntry> timerBaseEntries(m_timers.begin(), m_timers.end());
  m_epg.UpdateTimerEPGFallbackEntries(timerBaseEntries);

  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

bool Timers::TimerUpdatesAuto()
{
  std::vector<AutoTimer> newAutotimers;

  if (!LoadAutoTimers(newAutotimers))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to load auto timers, skipping auto timer update", __FUNCTION__);
    return false;
  }

  for (auto& autoTimer : m_autotimers)
  {
    autoTimer.SetUpdateState(UPDATE_STATE_NONE);
  }

  //Update any autotimers
  unsigned int iUpdated = 0;
  unsigned int iUnchanged = 0;

  for (auto& newAutoTimer : newAutotimers)
  {
    for (auto& existingAutoTimer : m_autotimers)
    {
      if (existingAutoTimer.Like(newAutoTimer))
      {
        if (existingAutoTimer == newAutoTimer)
        {
          existingAutoTimer.SetUpdateState(UPDATE_STATE_FOUND);
          newAutoTimer.SetUpdateState(UPDATE_STATE_FOUND);
          iUnchanged++;
        }
        else
        {
          newAutoTimer.SetUpdateState(UPDATE_STATE_UPDATED);
          existingAutoTimer.SetUpdateState(UPDATE_STATE_UPDATED);

          existingAutoTimer.UpdateFrom(newAutoTimer);

          iUpdated++;
        }
      }
    }
  }

  //Remove any autotimers that are no longer valid
  unsigned int iRemoved = m_autotimers.size();

  m_autotimers.erase(
    std::remove_if(m_autotimers.begin(), m_autotimers.end(),
      [](const AutoTimer& autoTimer) { return autoTimer.GetUpdateState() == UPDATE_STATE_NONE; }),
    m_autotimers.end());

  iRemoved -= m_autotimers.size();

  //Add any new autotimers
  unsigned int iNew = 0;

  for (auto& newAutoTimer : newAutotimers)
  {
    if (newAutoTimer.GetUpdateState() == UPDATE_STATE_NEW)
    {
      newAutoTimer.SetClientIndex(m_clientIndexCounter);

      if ((newAutoTimer.GetChannelId()) == PVR_TIMER_ANY_CHANNEL)
        newAutoTimer.SetAnyChannel(true);
      Logger::Log(LEVEL_INFO, "%s New auto timer: '%s', ClientIndex: '%d'", __FUNCTION__, newAutoTimer.GetTitle().c_str(), m_clientIndexCounter);
      m_autotimers.emplace_back(newAutoTimer);
      m_clientIndexCounter++;
      iNew++;
    }
  }

  //Link Any child timers to autotimers
  for (const auto& autoTimer : m_autotimers)
  {
    for (auto& timer : m_timers)
    {
      const std::string autotimerTag = ConvertToAutoTimerTag(autoTimer.GetTitle());

      if (timer.GetType() == Timer::EPG_AUTO_ONCE && timer.ContainsTag(TAG_FOR_AUTOTIMER) && timer.ContainsTag(autotimerTag))
      {
        timer.SetParentClientIndex(autoTimer.GetClientIndex());
        continue;
      }
    }
  }

  Logger::Log(LEVEL_INFO, "%s No of autotimers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew);

  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

void Timers::RunAutoTimerListCleanup()
{
  const std::string strTmp = StringUtils::Format("web/timercleanup?cleanup=true");
  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    Logger::Log(LEVEL_ERROR, "%s - AutomaticTimerlistCleanup failed!", __FUNCTION__);
}
