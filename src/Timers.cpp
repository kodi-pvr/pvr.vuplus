#include "Timers.h"
#include "client.h"
#include "VuData.h"
#include "LocalizedString.h"

#include <algorithm>
#include <ctime>
#include <regex>

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace vuplus;
using namespace ADDON;

bool Timer::isScheduled() const
{
  return state == PVR_TIMER_STATE_SCHEDULED
      || state == PVR_TIMER_STATE_RECORDING;
}

bool Timer::isRunning(std::time_t *now, std::string *channelName) const
{
  if (!isScheduled())
    return false;
  if (now && !(startTime <= *now && *now <= endTime))
    return false;
  if (channelName && strChannelName != *channelName)
    return false;
  return true;
}

template <typename T>
T *Timers::GetTimer(std::function<bool (const T&)> func,
  std::vector<T> &timerlist)
{
  for (auto &timer : timerlist)
  {
    if (func(timer))
      return &timer;
  }
  return nullptr;
}

std::vector<Timer> Timers::LoadTimers()
{
  std::vector<Timer> timers;

  std::string url; 
  url = StringUtils::Format("%s%s", vuData.getConnectionURL().c_str(), "web/timerlist"); 

  std::string strXML;
  strXML = vuData.GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return timers;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2timerlist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2timerlist> element!", __FUNCTION__);
    return timers;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2timer").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2timer> element");
    return timers;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2timer"))
  {
    std::string strTmp;

    int iTmp;
    bool bTmp;
    int iDisabled;
    
    if (XMLUtils::GetString(pNode, "e2name", strTmp)) 
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());
 
    if (!XMLUtils::GetInt(pNode, "e2state", iTmp)) 
      continue;

    if (!XMLUtils::GetInt(pNode, "e2disabled", iDisabled))
      continue;

    Timer timer;
    
    timer.strTitle          = strTmp;

    if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      timer.iChannelId = vuData.GetChannelNumber(strTmp.c_str());

    timer.strChannelName = vuData.GetChannels().at(timer.iChannelId-1).strChannelName;  

    if (!XMLUtils::GetInt(pNode, "e2timebegin", iTmp)) 
      continue; 
 
    timer.startTime         = iTmp;
    
    if (!XMLUtils::GetInt(pNode, "e2timeend", iTmp)) 
      continue; 
 
    timer.endTime           = iTmp;
    
    if (XMLUtils::GetString(pNode, "e2description", strTmp))
      timer.strPlot        = strTmp.c_str();
 
    if (XMLUtils::GetInt(pNode, "e2repeated", iTmp))
      timer.iWeekdays         = iTmp;
    else 
      timer.iWeekdays = 0;

    if (XMLUtils::GetInt(pNode, "e2eit", iTmp))
      timer.iEpgID = iTmp;
    else 
      timer.iEpgID = 0;

    timer.state = PVR_TIMER_STATE_NEW;

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
      continue;

    XBMC->Log(LOG_DEBUG, "%s e2state is: %d ", __FUNCTION__, iTmp);
  
    if (iTmp == 0) 
    {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: SCHEDULED", __FUNCTION__);
    }
    
    if (iTmp == 2) 
    {
      timer.state = PVR_TIMER_STATE_RECORDING;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: RECORDING", __FUNCTION__);
    }
    
    if (iTmp == 3 && iDisabled == 0) 
    {
      timer.state = PVR_TIMER_STATE_COMPLETED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: COMPLETED", __FUNCTION__);
    }

    if (XMLUtils::GetBoolean(pNode, "e2cancled", bTmp)) 
    {
      if (bTmp)  
      {
        timer.state = PVR_TIMER_STATE_ABORTED;
        XBMC->Log(LOG_DEBUG, "%s Timer state is: ABORTED", __FUNCTION__);
      }
    }

    if (iDisabled == 1) 
    {
      timer.state = PVR_TIMER_STATE_CANCELLED;
      XBMC->Log(LOG_DEBUG, "%s Timer state is: Cancelled", __FUNCTION__);
    }

    if (timer.state == PVR_TIMER_STATE_NEW)
      XBMC->Log(LOG_DEBUG, "%s Timer state is: NEW", __FUNCTION__);

    timer.tags = "";
    if (XMLUtils::GetString(pNode, "e2tags", strTmp))
      timer.tags        = strTmp.c_str();

    if (Timers::FindTagInTimerTags("Manual", timer.tags))
    {
      //We create a Manual tag on Manual timers created from Kodi, this allows us to set the Timer Type correctly
      if (timer.iWeekdays != PVR_WEEKDAY_NONE)
      {
        timer.type = Timer::MANUAL_REPEATING;
      }
      else
      {
        timer.type =  Timer::MANUAL_ONCE;
      }
    }
    else
    { //Default to EPG for all other standard timers
      if (timer.iWeekdays != PVR_WEEKDAY_NONE)
      {
        timer.type = Timer::EPG_REPEATING;
      }
      else
      {
        if (Timers::FindTagInTimerTags("AutoTimer", timer.tags))
        {
          timer.type =  Timer::EPG_AUTO_ONCE;
        }
        else
        {
          timer.type =  Timer::EPG_ONCE;
        }
      }
    }

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return timers; 
}

bool Timers::FindTagInTimerTags(std::string tag, std::string tags)
{
    std::regex regex ("^.* ?" + tag + " ?.*$");

    return (regex_match(tags, regex));
}

std::string Timers::ConvertToAutoTimerTag(std::string tag)
{
    std::regex regex (" ");
    std::string replaceWith = "_";

    return regex_replace(tag, regex, replaceWith);
}

std::vector<AutoTimer> Timers::LoadAutoTimers()
{
  std::vector<AutoTimer> autoTimers;

  std::string url; 
  url = StringUtils::Format("%s%s", vuData.getConnectionURL().c_str(), "autotimer"); 

  std::string strXML;
  strXML = vuData.GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return autoTimers;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("autotimer").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <autotimer> element!", __FUNCTION__);
    return autoTimers;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("timer").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <timer> element");
    return autoTimers;
  }

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("timer"))
  {

    std::string strTmp;

    int iTmp;
    bool bTmp;
    int iDisabled;

    std::string name;
    std::string match;
    std::string enabled;
    int id;
    std::string from;
    std::string to;
    std::string encoding;
    std::string searchType;
    std::string searchCase;
    std::string avoidDuplicateDescription;
    std::string searchForDuplicateDescription;

    AutoTimer autoTimer;
    autoTimer.type = Timer::EPG_AUTO_SEARCH;

    //this is an auto timer so the state is always scheduled unless it's disabled
    autoTimer.state = PVR_TIMER_STATE_SCHEDULED;

    if (pNode->QueryStringAttribute("name", &name) == TIXML_SUCCESS)
      autoTimer.strTitle = name;

    if (pNode->QueryStringAttribute("match", &match) == TIXML_SUCCESS)
      autoTimer.searchPhrase = match;
    
    if (pNode->QueryStringAttribute("enabled", &enabled) == TIXML_SUCCESS)
    {
      if (enabled == AUTOTIMER_ENABLED_NO)
      {
        autoTimer.state = PVR_TIMER_STATE_CANCELLED;
      }
    }

    if (pNode->QueryIntAttribute("id", &id) == TIXML_SUCCESS)
      autoTimer.backendId = id;

    pNode->QueryStringAttribute("from", &from);
    pNode->QueryStringAttribute("to", &to);
    pNode->QueryStringAttribute("avoidDuplicateDescription", &avoidDuplicateDescription);
    pNode->QueryStringAttribute("searchForDuplicateDescription", &searchForDuplicateDescription);

    if (avoidDuplicateDescription != AUTOTIMER_AVOID_DUPLICATE_DISABLED)
    {
      if (searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE)
        autoTimer.deDup = AutoTimer::DeDup::CHECK_TITLE;
      else if (searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC)
        autoTimer.deDup = AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC;
      else if (searchForDuplicateDescription == AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS)
        autoTimer.deDup = AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS;
    }

    if (pNode->QueryStringAttribute("encoding", &encoding) == TIXML_SUCCESS)
      autoTimer.encoding = encoding;    

    if (pNode->QueryStringAttribute("searchType", &searchType) == TIXML_SUCCESS)
    {
      autoTimer.searchType = searchType;
      if (searchType == AUTOTIMER_SEARCH_TYPE_DESCRIPTION)
        autoTimer.searchFulltext = true;
    }

    if (pNode->QueryStringAttribute("searchCase", &searchCase) == TIXML_SUCCESS)
      autoTimer.searchCase = searchCase;

    TiXmlElement* serviceNode = pNode->FirstChildElement("e2service");

    if (serviceNode)
    {
      const TiXmlElement *nextServiceNode = serviceNode->NextSiblingElement("e2service");

      if (!nextServiceNode)
      {
        //If we only have one channel
        if (XMLUtils::GetString(serviceNode, "e2servicereference", strTmp))
        {
          autoTimer.iChannelId = vuData.GetChannelNumber(strTmp.c_str());
          autoTimer.strChannelName = vuData.GetChannels().at(autoTimer.iChannelId-1).strChannelName;  
        }
      }
      else //otherwise set to any channel
      {
        autoTimer.iChannelId = PVR_TIMER_ANY_CHANNEL;
        autoTimer.anyChannel = true; 
      }
    } 

    autoTimer.iWeekdays = 0;
    
    TiXmlElement* includeNode = pNode->FirstChildElement("include");

    if (includeNode)
    {
      for (; includeNode != NULL; includeNode = includeNode->NextSiblingElement("include"))
      {
        std::string includeVal = includeNode->GetText();

        std::string where;
        if (includeNode->QueryStringAttribute("where", &where) == TIXML_SUCCESS)
        {
          if (where == "dayofweek")
          {
            autoTimer.iWeekdays |= (1 << atoi(includeVal.c_str()));
          }
        }
      }
    }

    if (autoTimer.iWeekdays != 0)
    {
      std::time_t t = std::time(nullptr);
      std::tm timeinfo = *std::localtime(&t);
      timeinfo.tm_sec = 0;
      autoTimer.startTime = 0;
      if (from != "")
      {
        ParseTime(from, timeinfo);
        autoTimer.startTime = std::mktime(&timeinfo);
      }

      timeinfo = *std::localtime(&t);
      timeinfo.tm_sec = 0;
      autoTimer.endTime = 0;
      if (to != "")
      {
        ParseTime(to, timeinfo);
        autoTimer.endTime = std::mktime(&timeinfo);
      }
    }
    else
    {
      for (int i = 0; i < 7; i++)
      {
        autoTimer.iWeekdays |= (1 << i);
      }
      autoTimer.startAnyTime = true;
      autoTimer.endAnyTime = true;
    }

    autoTimers.push_back(autoTimer);

    XBMC->Log(LOG_INFO, "%s fetched AutoTimer entry '%s', begin '%d', end '%d'", __FUNCTION__, autoTimer.strTitle.c_str(), autoTimer.startTime, autoTimer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u AutoTimer Entries", __FUNCTION__, autoTimers.size());
  return autoTimers; 
}

bool Timers::CanAutoTimers() const
{
  return vuData.GetWebIfVersion() >= WEBIF_VERSION_NUM(1, 2, 4);
}

bool Timers::IsAutoTimer(const PVR_TIMER &timer)
{
  return timer.iTimerType == Timer::Type::EPG_AUTO_SEARCH;
}

void Timers::GetTimerTypes(std::vector<PVR_TIMER_TYPE> &types)
{
  struct TimerType
    : PVR_TIMER_TYPE
  {
    TimerType(unsigned int id, unsigned int attributes,
      const std::string &description = std::string(),
      const std::vector< std::pair<int, std::string> > &groupValues
        = std::vector< std::pair<int, std::string> >(),
      const std::vector< std::pair<int, std::string> > &deDupValues
        = std::vector< std::pair<int, std::string> >())
    {
      int i;
      memset(this, 0, sizeof(PVR_TIMER_TYPE));

      iId         = id;
      iAttributes = attributes;
      PVR_STRCPY(strDescription, description.c_str());

      if ((iRecordingGroupSize = groupValues.size()))
        iRecordingGroupDefault = groupValues[0].first;
      i = 0;
      for (auto &group : groupValues)
      {
        recordingGroup[i].iValue = group.first;
        PVR_STRCPY(recordingGroup[i].strDescription, group.second.c_str());
        ++i;
      }

      if ((iPreventDuplicateEpisodesSize = deDupValues.size()))
        iPreventDuplicateEpisodesDefault = deDupValues[0].first;
      i = 0;
      for (auto &deDup : deDupValues)
      {
        preventDuplicateEpisodes[i].iValue = deDup.first;
        PVR_STRCPY(preventDuplicateEpisodes[i].strDescription,
            deDup.second.c_str());
        ++i;
      }      
    }
  };

  /* PVR_Timer.iRecordingGroup values and presentation.*/
  std::vector< std::pair<int, std::string> > groupValues = {
    { 0, LocalizedString(30410) }, //automatic
  };
  for (auto &recf : vuData.GetLocations())
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
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP,
      "", /* Let Kodi generate the description */
      groupValues);
  types.push_back(*t);

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
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP,
      "", /* Let Kodi generate the description */
      groupValues);
  types.push_back(*t);

  /* One-shot epg based */
  t = new TimerType(
      Timer::Type::EPG_ONCE,
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE,
      ""); /* Let Kodi generate the description */
  types.push_back(*t);

  /* Repeating epg based */
  t = new TimerType(
      Timer::Type::EPG_REPEATING,
      PVR_TIMER_TYPE_IS_REPEATING              |
      PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES     |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS         |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN,
      ""); /* Let Kodi generate the description */
  types.push_back(*t);

  if (CanAutoTimers())
  {
    /* PVR_Timer.iPreventDuplicateEpisodes values and presentation.*/
    static std::vector< std::pair<int, std::string> > deDupValues =
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
        PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN   |
        PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH    |
        PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH |
        PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP    |
        PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES,
        "", /* Let Kodi generate the description */
        groupValues, deDupValues);
    types.push_back(*t);
    types.back().iPreventDuplicateEpisodesDefault =
        AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC;

    /* One-shot created by epg auto search */
    types.emplace_back(TimerType(
        Timer::Type::EPG_AUTO_ONCE,
        PVR_TIMER_TYPE_IS_MANUAL                 |
        PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES     |
        PVR_TIMER_TYPE_IS_READONLY               |
        PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE   |
        PVR_TIMER_TYPE_SUPPORTS_CHANNELS         |
        PVR_TIMER_TYPE_SUPPORTS_START_TIME       |
        PVR_TIMER_TYPE_SUPPORTS_END_TIME         |
        PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
        PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP,
        LocalizedString(30420),
        groupValues));
    types.push_back(*t);
  }
}

int Timers::GetTimerCount()
{
  return m_timers.size();
}

int Timers::GetAutoTimerCount()
{
  return m_autotimers.size();
}

void Timers::GetTimers(std::vector<PVR_TIMER> &timers)
{
  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    Timer &timer = m_timers.at(i);
    XBMC->Log(LOG_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    tag.iTimerType        = timer.type;
    tag.iClientChannelUid = timer.iChannelId;
    tag.startTime         = timer.startTime;
    tag.endTime           = timer.endTime;
    strncpy(tag.strTitle, timer.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
    strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
    tag.state             = timer.state;
    tag.iPriority         = 0;     // unused
    tag.iLifetime         = 0;     // unused
    tag.firstDay          = 0;     // unused
    tag.iWeekdays         = timer.iWeekdays;
    tag.iEpgUid           = timer.iEpgID;
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused
    tag.iClientIndex = timer.iClientIndex;
    tag.iParentClientIndex = timer.iParentClientIndex;

    timers.emplace_back(tag);
  }
}

void Timers::GetAutoTimers(std::vector<PVR_TIMER> &timers)
{
  for (unsigned int i=0; i<m_autotimers.size(); i++)
  {
    AutoTimer &autoTimer = m_autotimers.at(i);
    XBMC->Log(LOG_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, autoTimer.strTitle.c_str(), autoTimer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    tag.iTimerType        = autoTimer.type;
    if (autoTimer.anyChannel)
      tag.iClientChannelUid = PVR_TIMER_ANY_CHANNEL;
    else
      tag.iClientChannelUid = autoTimer.iChannelId;
    tag.startTime         = autoTimer.startTime;
    tag.endTime           = autoTimer.endTime;
    strncpy(tag.strTitle, autoTimer.strTitle.c_str(), sizeof(tag.strTitle));
    //strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
    //strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
    tag.state             = autoTimer.state;
    tag.iPriority         = 0;     // unused
    tag.iLifetime         = 0;     // unused
    tag.firstDay          = 0;     // unused
    tag.iWeekdays         = autoTimer.iWeekdays;
    //tag.iEpgUid           = timer.iEpgID;
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused
    tag.iClientIndex = autoTimer.iClientIndex;
    strncpy(tag.strEpgSearchString, autoTimer.searchPhrase.c_str(), sizeof(tag.strEpgSearchString));
    tag.bStartAnyTime     = autoTimer.startAnyTime;
    tag.bEndAnyTime     = autoTimer.endAnyTime;
    tag.bFullTextEpgSearch = autoTimer.searchFulltext;
    tag.iPreventDuplicateEpisodes = autoTimer.deDup;

    timers.emplace_back(tag);
  }
}

Timer *Timers::GetTimer(std::function<bool (const Timer&)> func)
{
  return GetTimer<Timer>(func, m_timers);
}

AutoTimer *Timers::GetAutoTimer(std::function<bool (const AutoTimer&)> func)
{
  return GetTimer<AutoTimer>(func, m_autotimers);
}

PVR_ERROR Timers::AddTimer(const PVR_TIMER &timer)
{
  if (IsAutoTimer(timer))
    return AddAutoTimer(timer);

  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  std::string tags = "EPG";
  if (timer.iTimerType == Timer::MANUAL_ONCE || timer.iTimerType == Timer::MANUAL_REPEATING)
    tags = "Manual";

  std::string strTmp;
  std::string strServiceReference = vuData.GetChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();

  time_t startTime, endTime;
  startTime = timer.startTime - (timer.iMarginStart * 60);
  endTime = timer.endTime + (timer.iMarginEnd * 60);
  
  if (!g_strRecordingPath.compare(""))
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&tags=%s&dirname=&s", vuData.URLEncodeInline(strServiceReference).c_str(), timer.iWeekdays, startTime, endTime, vuData.URLEncodeInline(timer.strTitle).c_str(), vuData.URLEncodeInline(timer.strSummary).c_str(), timer.iEpgUid, vuData.URLEncodeInline(tags).c_str(), vuData.URLEncodeInline(g_strRecordingPath).c_str());
  else
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&tags=%s", vuData.URLEncodeInline(strServiceReference).c_str(), timer.iWeekdays, startTime, endTime, vuData.URLEncodeInline(timer.strTitle).c_str(), vuData.URLEncodeInline(timer.strSummary).c_str(), timer.iEpgUid, vuData.URLEncodeInline(tags).c_str());

  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;    
}

PVR_ERROR Timers::AddAutoTimer(const PVR_TIMER &timer)
{
  std::string strTmp;
  strTmp = StringUtils::Format("autotimer/edit?");

  strTmp += StringUtils::Format("name=%s", vuData.URLEncodeInline(timer.strTitle).c_str());
  strTmp += StringUtils::Format("&match=%s", vuData.URLEncodeInline(timer.strEpgSearchString).c_str());

  if (timer.state != PVR_TIMER_STATE_CANCELLED)
    strTmp += StringUtils::Format("&enabled=%s", vuData.URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
  else
    strTmp += StringUtils::Format("&enabled=%s", vuData.URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());


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

  strTmp += StringUtils::Format("&encoding=%s", vuData.URLEncodeInline(AUTOTIMER_ENCODING).c_str());
  strTmp += StringUtils::Format("&searchCase=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());
  if (timer.bFullTextEpgSearch)
    strTmp += StringUtils::Format("&searchType=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());  
  else
    strTmp += StringUtils::Format("&searchType=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

  std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.iPreventDuplicateEpisodes);
  if (deDup == AutoTimer::DeDup::DISABLED)
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=0");
    //strTmp += StringUtils::Format("&searchForDuplicateDescription=");
  }
  else
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=%s", AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE_OR_RECORDING);
    switch (deDup)
    {
      case AutoTimer::DeDup::CHECK_TITLE:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE);
        break;
      case AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC);
        break;
      // For AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS we don't need to set a value as this is the default
    }
  }

  if (timer.iClientChannelUid != PVR_TIMER_ANY_CHANNEL)
  {
    //single channel
    strTmp += StringUtils::Format("&services=%s", vuData.URLEncodeInline(vuData.GetChannels().at(timer.iClientChannelUid-1).strServiceReference).c_str());
  }

  bool everyday = true;
  std::string includeParams;
  if (timer.iWeekdays)
  {
      for (int i = 0; i < 7; i++)
      {
        if (1 & (timer.iWeekdays >> i))
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
  strTmp += includeParams;

  //TODO Tags

  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::UpdateTimer(const PVR_TIMER &timer)
{
  if (IsAutoTimer(timer))
    return UpdateAutoTimer(timer);
    
  XBMC->Log(LOG_DEBUG, "%s timer channelid '%d'", __FUNCTION__, timer.iClientChannelUid);

  std::string strTmp;
  std::string strServiceReference = vuData.GetChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();  

  unsigned int i=0;

  while (i<m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  Timer &oldTimer = m_timers.at(i);
  std::string strOldServiceReference = vuData.GetChannels().at(oldTimer.iChannelId-1).strServiceReference.c_str();  
  XBMC->Log(LOG_DEBUG, "%s old timer channelid '%d'", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
    iDisabled = 1;

  strTmp = StringUtils::Format("web/timerchange?sRef=%s&begin=%d&end=%d&name=%s&eventID=&description=%s&tags=%s&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%d&endOld=%d&deleteOldOnSave=1", vuData.URLEncodeInline(strServiceReference).c_str(), timer.startTime, timer.endTime, vuData.URLEncodeInline(timer.strTitle).c_str(), vuData.URLEncodeInline(timer.strSummary).c_str(), vuData.URLEncodeInline(oldTimer.tags).c_str(), iDisabled, timer.iWeekdays, vuData.URLEncodeInline(strOldServiceReference).c_str(), oldTimer.startTime, oldTimer.endTime  );
  
  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();   

  return PVR_ERROR_NO_ERROR; 
}

PVR_ERROR Timers::UpdateAutoTimer(const PVR_TIMER &timer)
{
  unsigned int i=0;
  while (i<m_autotimers.size())
  {
    if (m_autotimers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  AutoTimer timerToUpdate = m_autotimers.at(i);

  std::string strTmp;
  strTmp = StringUtils::Format("autotimer/edit?id=%d", timerToUpdate.backendId);

  strTmp += StringUtils::Format("&name=%s", vuData.URLEncodeInline(timer.strTitle).c_str());
  strTmp += StringUtils::Format("&match=%s", vuData.URLEncodeInline(timer.strEpgSearchString).c_str());

  if (timer.state != PVR_TIMER_STATE_CANCELLED)
    strTmp += StringUtils::Format("&enabled=%s", vuData.URLEncodeInline(AUTOTIMER_ENABLED_YES).c_str());
  else
    strTmp += StringUtils::Format("&enabled=%s", vuData.URLEncodeInline(AUTOTIMER_ENABLED_NO).c_str());


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

  strTmp += StringUtils::Format("&encoding=%s", vuData.URLEncodeInline(AUTOTIMER_ENCODING).c_str());

  if (timer.bFullTextEpgSearch)
    strTmp += StringUtils::Format("&searchType=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_TYPE_DESCRIPTION).c_str());  
  else
    strTmp += StringUtils::Format("&searchType=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_TYPE_EXACT).c_str());

  if (!AUTOTIMER_DEFAULT_OMIT(timerToUpdate.searchCase))
  strTmp += StringUtils::Format("&searchCase=%s", vuData.URLEncodeInline(AUTOTIMER_SEARCH_CASE_SENSITIVE).c_str());

  std::underlying_type<AutoTimer::DeDup>::type deDup = static_cast<AutoTimer::DeDup>(timer.iPreventDuplicateEpisodes);
  if (deDup == AutoTimer::DeDup::DISABLED)
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=0");
  }
  else
  {
    strTmp += StringUtils::Format("&avoidDuplicateDescription=%s", AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE_OR_RECORDING);
    switch (deDup)
    {
      case AutoTimer::DeDup::CHECK_TITLE:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE);
        break;
      case AutoTimer::DeDup::CHECK_TITLE_AND_SHORT_DESC:
        strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC);
        break;
      case AutoTimer::DeDup::CHECK_TITLE_AND_ALL_DESCS:
        //TODO Below is unsupported currently due to bug in the OpenWebIf API, the value cannot be unset
        //strTmp += StringUtils::Format("&searchForDuplicateDescription=%s", AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS);
        break;
    }
  }

  if (timerToUpdate.anyChannel && timer.iClientChannelUid != PVR_TIMER_ANY_CHANNEL)
  {
    //move to single channel
    strTmp += StringUtils::Format("&services=%s", vuData.URLEncodeInline(vuData.GetChannels().at(timer.iClientChannelUid-1).strServiceReference).c_str());
  }
  else if (!timerToUpdate.anyChannel && timer.iClientChannelUid == PVR_TIMER_ANY_CHANNEL)
  {
    //Move to any channel
    strTmp += StringUtils::Format("&services=");
  }

  bool everyday = true;
  std::string includeParams;
  if (timer.iWeekdays)
  {
      for (int i = 0; i < 7; i++)
      {
        if (1 & (timer.iWeekdays >> i))
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
  strTmp += includeParams;

  //TODO Tags
    //TODO Full EPG Search <include description match text

  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::DeleteTimer(const PVR_TIMER &timer)
{
  if (IsAutoTimer(timer))
    return DeleteAutoTimer(timer);

  std::string strTmp;
  std::string strServiceReference = vuData.GetChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();

  time_t startTime, endTime;
  startTime = timer.startTime - (timer.iMarginStart * 60);
  endTime = timer.endTime + (timer.iMarginEnd * 60);
  
  strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%d&end=%d", vuData.URLEncodeInline(strServiceReference).c_str(), startTime, endTime);

  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Timers::DeleteAutoTimer(const PVR_TIMER &timer)
{
  unsigned int i=0;
  while (i<m_autotimers.size())
  {
    if (m_autotimers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  AutoTimer timerToDelete = m_autotimers.at(i);

  std::string strTmp;
  strTmp = StringUtils::Format("autotimer/remove?id=%u", timerToDelete.backendId);

  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

void Timers::ClearTimers()
{
    m_timers.clear();
    m_autotimers.clear();
}

void Timers::TimerUpdates()
{
  bool regularTimersChanged = TimerUpdatesRegular();
  bool autoTimersChanged = TimerUpdatesAuto();

  if (regularTimersChanged || autoTimersChanged) 
  {
    XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}

bool Timers::TimerUpdatesRegular()
{
  std::vector<Timer> newtimers = LoadTimers();

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    m_timers[i].iUpdateState = VU_UPDATE_STATE_NONE;
  }

  unsigned int iUpdated=0;
  unsigned int iUnchanged=0; 

  for (unsigned int j=0;j<newtimers.size(); j++) 
  {
    for (unsigned int i=0; i<m_timers.size(); i++) 
    {
      if (m_timers[i].like(newtimers[j]))
      {
        if(m_timers[i] == newtimers[j])
        {
          m_timers[i].iUpdateState = VU_UPDATE_STATE_FOUND;
          newtimers[j].iUpdateState = VU_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newtimers[j].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].strTitle = newtimers[j].strTitle;
          m_timers[i].strPlot = newtimers[j].strPlot;
          m_timers[i].iChannelId = newtimers[j].iChannelId;
          m_timers[i].strChannelName = newtimers[j].strChannelName;
          m_timers[i].startTime = newtimers[j].startTime;
          m_timers[i].endTime = newtimers[j].endTime;
          m_timers[i].iWeekdays = newtimers[j].iWeekdays;
          m_timers[i].iEpgID = newtimers[j].iEpgID;
          m_timers[i].tags = newtimers[j].tags;

          iUpdated++;
        }
      }
    }
  }

  unsigned int iRemoved = 0;

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    if (m_timers.at(i).iUpdateState == VU_UPDATE_STATE_NONE)
    {
        XBMC->Log(LOG_INFO, "%s Removed timer: '%s', ClientIndex: '%d'", __FUNCTION__, m_timers.at(i).strTitle.c_str(), m_timers.at(i).iClientIndex);
        m_timers.erase(m_timers.begin()+i);
        i=0;
        iRemoved++;
    }
  }
  unsigned int iNew=0;

  for (unsigned int i=0; i<newtimers.size();i++)
  { 
    if(newtimers.at(i).iUpdateState == VU_UPDATE_STATE_NEW)
    {  
      Timer &timer = newtimers.at(i);
      timer.iClientIndex = m_iClientIndexCounter;
      timer.strChannelName = vuData.GetChannels().at(timer.iChannelId-1).strChannelName;
      XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
      m_iClientIndexCounter++;
      iNew++;  
    } 
  }  

  XBMC->Log(LOG_INFO , "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 

  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

bool Timers::TimerUpdatesAuto()
{
  std::vector<AutoTimer> newautotimers = LoadAutoTimers();

  for (unsigned int i=0; i<m_autotimers.size(); i++)
  {
    m_autotimers[i].iUpdateState = VU_UPDATE_STATE_NONE;
  }

  unsigned int iUpdated=0;
  unsigned int iUnchanged=0; 

  for (unsigned int j=0;j<m_autotimers.size(); j++) 
  {
    for (unsigned int i=0; i<m_autotimers.size(); i++) 
    {
      if (m_autotimers[i].like(newautotimers[j]))
      {
        if(m_autotimers[i] == newautotimers[j])
        {
          m_autotimers[i].iUpdateState = VU_UPDATE_STATE_FOUND;
          newautotimers[j].iUpdateState = VU_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newautotimers[j].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_autotimers[i].iUpdateState = VU_UPDATE_STATE_UPDATED;

          m_autotimers[i].strTitle = newautotimers[j].strTitle;
          m_autotimers[i].strPlot = newautotimers[j].strPlot;
          m_autotimers[i].iChannelId = newautotimers[j].iChannelId;
          m_autotimers[i].strChannelName = newautotimers[j].strChannelName;
          m_autotimers[i].startTime = newautotimers[j].startTime;
          m_autotimers[i].endTime = newautotimers[j].endTime;
          m_autotimers[i].iWeekdays = newautotimers[j].iWeekdays;
          m_autotimers[i].iEpgID = newautotimers[j].iEpgID;
          m_autotimers[i].tags = newautotimers[j].tags;

          m_autotimers[i].searchPhrase = newautotimers[j].searchPhrase;
          m_autotimers[i].encoding = newautotimers[j].encoding;
          m_autotimers[i].searchCase = newautotimers[j].searchCase;
          m_autotimers[i].searchType = newautotimers[j].searchType;
          m_autotimers[i].searchFulltext = newautotimers[j].searchFulltext;
          m_autotimers[i].startAnyTime = newautotimers[j].startAnyTime;
          m_autotimers[i].endAnyTime = newautotimers[j].endAnyTime;
          m_autotimers[i].anyChannel = newautotimers[j].anyChannel;
          m_autotimers[i].deDup = newautotimers[j].deDup;

          iUpdated++;
        }
      }
    }
  }

  unsigned int iRemoved = 0;

  for (unsigned int i=0; i<m_autotimers.size(); i++)
  {
    if (m_autotimers.at(i).iUpdateState == VU_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_INFO, "%s Removed timer: '%s', ClientIndex: '%d'", __FUNCTION__, m_autotimers.at(i).strTitle.c_str(), m_autotimers.at(i).iClientIndex);
      m_autotimers.erase(m_autotimers.begin()+i);
      i=0;
      iRemoved++;
    }
  }
  unsigned int iNew=0;

  for (unsigned int i=0; i<newautotimers.size();i++)
  { 
    if(newautotimers.at(i).iUpdateState == VU_UPDATE_STATE_NEW)
    {  
      AutoTimer &autoTimer = newautotimers.at(i);
      autoTimer.iClientIndex = m_iClientIndexCounter;
      autoTimer.strChannelName = vuData.GetChannels().at(autoTimer.iChannelId-1).strChannelName;
      XBMC->Log(LOG_INFO, "%s New auto timer: '%s', ClientIndex: '%d'", __FUNCTION__, autoTimer.strTitle.c_str(), m_iClientIndexCounter);
      m_autotimers.push_back(autoTimer);
      m_iClientIndexCounter++;
      iNew++;
    } 
  }

  for (unsigned int i=0; i<m_timers.size();i++)
  {
    for (unsigned int j=0; j<m_autotimers.size();j++)
    {
      std::string autotimerTag = ConvertToAutoTimerTag(m_autotimers.at(j).strTitle);

      if (m_timers.at(i).type == Timer::EPG_AUTO_ONCE && FindTagInTimerTags(autotimerTag, m_timers.at(i).tags))
      {
        m_timers.at(i).iParentClientIndex = m_autotimers.at(j).iClientIndex;
        continue;
      }
    }
  }
 
  XBMC->Log(LOG_INFO, "%s No of autotimers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 
  
  return (iRemoved != 0 || iUpdated != 0 || iNew != 0);
}

void Timers::ParseTime(const std::string &time, std::tm &timeinfo)
{
  std::sscanf(time.c_str(), "%02d:%02d", &timeinfo.tm_hour,
      &timeinfo.tm_min);
}