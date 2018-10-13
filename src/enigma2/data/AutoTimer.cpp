#include "AutoTimer.h"

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2::data;
using namespace enigma2::utilities;

bool AutoTimer::Like(const AutoTimer &right) const
{
  return m_backendId == right.m_backendId;;
}

bool AutoTimer::operator==(const AutoTimer &right) const
{
  bool bChanged = true;
  bChanged = bChanged && (!m_title.compare(right.m_title));
  bChanged = bChanged && (m_startTime == right.m_startTime); 
  bChanged = bChanged && (m_endTime == right.m_endTime); 
  bChanged = bChanged && (m_channelId == right.m_channelId); 
  bChanged = bChanged && (m_weekdays == right.m_weekdays); 

  bChanged = bChanged && (m_searchPhrase == right.m_searchPhrase); 
  bChanged = bChanged && (m_searchType == right.m_searchType); 
  bChanged = bChanged && (m_searchCase == right.m_searchCase); 
  bChanged = bChanged && (m_state == right.m_state); 
  bChanged = bChanged && (m_searchFulltext == right.m_searchFulltext); 
  bChanged = bChanged && (m_startAnyTime == right.m_startAnyTime); 
  bChanged = bChanged && (m_endAnyTime == right.m_endAnyTime); 
  bChanged = bChanged && (m_anyChannel == right.m_anyChannel); 
  bChanged = bChanged && (m_deDup == right.m_deDup); 

  return bChanged;
}

void AutoTimer::UpdateFrom(const AutoTimer &right)
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
}

void AutoTimer::UpdateTo(PVR_TIMER &left) const
{
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle));
  //strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
  //strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
  strncpy(left.strEpgSearchString, m_searchPhrase.c_str(), sizeof(left.strEpgSearchString));
  left.iTimerType          = m_type;
  if (m_anyChannel)
    left.iClientChannelUid = PVR_TIMER_ANY_CHANNEL;
  else
    left.iClientChannelUid = m_channelId;
  left.startTime           = m_startTime;
  left.endTime             = m_endTime;
  left.state               = m_state;
  left.iPriority           = 0;     // unused
  left.iLifetime           = 0;     // unused
  left.firstDay            = 0;     // unused
  left.iWeekdays           = m_weekdays;
  //right.iEpgUid             = timer.iEpgID;
  left.iMarginStart        = 0;     // unused
  left.iMarginEnd          = 0;     // unused
  left.iGenreType          = 0;     // unused
  left.iGenreSubType       = 0;     // unused
  left.iClientIndex        = m_clientIndex;
  left.bStartAnyTime       = m_startAnyTime;
  left.bEndAnyTime         = m_endAnyTime;
  left.bFullTextEpgSearch  = m_searchFulltext;
  left.iPreventDuplicateEpisodes = m_deDup;
}

bool AutoTimer::UpdateFrom(TiXmlElement* autoTimerNode, Channels &channels)
{
  std::string strTmp;
  int iTmp;

  m_type = Timer::EPG_AUTO_SEARCH;

  //this is an auto timer so the state is always scheduled unless it's disabled
  m_state = PVR_TIMER_STATE_SCHEDULED;

  if (autoTimerNode->QueryStringAttribute("name", &strTmp) == TIXML_SUCCESS)
    m_title = strTmp;

  if (autoTimerNode->QueryStringAttribute("match", &strTmp) == TIXML_SUCCESS)
    m_searchPhrase = strTmp;
  
  if (autoTimerNode->QueryStringAttribute("enabled", &strTmp) == TIXML_SUCCESS)
  {
    if (strTmp == AUTOTIMER_ENABLED_NO)
    {
      m_state = PVR_TIMER_STATE_CANCELLED;
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
    else if (searchForDuplicateDescription.empty()) //Even though this value should be 2 it is sent as ommitted for this attribute
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
    const TiXmlElement *nextServiceNode = serviceNode->NextSiblingElement("e2service");

    if (!nextServiceNode)
    {
      //If we only have one channel
      if (XMLUtils::GetString(serviceNode, "e2servicereference", strTmp))
      {
        m_channelId = channels.GetChannelUniqueId(strTmp.c_str());

        // Skip autotimers for channels we don't know about, such as when the addon only uses one bouquet or an old channel referene that doesn't exist
        if (m_channelId < 0)
        {
          Logger::Log(LEVEL_DEBUG, "%s could not find channel so skipping autotimer: '%s'", __FUNCTION__, m_title.c_str());
          return false;
        }

        m_channelName = channels.GetChannel(m_channelId).GetChannelName();  
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
          m_weekdays = m_weekdays |= (1 << atoi(includeVal.c_str()));
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

  return true;
}

void AutoTimer::ParseTime(const std::string &time, std::tm &timeinfo) const
{
  std::sscanf(time.c_str(), "%02d:%02d", &timeinfo.tm_hour,
      &timeinfo.tm_min);
}