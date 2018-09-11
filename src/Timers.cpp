#include "Timers.h"
#include "client.h"
#include "VuData.h"
#include "LocalizedString.h"

#include <algorithm>
#include <ctime>

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

    timer.strChannelName = vuData.getChannels().at(timer.iChannelId-1).strChannelName;  

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

    std::string tags = "Blank";
    if (XMLUtils::GetString(pNode, "e2tags", strTmp))
      tags        = strTmp.c_str();

    if (tags == "Manual")
    {
      if (timer.iWeekdays)
        timer.type = Timer::MANUAL_REPEATING;
      else
        timer.type =  Timer::MANUAL_ONCE;
    }
    else
    { //Default to EPG
      timer.type = Timer::EPG_ONCE;
    }

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return timers; 
}

void Timers::GetTimerTypes(std::vector<PVR_TIMER_TYPE> &types)
{
  struct TimerType
    : PVR_TIMER_TYPE
  {
    TimerType(unsigned int id, unsigned int attributes,
      const std::string &description = std::string(),
      const std::vector< std::pair<int, std::string> > &groupValues
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
    }
  };

  /* PVR_Timer.iRecordingGroup values and presentation.*/
  std::vector< std::pair<int, std::string> > groupValues = {
    { 0, LocalizedString(30410) }, //automatic
  };
  for (auto &recf : vuData.getLocations())
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
}

int Timers::GetTimerCount()
{
  return m_timers.size();
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

    timers.emplace_back(tag);
  }
}

Timer *Timers::GetTimer(std::function<bool (const Timer&)> func)
{
  return GetTimer<Timer>(func, m_timers);
}

PVR_ERROR Timers::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  std::string tags = "EPG";
  if (timer.iTimerType == Timer::MANUAL_ONCE || timer.iTimerType == Timer::MANUAL_REPEATING)
    tags = "Manual";

  std::string strTmp;
  std::string strServiceReference = vuData.getChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();

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

PVR_ERROR Timers::UpdateTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s timer channelid '%d'", __FUNCTION__, timer.iClientChannelUid);

  std::string strTmp;
  std::string strServiceReference = vuData.getChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();  

  unsigned int i=0;

  while (i<m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  Timer &oldTimer = m_timers.at(i);
  std::string strOldServiceReference = vuData.getChannels().at(oldTimer.iChannelId-1).strServiceReference.c_str();  
  XBMC->Log(LOG_DEBUG, "%s old timer channelid '%d'", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
    iDisabled = 1;

  strTmp = StringUtils::Format("web/timerchange?sRef=%s&begin=%d&end=%d&name=%s&eventID=&description=%s&tags=&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%d&endOld=%d&deleteOldOnSave=1", vuData.URLEncodeInline(strServiceReference).c_str(), timer.startTime, timer.endTime, vuData.URLEncodeInline(timer.strTitle).c_str(), vuData.URLEncodeInline(timer.strSummary).c_str(), iDisabled, timer.iWeekdays, vuData.URLEncodeInline(strOldServiceReference).c_str(), oldTimer.startTime, oldTimer.endTime  );
  
  std::string strResult;
  if(!vuData.SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();   

  return PVR_ERROR_NO_ERROR; 
}

PVR_ERROR Timers::DeleteTimer(const PVR_TIMER &timer)
{
  std::string strTmp;
  std::string strServiceReference = vuData.getChannels().at(timer.iClientChannelUid-1).strServiceReference.c_str();

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

void Timers::ClearTimers()
{
    m_timers.clear();
}

void Timers::TimerUpdates()
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
          m_timers[i].startTime = newtimers[j].startTime;
          m_timers[i].endTime = newtimers[j].endTime;
          m_timers[i].iWeekdays = newtimers[j].iWeekdays;
          m_timers[i].iEpgID = newtimers[j].iEpgID;

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
      timer.strChannelName = vuData.getChannels().at(timer.iChannelId-1).strChannelName;
      XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
      m_iClientIndexCounter++;
      iNew++;
    } 
  }
 
  XBMC->Log(LOG_INFO , "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 

  if (iRemoved != 0 || iUpdated != 0 || iNew != 0) 
  {
    XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}