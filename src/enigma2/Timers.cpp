/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Timers.h"

#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/UpdateState.h"
#include "utilities/WebUtils.h"
#include "utilities/XMLUtils.h"

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <regex>

#include <kodi/tools/StringUtils.h>
#include <kodi/General.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using namespace kodi::tools;

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
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2timerlist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2timerlist> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2timer").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2timer> element", __func__);
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

    Logger::Log(LEVEL_DEBUG, "%s fetched Timer entry '%s', begin '%lld', end '%lld', start padding mins '%u', end padding mins '%u'",
                __func__, newTimer.GetTitle().c_str(), static_cast<long long>(newTimer.GetStartTime()), static_cast<long long>(newTimer.GetEndTime()), newTimer.GetPaddingStartMins(), newTimer.GetPaddingEndMins());
  }

  Logger::Log(LEVEL_INFO, "%s fetched %u Timer Entries", __func__, timers.size());
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
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("autotimer").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <autotimer> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("timer").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <timer> element", __func__);
    return true; //No timers is valid
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("timer"))
  {
    AutoTimer newAutoTimer;

    if (!newAutoTimer.UpdateFrom(pNode, m_channels))
      continue;

    autoTimers.emplace_back(newAutoTimer);

    Logger::Log(LEVEL_DEBUG, "%s fetched AutoTimer entry '%s', begin '%lld', end '%lld'", __func__, newAutoTimer.GetTitle().c_str(), static_cast<long long>(newAutoTimer.GetStartTime()), static_cast<long long>(newAutoTimer.GetEndTime()));
  }

  Logger::Log(LEVEL_INFO, "%s fetched %u AutoTimer Entries", __func__, autoTimers.size());
  return true;
}

bool Timers::IsAutoTimer(const kodi::addon::PVRTimer& timer) const
{
  return timer.GetTimerType() == Timer::Type::EPG_AUTO_SEARCH;
}

void Timers::GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types) const
{
  struct TimerType : kodi::addon::PVRTimerType
  {
    TimerType(unsigned int id,
              unsigned int attributes,
              const std::string& description = std::string(),
              const std::vector<kodi::addon::PVRTypeIntValue>& groupValues = std::vector<kodi::addon::PVRTypeIntValue>(),
              const std::vector<kodi::addon::PVRTypeIntValue>& deDupValues = std::vector<kodi::addon::PVRTypeIntValue>(),
              int preventDuplicateEpisodesDefault = AutoTimer::DeDup::DISABLED)
    {
      SetId(id);
      SetAttributes(attributes);
      SetDescription(description);
      if (!groupValues.empty())
        SetRecordingGroups(groupValues, groupValues[0].GetValue());
      if (!deDupValues.empty())
        SetPreventDuplicateEpisodes(deDupValues, preventDuplicateEpisodesDefault);
    }
  };

  /* PVR_Timer.iRecordingGroup values and presentation.*/
  std::vector<kodi::addon::PVRTypeIntValue> groupValues = {
      {0, kodi::GetLocalizedString(30410)}, //automatic
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
    kodi::GetLocalizedString(30422), // Once off time/channel based
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
    kodi::GetLocalizedString(30421), // Once off timer (set by repeating time/channel based rule)
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
    kodi::GetLocalizedString(30423), // Repeating time/channel based
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
    kodi::GetLocalizedString(30424)); // One time guide-based
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
      kodi::GetLocalizedString(30425)); // Repeating guide-based
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
      kodi::GetLocalizedString(30425)); // Repeating guide-based
    types.emplace_back(*t);
    delete t;

    /* PVR_Timer.iPreventDuplicateEpisodes values and presentation.*/
    static std::vector<kodi::addon::PVRTypeIntValue> deDupValues =
    {
      { AutoTimer::DeDup::DISABLED,                   kodi::GetLocalizedString(30430) },
      { AutoTimer::DeDup::CHECK_TITLE,                kodi::GetLocalizedString(30431) },
      { AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC, kodi::GetLocalizedString(30432) },
      { AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS,  kodi::GetLocalizedString(30433) },
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
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN   |
      PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME      |
      PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME        |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS           |
      PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH    |
      PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH |
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP    |
      PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES |
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE,
      kodi::GetLocalizedString(30426), // Auto guide-based
      groupValues, deDupValues, AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS);
    types.emplace_back(*t);
    delete t;
  }

  // We always create this as even if autotimers are disabled the backend
  // can still send regular timers of this type

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
    kodi::GetLocalizedString(30420), // Once off timer (set by auto guide-based rule)
    groupValues);
  types.emplace_back(*t);
  delete t;
}

int Timers::GetTimerCount() const
{
  return m_timers.size();
}

int Timers::GetAutoTimerCount() const
{
  return m_autotimers.size();
}

void Timers::GetTimers(std::vector<kodi::addon::PVRTimer>& timers) const
{
  for (const auto& timer : m_timers)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __func__, timer.GetTitle().c_str(), timer.GetClientIndex());
    kodi::addon::PVRTimer tag;

    timer.UpdateTo(tag);

    timers.emplace_back(tag);
  }
}

void Timers::GetAutoTimers(std::vector<kodi::addon::PVRTimer>& timers) const
{
  for (const auto& autoTimer : m_autotimers)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __func__, autoTimer.GetTitle().c_str(),
                autoTimer.GetClientIndex());
    kodi::addon::PVRTimer tag;

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

PVR_ERROR Timers::AddTimer(const kodi::addon::PVRTimer& timer)
{
  if (IsAutoTimer(timer))
    return AddAutoTimer(timer);

  Logger::Log(LEVEL_DEBUG, "%s - Start", __func__);

  const std::string serviceReference = m_channels.GetChannel(timer.GetClientChannelUid())->GetServiceReference().c_str();
  Tags tags;

  if (timer.GetTimerType() == Timer::MANUAL_ONCE || timer.GetTimerType() == Timer::MANUAL_REPEATING)
    tags.AddTag(TAG_FOR_MANUAL_TIMER);
  else
    tags.AddTag(TAG_FOR_EPG_TIMER);

  if (m_channels.GetChannel(timer.GetClientChannelUid())->IsRadio())
    tags.AddTag(TAG_FOR_CHANNEL_TYPE, VALUE_FOR_CHANNEL_TYPE_RADIO);
  else
    tags.AddTag(TAG_FOR_CHANNEL_TYPE, VALUE_FOR_CHANNEL_TYPE_TV);

  tags.AddTag(TAG_FOR_CHANNEL_REFERENCE, serviceReference, true);

  unsigned int startPadding = timer.GetMarginStart();
  unsigned int endPadding = timer.GetMarginEnd();

  if (startPadding == 0 && endPadding == 0)
  {
    startPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingStartMargin();
    endPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingEndMargin();
  }

  bool alreadyStarted = false;
  time_t startTime, endTime;
  time_t now = std::time(nullptr);
  if ((timer.GetStartTime() - (startPadding * 60)) < now)
  {
    alreadyStarted = true;
    startTime = now;
    if (timer.GetStartTime() < now)
      startPadding = 0;
    else
      startPadding = (timer.GetStartTime() - now) / 60;
  }
  else
  {
    startTime = timer.GetStartTime() - (startPadding * 60);
  }
  endTime = timer.GetEndTime() + (endPadding * 60);

  if (endTime <= startTime)
    endTime = timer.GetStartTime() + (60 * 60 * 2) + (endPadding * 60); // default to 2 hours for invalid end time

  tags.AddTag(TAG_FOR_PADDING, StringUtils::Format("%u,%u", startPadding, endPadding));

  std::string title = timer.GetTitle();
  std::string description = timer.GetSummary();
  unsigned int epgUid = timer.GetEPGUid();
  bool foundEntry = false;

  if (Settings::GetInstance().IsOpenWebIf() && (timer.GetTimerType() == Timer::EPG_ONCE || timer.GetTimerType() == Timer::MANUAL_ONCE))
  {
    // We try to find the EPG Entry and use it's details
    EpgPartialEntry partialEntry = m_epg.LoadEPGEntryPartialDetails(serviceReference, timer.GetStartTime() < now ? now : timer.GetStartTime());

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
    tags.AddTag(TAG_FOR_GENRE_ID, StringUtils::Format("0x%02X", timer.GetGenreType() | timer.GetGenreSubType()));

  std::string strTmp;
  if (!m_settings.GetNewTimerRecordingPath().empty())
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%lld&end=%lld&name=%s&description=%s&eit=%d&tags=%s&dirname=&s",
              WebUtils::URLEncodeInline(serviceReference).c_str(), timer.GetWeekdays(), static_cast<long long>(startTime), static_cast<long long>(endTime),
              WebUtils::URLEncodeInline(title).c_str(), WebUtils::URLEncodeInline(description).c_str(), epgUid,
              WebUtils::URLEncodeInline(tags.GetTags()).c_str(), WebUtils::URLEncodeInline(m_settings.GetNewTimerRecordingPath()).c_str());
  else
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%lld&end=%lld&name=%s&description=%s&eit=%d&tags=%s",
              WebUtils::URLEncodeInline(serviceReference).c_str(), timer.GetWeekdays(), static_cast<long long>(startTime), static_cast<long long>(endTime),
              WebUtils::URLEncodeInline(title).c_str(), WebUtils::URLEncodeInline(description).c_str(), epgUid,
              WebUtils::URLEncodeInline(tags.GetTags()).c_str());

  Logger::Log(LEVEL_DEBUG, "%s - Command: %s", __func__, strTmp.c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  Logger::Log(LEVEL_DEBUG, "%s - Updating timers", __func__);

  TimerUpdates();

  if (alreadyStarted)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Timer started, triggering recording update", __func__);
    m_connectionListener.TriggerRecordingUpdate();
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::AddAutoTimer(const kodi::addon::PVRTimer& timer)
{
  Logger::Log(LEVEL_DEBUG, "%s - Start", __func__);

  std::string strTmp = StringUtils::Format("autotimer/edit?");

  strTmp += StringUtils::Format("name=%s", WebUtils::URLEncodeInline(timer.GetTitle()).c_str());
  strTmp += StringUtils::Format("&match=%s", WebUtils::URLEncodeInline(timer.GetEPGSearchString()).c_str());

  if (timer.GetState() != PVR_TIMER_STATE_DISABLED)
    strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
  else
    strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());

  if (!timer.GetStartAnyTime())
  {
    time_t startTime = timer.GetStartTime();
    std::tm timeinfo = *std::localtime(&startTime);
    strTmp += StringUtils::Format("&timespanFrom=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  if (!timer.GetEndAnyTime())
  {
    time_t endTime = timer.GetEndTime();
    std::tm timeinfo = *std::localtime(&endTime);
    strTmp += StringUtils::Format("&timespanTo=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  if (timer.GetMarginStart() != 0 || timer.GetMarginEnd() != 0)
  {
    if (timer.GetMarginStart() == timer.GetMarginEnd())
      strTmp += StringUtils::Format("&offset=%d", timer.GetMarginStart());
    else
      strTmp += StringUtils::Format("&offset=%d,%d", timer.GetMarginStart(), timer.GetMarginEnd());
  }

  strTmp += StringUtils::Format("&encoding=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENCODING).c_str());
  strTmp += StringUtils::Format("&searchCase=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());
  if (timer.GetFullTextEpgSearch())
    strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());
  else
    strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

  std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.GetPreventDuplicateEpisodes());
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

  if (timer.GetClientChannelUid() != PVR_TIMER_ANY_CHANNEL)
  {
    const std::string serviceReference = m_channels.GetChannel(timer.GetClientChannelUid())->GetServiceReference();

    //single channel
    strTmp += StringUtils::Format("&services=%s", WebUtils::URLEncodeInline(serviceReference).c_str());

    if (m_channels.GetChannel(timer.GetClientChannelUid())->IsRadio())
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_RADIO.c_str())).c_str());
    else
      strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_TYPE.c_str(), VALUE_FOR_CHANNEL_TYPE_TV.c_str())).c_str());

    std::string tagValue = serviceReference;
    std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
    strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("%s=%s", TAG_FOR_CHANNEL_REFERENCE.c_str(), tagValue.c_str())).c_str());
  }
  else
  {
    const std::string serviceReference = m_epg.FindServiceReference(timer.GetTitle(), timer.GetEPGUid(), timer.GetStartTime(), timer.GetEndTime());
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

  strTmp += StringUtils::Format("&tag=%s", WebUtils::URLEncodeInline(StringUtils::Format("GenreId=0x%02X", timer.GetGenreType() | timer.GetGenreSubType())).c_str());

  strTmp += Timers::BuildAddUpdateAutoTimerIncludeParams(timer.GetWeekdays());

  Logger::Log(LEVEL_DEBUG, "%s - Command: %s", __func__, strTmp.c_str());

  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  if (timer.GetState() == PVR_TIMER_STATE_RECORDING)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Timer started, triggering recording update", __func__);
    m_connectionListener.TriggerRecordingUpdate();
  }

  Logger::Log(LEVEL_DEBUG, "%s - Updating timers", __func__);

  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::UpdateTimer(const kodi::addon::PVRTimer& timer)
{
  if (IsAutoTimer(timer))
    return UpdateAutoTimer(timer);

  Logger::Log(LEVEL_DEBUG, "%s timer channelid '%d'", __func__, timer.GetClientChannelUid());

  const std::string strServiceReference = m_channels.GetChannel(timer.GetClientChannelUid())->GetServiceReference().c_str();

  const auto it = std::find_if(m_timers.cbegin(), m_timers.cend(), [&timer](const Timer& myTimer)
  {
    return myTimer.GetClientIndex() == timer.GetClientIndex();
  });

  if (it != m_timers.cend())
  {
    Timer oldTimer = *it;

    Logger::Log(LEVEL_DEBUG, "%s old timer channelid '%d'", __func__, oldTimer.GetChannelId());

    Tags tags(oldTimer.GetTags());
    tags.AddTag(TAG_FOR_CHANNEL_REFERENCE, strServiceReference, true);

    int iDisabled = 0;
    if (timer.GetState() == PVR_TIMER_STATE_DISABLED)
      iDisabled = 1;

    unsigned int startPadding = timer.GetMarginStart();
    unsigned int endPadding = timer.GetMarginEnd();

    if (startPadding == 0 && endPadding == 0)
    {
      startPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingStartMargin();
      endPadding = Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingEndMargin();
    }

    bool alreadyStarted = false;
    time_t startTime, endTime;
    time_t now = std::time(nullptr);
    if ((timer.GetStartTime() - (startPadding * 60)) < now)
    {
      alreadyStarted = true;
      startTime = now;
      if (timer.GetStartTime() < now)
        startPadding = 0;
      else
        startPadding = (timer.GetStartTime() - now) / 60;
    }
    else
    {
      startTime = timer.GetStartTime() - (startPadding * 60);
    }
    endTime = timer.GetEndTime() + (endPadding * 60);

    tags.AddTag(TAG_FOR_PADDING, StringUtils::Format("%u,%u", startPadding, endPadding));

    /* Note that plot (long desc) is automatically written to a timer entry by the backend
       therefore we only need to send outline as description to preserve both.
       Once a timer completes, long description will be cleared so if description
       is not populated we set it to the value of long description.
       Very important for providers that only use the plot field.
       */
    const std::string& description = !oldTimer.GetPlotOutline().empty() ? oldTimer.GetPlotOutline() : oldTimer.GetPlot();

    const std::string strTmp = StringUtils::Format("web/timerchange?sRef=%s&begin=%lld&end=%lld&name=%s&eventID=&description=%s&tags=%s&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%lld&endOld=%lld&deleteOldOnSave=1",
                                    WebUtils::URLEncodeInline(strServiceReference).c_str(), static_cast<long long>(startTime), static_cast<long long>(endTime),
                                    WebUtils::URLEncodeInline(timer.GetTitle()).c_str(), WebUtils::URLEncodeInline(description).c_str(),
                                    WebUtils::URLEncodeInline(tags.GetTags()).c_str(), iDisabled, timer.GetWeekdays(),
                                    WebUtils::URLEncodeInline(oldTimer.GetServiceReference()).c_str(),
                                    static_cast<long long>(oldTimer.GetRealStartTime()),
                                    static_cast<long long>(oldTimer.GetRealEndTime()));

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    TimerUpdates();

    if (alreadyStarted)
      m_connectionListener.TriggerRecordingUpdate();

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

PVR_ERROR Timers::UpdateAutoTimer(const kodi::addon::PVRTimer& timer)
{
  const auto it = std::find_if(m_autotimers.cbegin(), m_autotimers.cend(), [&timer](const AutoTimer& autoTimer)
  {
    return autoTimer.GetClientIndex() == timer.GetClientIndex();
  });

  if (it != m_autotimers.cend())
  {
    AutoTimer timerToUpdate = *it;

    std::string strTmp = StringUtils::Format("autotimer/edit?id=%d", timerToUpdate.GetBackendId());

    strTmp += StringUtils::Format("&name=%s", WebUtils::URLEncodeInline(timer.GetTitle()).c_str());
    strTmp += StringUtils::Format("&match=%s", WebUtils::URLEncodeInline(timer.GetEPGSearchString()).c_str());

    if (timer.GetState() != PVR_TIMER_STATE_DISABLED)
      strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
    else
      strTmp += StringUtils::Format("&enabled=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());

    if (!timer.GetStartAnyTime())
    {
      time_t startTime = timer.GetStartTime();
      std::tm timeinfo = *std::localtime(&startTime);
      strTmp += StringUtils::Format("&timespanFrom=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    if (!timer.GetEndAnyTime())
    {
      time_t endTime = timer.GetEndTime();
      std::tm timeinfo = *std::localtime(&endTime);
      strTmp += StringUtils::Format("&timespanTo=%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    if (timer.GetMarginStart() == 0 && timer.GetMarginEnd() == 0)
    {
      strTmp += "&offset=";
    }
    else
    {
      if (timer.GetMarginStart() == timer.GetMarginEnd())
        strTmp += StringUtils::Format("&offset=%d", timer.GetMarginStart());
      else
        strTmp += StringUtils::Format("&offset=%d,%d", timer.GetMarginStart(), timer.GetMarginEnd());
    }

    strTmp += StringUtils::Format("&encoding=%s", WebUtils::URLEncodeInline(AUTOTIMER_ENCODING).c_str());

    if (timer.GetFullTextEpgSearch())
      strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());
    else
      strTmp += StringUtils::Format("&searchType=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

    if (!timerToUpdate.GetSearchCase().empty())
      strTmp += StringUtils::Format("&searchCase=%s", WebUtils::URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());

    std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.GetPreventDuplicateEpisodes());
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

    if (timer.GetClientChannelUid() != PVR_TIMER_ANY_CHANNEL &&
        (timerToUpdate.GetAnyChannel() || (!timerToUpdate.GetAnyChannel() && timer.GetClientChannelUid() != timerToUpdate.GetChannelId())))
    {
      const std::string serviceReference = m_channels.GetChannel(timer.GetClientChannelUid())->GetServiceReference();

      //move to single channel
      strTmp += StringUtils::Format("&services=%s", WebUtils::URLEncodeInline(serviceReference).c_str());

      //Update tags
      strTmp += StringUtils::Format("&tag=");

      if (m_channels.GetChannel(timer.GetClientChannelUid())->IsRadio())
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
    else if (!timerToUpdate.GetAnyChannel() && timer.GetClientChannelUid() == PVR_TIMER_ANY_CHANNEL)
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

    strTmp += Timers::BuildAddUpdateAutoTimerIncludeParams(timer.GetWeekdays());

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    if (timer.GetState() == PVR_TIMER_STATE_RECORDING)
      m_connectionListener.TriggerRecordingUpdate();

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

PVR_ERROR Timers::DeleteTimer(const kodi::addon::PVRTimer& timer)
{
  if (IsAutoTimer(timer))
    return DeleteAutoTimer(timer);

  const auto it = std::find_if(m_timers.cbegin(), m_timers.cend(), [&timer](const Timer& myTimer)
  {
    return myTimer.GetClientIndex() == timer.GetClientIndex();
  });

  if (it != m_timers.cend())
  {
    Timer timerToDelete = *it;

    const std::string strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%lld&end=%lld",
                                                   WebUtils::URLEncodeInline(timerToDelete.GetServiceReference()).c_str(),
                                                   static_cast<long long>(timerToDelete.GetRealStartTime()),
                                                   static_cast<long long>(timerToDelete.GetRealEndTime()));

    std::string strResult;
    if (!WebUtils::SendSimpleCommand(strTmp, strResult))
      return PVR_ERROR_SERVER_ERROR;

    if (timer.GetState() == PVR_TIMER_STATE_RECORDING)
      m_connectionListener.TriggerRecordingUpdate();

    TimerUpdates();

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR Timers::DeleteAutoTimer(const kodi::addon::PVRTimer &timer)
{
  const auto it = std::find_if(m_autotimers.cbegin(), m_autotimers.cend(), [&timer](const AutoTimer& autoTimer)
  {
    return autoTimer.GetClientIndex() == timer.GetClientIndex();
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
        const std::string strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%lld&end=%lld",
                                                       WebUtils::URLEncodeInline(childTimer.GetServiceReference()).c_str(),
                                                       static_cast<long long>(childTimer.GetRealStartTime()),
                                                       static_cast<long long>(childTimer.GetRealEndTime()));

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

    if (timer.GetState() == PVR_TIMER_STATE_RECORDING || childTimerIsRecording)
      m_connectionListener.TriggerRecordingUpdate();

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
    Logger::Log(LEVEL_DEBUG, "%s Changes in timerlist detected, trigger an update!", __func__);
    m_connectionListener.TriggerTimerUpdate();

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
    Logger::Log(LEVEL_ERROR, "%s Unable to load timers, skipping timer update", __func__);
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
      Logger::Log(LEVEL_DEBUG, "%s New timer: '%s', ClientIndex: '%d'", __func__, newTimer.GetTitle().c_str(), m_clientIndexCounter);
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

  Logger::Log(LEVEL_DEBUG, "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __func__, iRemoved, iUnchanged, iUpdated, iNew);

  std::vector<EpgEntry> timerBaseEntries;
  for (auto& timer : m_timers)
  {
    EpgEntry entry = timer;
    entry.SetStartTime(timer.GetRealStartTime());
    entry.SetEndTime(timer.GetRealEndTime());
    timerBaseEntries.emplace_back(entry);
  }
  m_epg.UpdateTimerEPGFallbackEntries(timerBaseEntries);

  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

bool Timers::TimerUpdatesAuto()
{
  std::vector<AutoTimer> newAutotimers;

  if (!LoadAutoTimers(newAutotimers))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to load auto timers, skipping auto timer update", __func__);
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
      Logger::Log(LEVEL_DEBUG, "%s New auto timer: '%s', ClientIndex: '%d'", __func__, newAutoTimer.GetTitle().c_str(), m_clientIndexCounter);
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

  Logger::Log(LEVEL_DEBUG, "%s No of autotimers: removed [%d], untouched [%d], updated '%d', new '%d'", __func__, iRemoved, iUnchanged, iUpdated, iNew);

  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

void Timers::RunAutoTimerListCleanup()
{
  const std::string strTmp = StringUtils::Format("web/timercleanup?cleanup=true");
  std::string strResult;
  if (!WebUtils::SendSimpleCommand(strTmp, strResult))
    Logger::Log(LEVEL_ERROR, "%s - AutomaticTimerlistCleanup failed!", __func__);
}
