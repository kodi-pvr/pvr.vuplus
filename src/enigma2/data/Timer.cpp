#include "Timer.h"


#include "inttypes.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2::data;

bool Timer::Like(const Timer &right) const
{
  bool bChanged = true;
  bChanged = bChanged && (m_startTime == right.m_startTime); 
  bChanged = bChanged && (m_endTime == right.m_endTime); 
  bChanged = bChanged && (m_channelId == right.m_channelId); 
  bChanged = bChanged && (m_weekdays == right.m_weekdays); 
  bChanged = bChanged && (m_epgId == right.m_epgId); 

  return bChanged;
}

bool Timer::operator==(const Timer &right) const
{
  bool bChanged = true;
  bChanged = bChanged && (m_startTime == right.m_startTime); 
  bChanged = bChanged && (m_endTime == right.m_endTime); 
  bChanged = bChanged && (m_channelId == right.m_channelId); 
  bChanged = bChanged && (m_weekdays == right.m_weekdays); 
  bChanged = bChanged && (m_epgId == right.m_epgId); 
  bChanged = bChanged && (m_state == right.m_state); 
  bChanged = bChanged && (m_title == right.m_title);
  bChanged = bChanged && (m_plot == right.m_plot);

  return bChanged;
}

void Timer::UpdateFrom(const Timer &right)
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
}

void Timer::UpdateTo(PVR_TIMER &left) const
{
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle));
  strncpy(left.strDirectory, "/", sizeof(left.strDirectory));   // unused
  strncpy(left.strSummary, m_plot.c_str(), sizeof(left.strSummary));
  left.iTimerType         = m_type;
  left.iClientChannelUid   = m_channelId;
  left.startTime           = m_startTime;
  left.endTime             = m_endTime;
  left.state               = m_state;
  left.iPriority           = 0;     // unused
  left.iLifetime           = 0;     // unused
  left.firstDay            = 0;     // unused
  left.iWeekdays           = m_weekdays;
  left.iEpgUid             = m_epgId;
  left.iMarginStart        = 0;     // unused
  left.iMarginEnd          = 0;     // unused
  left.iGenreType          = 0;     // unused
  left.iGenreSubType       = 0;     // unused
  left.iClientIndex        = m_clientIndex;
  left.iParentClientIndex  = m_parentClientIndex;
}

bool Timer::isScheduled() const
{
  return m_state == PVR_TIMER_STATE_SCHEDULED
      || m_state == PVR_TIMER_STATE_RECORDING;
}

bool Timer::isRunning(std::time_t *now, std::string *channelName) const
{
  if (!isScheduled())
    return false;
  if (now && !(m_startTime <= *now && *now <= m_endTime))
    return false;
  if (channelName && m_channelName != *channelName)
    return false;
  return true;
}

bool Timer::isChildOfParent(const Timer &parent) const
{
  time_t time;
  std::tm timeinfo;
  int weekday = 0;

  time = m_startTime;
  timeinfo = *std::localtime(&time);
  std::string childStartTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  int tmDayOfWeek = timeinfo.tm_wday - 1;
  if (tmDayOfWeek < 0)
    tmDayOfWeek = 6;
  weekday = (1 << tmDayOfWeek);

  time = m_endTime;
  timeinfo = *std::localtime(&time);
  std::string childEndTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  time = parent.m_startTime;
  timeinfo = *std::localtime(&time);
  std::string parentStartTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  time = parent.m_endTime;
  timeinfo = *std::localtime(&time);
  std::string parentEndTime = StringUtils::Format("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  bool isChild = true;

  isChild = isChild && (m_title == parent.m_title);  
  isChild = isChild && (childStartTime == parentStartTime); 
  isChild = isChild && (childEndTime == parentEndTime); 
  isChild = isChild && (m_channelId == parent.m_channelId); 
  isChild = isChild && (weekday & parent.m_weekdays); 

  return isChild; 
}