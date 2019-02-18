#include "Timer.h"

#include <regex>

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

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
  bChanged = bChanged && (m_paddingStartMins == right.m_paddingStartMins);
  bChanged = bChanged && (m_paddingEndMins == right.m_paddingEndMins);
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
  m_state = right.m_state;
  m_paddingStartMins = right.m_paddingStartMins;
  m_paddingEndMins = right.m_paddingEndMins;
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
  left.iMarginStart        = m_paddingStartMins;
  left.iMarginEnd          = m_paddingEndMins;
  left.iGenreType          = 0;     // unused
  left.iGenreSubType       = 0;     // unused
  left.iClientIndex        = m_clientIndex;
  left.iParentClientIndex  = m_parentClientIndex;
}

bool Timer::IsScheduled() const
{
  return m_state == PVR_TIMER_STATE_SCHEDULED
      || m_state == PVR_TIMER_STATE_RECORDING;
}

bool Timer::IsRunning(std::time_t *now, std::string *channelName) const
{
  if (!IsScheduled())
    return false;
  if (now && !(GetRealStartTime() <= *now && *now <= GetRealEndTime()))
    return false;
  if (channelName && m_channelName != *channelName)
    return false;
  return true;
}

bool Timer::IsChildOfParent(const Timer &parent) const
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

bool Timer::UpdateFrom(TiXmlElement* timerNode, Channels &channels)
{
  std::string strTmp;

  int iTmp;
  bool bTmp;
  int iDisabled;

  if (XMLUtils::GetString(timerNode, "e2name", strTmp))
    Logger::Log(LEVEL_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());

  if (!XMLUtils::GetInt(timerNode, "e2state", iTmp))
    return false;

  if (!XMLUtils::GetInt(timerNode, "e2disabled", iDisabled))
    return false;

  m_title = strTmp;

  if (XMLUtils::GetString(timerNode, "e2servicereference", strTmp))
    m_channelId = channels.GetChannelUniqueId(strTmp.c_str());

  // Skip timers for channels we don't know about, such as when the addon only uses one bouquet or an old channel referene that doesn't exist
  if (m_channelId < 0)
  {
    Logger::Log(LEVEL_DEBUG, "%s could not find channel so skipping timer: '%s'", __FUNCTION__, m_title.c_str());
    return false;
  }

  m_channelName = channels.GetChannel(m_channelId)->GetChannelName();

  if (!XMLUtils::GetInt(timerNode, "e2timebegin", iTmp))
    return false;

  m_startTime = iTmp;

  if (!XMLUtils::GetInt(timerNode, "e2timeend", iTmp))
    return false;

  m_endTime = iTmp;

  if (XMLUtils::GetString(timerNode, "e2description", strTmp))
    m_plot = strTmp;

  if (XMLUtils::GetInt(timerNode, "e2repeated", iTmp))
    m_weekdays = iTmp;
  else
    m_weekdays = 0;

  if (XMLUtils::GetInt(timerNode, "e2eit", iTmp))
    m_epgId = iTmp;
  else
    m_epgId = 0;

  m_state = PVR_TIMER_STATE_NEW;

  if (!XMLUtils::GetInt(timerNode, "e2state", iTmp))
    return false;

  Logger::Log(LEVEL_DEBUG, "%s e2state is: %d ", __FUNCTION__, iTmp);

  if (iTmp == 0)
  {
    m_state = PVR_TIMER_STATE_SCHEDULED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: SCHEDULED", __FUNCTION__);
  }

  if (iTmp == 2)
  {
    m_state = PVR_TIMER_STATE_RECORDING;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: RECORDING", __FUNCTION__);
  }

  if (iTmp == 3 && iDisabled == 0)
  {
    m_state = PVR_TIMER_STATE_COMPLETED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: COMPLETED", __FUNCTION__);
  }

  if (XMLUtils::GetBoolean(timerNode, "e2cancled", bTmp))
  {
    if (bTmp)
    {
      m_state = PVR_TIMER_STATE_ABORTED;
      Logger::Log(LEVEL_DEBUG, "%s Timer state is: ABORTED", __FUNCTION__);
    }
  }

  if (iDisabled == 1)
  {
    m_state = PVR_TIMER_STATE_CANCELLED;
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: Cancelled", __FUNCTION__);
  }

  if (m_state == PVR_TIMER_STATE_NEW)
    Logger::Log(LEVEL_DEBUG, "%s Timer state is: NEW", __FUNCTION__);

  m_tags = "";
  if (XMLUtils::GetString(timerNode, "e2tags", strTmp))
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
            Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingStartMargin(),
            Settings::GetInstance().GetDeviceSettings()->GetGlobalRecordingEndMargin()));
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
    if (std::sscanf(ReadTag(TAG_FOR_PADDING).c_str(), "Padding=%u,%u", &m_paddingStartMins, &m_paddingEndMins) != 2)
    {
      m_paddingStartMins = 0;
      m_paddingEndMins = 0;
    }
  }

  if (m_paddingStartMins > 0)
    m_startTime += m_paddingStartMins * 60;

  if (m_paddingEndMins > 0)
    m_endTime -= m_paddingEndMins * 60;

  return true;
}

bool Timer::ContainsTag(const std::string &tag) const
{
  std::regex regex("^.* ?" + tag + " ?.*$");

  return (regex_match(m_tags, regex));
}

std::string Timer::ReadTag(const std::string &tagName) const
{
  std::string tag;

  size_t found = m_tags.find(tagName);
  if (found != std::string::npos)
  {
    tag = m_tags.substr(found, m_tags.size());

    found = tag.find(" ");
    if (found != std::string::npos)
      tag = tag.substr(0, found);

    tag = StringUtils::Trim(tag);
  }

  return tag;
}