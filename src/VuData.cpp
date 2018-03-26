/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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

#include "VuData.h"
#include "client.h" 

#include <algorithm>
#include <iostream> 
#include <fstream> 
#include <string>
#include <p8-platform/util/StringUtils.h>
#include "util/XMLUtils.h"

#if defined(_WIN32)
#include <Bits.h>
#endif


using namespace ADDON;
using namespace P8PLATFORM;

bool CCurlFile::Get(const std::string &strURL, std::string &strResult)
{
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
      strResult.append(buffer);
    XBMC->CloseFile(fileHandle);
    return true;
  }
  return false;
}

std::string& Vu::Escape(std::string &s, std::string from, std::string to)
{ 
  std::string::size_type pos = -1;
  while ( (pos = s.find(from, pos+1) ) != std::string::npos)         
    s.erase(pos, from.length()).insert(pos, to);        

  return s;     
} 

bool Vu::LoadLocations() 
{
  std::string url;
  if (g_bOnlyCurrentLocation)
    url = StringUtils::Format("%s%s",  m_strURL.c_str(), "web/getcurrlocation"); 
  else 
    url = StringUtils::Format("%s%s",  m_strURL.c_str(), "web/getlocations"); 
 
  std::string strXML;
  strXML = GetHttpXML(url);

  int iNumLocations = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2locations> element");
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2location> element");
    return false;
  }

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2location"))
  {
    std::string strTmp;
    strTmp = pNode->GetText();

    m_locations.push_back(strTmp);
    iNumLocations++;

    XBMC->Log(LOG_DEBUG, "%s Added '%s' as a recording location", __FUNCTION__, strTmp.c_str());
  }

  XBMC->Log(LOG_INFO, "%s Loded '%d' recording locations", __FUNCTION__, iNumLocations);

  return true;
}

void Vu::TimerUpdates()
{
  std::vector<VuTimer> newtimer = LoadTimers();

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    m_timers[i].iUpdateState = VU_UPDATE_STATE_NONE;
  }

  unsigned int iUpdated=0;
  unsigned int iUnchanged=0; 

  for (unsigned int j=0;j<newtimer.size(); j++) 
  {
    for (unsigned int i=0; i<m_timers.size(); i++) 
    {
      if (m_timers[i].like(newtimer[j]))
      {
        if(m_timers[i] == newtimer[j])
        {
          m_timers[i].iUpdateState = VU_UPDATE_STATE_FOUND;
          newtimer[j].iUpdateState = VU_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newtimer[j].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].iUpdateState = VU_UPDATE_STATE_UPDATED;
          m_timers[i].strTitle = newtimer[j].strTitle;
          m_timers[i].strPlot = newtimer[j].strPlot;
          m_timers[i].iChannelId = newtimer[j].iChannelId;
          m_timers[i].startTime = newtimer[j].startTime;
          m_timers[i].endTime = newtimer[j].endTime;
          m_timers[i].iWeekdays = newtimer[j].iWeekdays;
          m_timers[i].iEpgID = newtimer[j].iEpgID;

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

  for (unsigned int i=0; i<newtimer.size();i++)
  { 
    if(newtimer.at(i).iUpdateState == VU_UPDATE_STATE_NEW)
    {  
      VuTimer &timer = newtimer.at(i);
      timer.iClientIndex = m_iClientIndexCounter;
      XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
      m_iClientIndexCounter++;
      iNew++;
    } 
  }
 
  XBMC->Log(LOG_INFO, "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 

  if (iRemoved != 0 || iUpdated != 0 || iNew != 0) 
  {
    XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}

Vu::Vu() 
{
  m_bIsConnected = false;
  m_strServerName = "Vu";
  std::string strURL = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
  {
    strURL = StringUtils::Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  }
  
  if (!g_bUseSecureHTTP)
    strURL = StringUtils::Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  else
    strURL = StringUtils::Format("https://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  
  m_strURL = strURL.c_str();

  m_iNumRecordings = 0;
  m_iNumChannelGroups = 0;
  m_iCurrentChannel = -1;
  m_iClientIndexCounter = 1;

  m_bUpdating = false;
  m_iUpdateTimer = 0;
  m_bInitialEPG = true;

  std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
  m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
  XBMC->WriteFile(m_writeHandle, "Y", 1);
  XBMC->CloseFile(m_writeHandle);
  
}

bool Vu::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - VU+ Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);
  XBMC->Log(LOG_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, g_iPortStream);
  if (!g_bUseSecureHTTP)
    XBMC->Log(LOG_NOTICE, "%s Use HTTPS: 'false'", __FUNCTION__);
  else
    XBMC->Log(LOG_NOTICE, "%s Use HTTPS: 'true'", __FUNCTION__);
  
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
  {
    if ((g_strUsername.find("@") != std::string::npos) || (g_strPassword.find("@") != std::string::npos))
    {
      XBMC->Log(LOG_ERROR, "%s - You cannot use the '@' character in either the username or the password with this addon. Please change your configuraton!", __FUNCTION__);
      return false;
    }
  } 
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
  {
    XBMC->Log(LOG_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    return false;
  }

  LoadLocations();

  if (m_channels.size() == 0) 
  {
    // Load the TV channels - close connection if no channels are found
    if (!LoadChannelGroups())
      return false;

    if (!LoadChannels())
      return false;

  }
  TimerUpdates();

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 
  
  return IsRunning(); 
}

void  *Vu::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  // Wait for the initial EPG update to complete 
  bool bwait = true;
  int cycles = 0;

  while (bwait)
  {
    if (cycles == 30)
      bwait = false;

    cycles++;
    std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
    m_readHandle = XBMC->OpenFile(initialEPGReady.c_str(), 0);
    byte buf[1];
    XBMC->ReadFile(m_readHandle, buf, 1);
    XBMC->CloseFile(m_readHandle);
    char buf2[] = { "N" };
    if (buf[0] == buf2[0])
    {
      XBMC->Log(LOG_DEBUG, "%s - Intial EPG update COMPLETE!", __FUNCTION__);
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s - Intial EPG update not completed yet.", __FUNCTION__);
      Sleep(5 * 1000);
    }
  }

  // Trigger "Real" EPG updates 
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    XBMC->Log(LOG_DEBUG, "%s - Trigger EPG update for channel '%d'", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_channels.at(iChannelPtr).iUniqueId);
  }

  while(!IsStopped())
  {
    Sleep(5 * 1000);
    m_iUpdateTimer += 5;

    if ((int)m_iUpdateTimer > (g_iUpdateInterval * 60)) 
    {
      m_iUpdateTimer = 0;
 
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "%s Perform Updates!", __FUNCTION__);

      if (g_bAutomaticTimerlistCleanup) 
      {
        std::string strTmp;
        strTmp = StringUtils::Format("web/timercleanup?cleanup=true");
        std::string strResult;
        if(!SendSimpleCommand(strTmp, strResult))
          XBMC->Log(LOG_ERROR, "%s - AutomaticTimerlistCleanup failed!", __FUNCTION__);
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }

  }

  //CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}

bool Vu::LoadChannels() 
{    
  bool bOk = false;

  m_channels.clear();
  // Load Channels
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    VuChannelGroup &myGroup = m_groups.at(i);
    if (LoadChannels(myGroup.strServiceReference, myGroup.strGroupName))
      bOk = true;
  }

  // Load the radio channels - continue if no channels are found 
  std::string strTmp;
  strTmp = StringUtils::Format("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet");
  LoadChannels(strTmp, "radio");

  return bOk;
}

bool Vu::LoadChannelGroups() 
{
  std::string strTmp; 

  strTmp = StringUtils::Format("%sweb/getservices", m_strURL.c_str());

  std::string strXML = GetHttpXML(strTmp);  

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2service> element", __FUNCTION__);
    return false;
  }

  m_groups.clear();
  m_iNumChannelGroups = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    std::string strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannelGroup newGroup;
    newGroup.strServiceReference = strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTmp)) 
      continue;

    newGroup.strGroupName = strTmp;

    if (g_bOnlyOneGroup && g_strOneGroup.compare(strTmp.c_str())) {
        XBMC->Log(LOG_INFO, "%s Only one group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, strTmp.c_str(), g_strOneGroup.c_str());
        continue;
    }
 
    m_groups.push_back(newGroup);

    XBMC->Log(LOG_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newGroup.strGroupName.c_str());
    m_iNumChannelGroups++; 
  }

  XBMC->Log(LOG_INFO, "%s Loaded %d Channelsgroups", __FUNCTION__, m_iNumChannelGroups);
  return true;
}

bool Vu::LoadChannels(std::string strServiceReference, std::string strGroupName) 
{
  XBMC->Log(LOG_INFO, "%s loading channel group: '%s'", __FUNCTION__, strGroupName.c_str());

  std::string strTmp;
  strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", m_strURL.c_str(), URLEncodeInline(strServiceReference).c_str());

  std::string strXML = GetHttpXML(strTmp);  
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2service> element");
    return false;
  }
  
  bool bRadio;

  bRadio = !strGroupName.compare("radio");

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    std::string strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      continue;
    
    // Check whether the current element is not just a label
    if (strTmp.compare(0,5,"1:64:") == 0)
      continue;

    VuChannel newChannel;
    newChannel.bRadio = bRadio;
    newChannel.bInitialEPG = true;
    newChannel.strGroupName = strGroupName;
    newChannel.iUniqueId = m_channels.size()+1;
    newChannel.iChannelNumber = m_channels.size()+1;
    newChannel.strServiceReference = strTmp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTmp)) 
      continue;

    newChannel.strChannelName = strTmp;
 
    std::string strIcon;
    strIcon = newChannel.strServiceReference.c_str();

    int j = 0;
    std::string::iterator it = strIcon.begin();

    while (j<10 && it != strIcon.end())
    {
      if (*it == ':')
        j++;

      it++;
    }
    std::string::size_type index = it-strIcon.begin();

    strIcon = strIcon.substr(0,index);

    it = strIcon.end() - 1;
    if (*it == ':')
    {
      strIcon.erase(it);
    }
    
    std::string strTmp2;

    strTmp2 = StringUtils::Format("%s", strIcon.c_str());

    std::replace(strIcon.begin(), strIcon.end(), ':', '_');
    strIcon = g_strIconPath.c_str() + strIcon + ".png";

    newChannel.strIconPath = strIcon;

    strTmp = StringUtils::Format("%s/web/stream.m3u?ref=%s", m_strURL.c_str(), URLEncodeInline(newChannel.strServiceReference).c_str());
    newChannel.strM3uURL = strTmp;

    strTmp = StringUtils::Format("http://%s:%d/%s", g_strHostname.c_str(), g_iPortStream, strTmp2.c_str());
    newChannel.strStreamURL = strTmp;

    if (g_bOnlinePicons == true)
    {
      std::replace(strTmp2.begin(), strTmp2.end(), ':', '_');
      strTmp = StringUtils::Format("%spicon/%s.png", m_strURL.c_str(), strTmp2.c_str());
      newChannel.strIconPath = strTmp;
    }

    m_channels.push_back(newChannel);
    XBMC->Log(LOG_INFO, "%s Loaded channel: %s, Icon: %s", __FUNCTION__, newChannel.strChannelName.c_str(), newChannel.strIconPath.c_str());
  }

  XBMC->Log(LOG_INFO, "%s Loaded %d Channels", __FUNCTION__, m_channels.size());
  return true;
}

bool Vu::IsConnected() 
{
  return m_bIsConnected;
}

/**
  * GetStreamURL() reads out a stream-URL from a M3U-file.
  *
  * This method downloads a M3U-file from the address that is given by strM3uURL.
  * It returns the first line that starts with "http".
  * If no line starts with "http" the last line is returned.
  */
std::string Vu::GetStreamURL(std::string& strM3uURL)
{
  std::string strTmp;
  strTmp = strM3uURL;
  std::string strM3U;
  strM3U = GetHttpXML(strTmp);
  std::istringstream streamM3U(strM3U);
  std::string strURL = "";
  while (std::getline(streamM3U, strURL))
  {
    if (strURL.compare(0, 4, "http", 4) == 0)
      break;
  };
  return strURL;
}

std::string Vu::GetHttpXML(std::string& url)
{
//  CLockObject lock(m_mutex);

  XBMC->Log(LOG_INFO, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  std::string strTmp;

  CCurlFile http;
  if(!http.Get(url, strTmp))
  {
    XBMC->Log(LOG_DEBUG, "%s - Could not open webAPI.", __FUNCTION__);
    return "";
  }

  XBMC->Log(LOG_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());
  

  return strTmp;
}

const char * Vu::GetServerName() 
{
  return m_strServerName.c_str();  
}

int Vu::GetChannelsAmount()
{
  return m_channels.size();
}

int Vu::GetTimersAmount()
{
  return m_timers.size();
}

unsigned int Vu::GetRecordingsAmount() {
  return m_iNumRecordings;
}

PVR_ERROR Vu::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    VuChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName));
      strncpy(xbmcChannel.strInputFormat, "", 0); // unused
      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.bIsHidden         = false;
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath));

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

std::string Vu::GetChannelURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  if (g_bAutoConfig)
  {
    // we need to download the M3U file that contains the URL for the stream...
    // we do it here for 2 reasons:
    //  1. This is faster than doing it during initialization
    //  2. The URL can change, so this is more up-to-date.
    return GetStreamURL(m_channels.at(channelinfo.iUniqueId - 1).strM3uURL);
  }

  return m_channels.at(channelinfo.iUniqueId - 1).strStreamURL;
}

Vu::~Vu() 
{
  CLockObject lock(m_mutex);
  XBMC->Log(LOG_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.clear();  
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal timers list...", __FUNCTION__);
  m_timers.clear();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal recordings list...", __FUNCTION__);
  m_recordings.clear();
  
  XBMC->Log(LOG_DEBUG, "%s Removing internal group list...", __FUNCTION__);
  m_groups.clear();
  m_bIsConnected = false;
}

bool Vu::GetInitialEPGForGroup(VuChannelGroup &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  std::string url;
  url = StringUtils::Format("%s%s%s",  m_strURL.c_str(), "web/epgnownext?bRef=",  URLEncodeInline(group.strServiceReference).c_str());
 
  std::string strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2event"))
  {
    std::string strTmp;

    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart)) 
      continue;

    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
      continue;

    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventid", entry.iEventId))  
      continue;

    
    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;

    if(!XMLUtils::GetString(pNode, "e2eventservicereference", strTmp))
      continue;

    entry.strServiceReference = strTmp;
    
    entry.iChannelId = GetChannelNumber(entry.strServiceReference.c_str());

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
       entry.strPlotOutline = strTmp;

    iNumEPG++; 
    
    group.initialEPG.push_back(entry);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group.strGroupName.c_str());
  return true;
}

PVR_ERROR Vu::GetInitialEPGForChannel(ADDON_HANDLE handle, const VuChannel &channel, time_t iStart, time_t iEnd)
{
  if (m_iNumChannelGroups < 1)
    return PVR_ERROR_SERVER_ERROR;

  XBMC->Log(LOG_DEBUG, "%s Fetch information for group '%s'", __FUNCTION__, channel.strGroupName.c_str());

  VuChannelGroup &myGroup = m_groups.at(0);
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    myGroup = m_groups.at(i);
    if (!myGroup.strGroupName.compare(channel.strGroupName))
      if (myGroup.initialEPG.size() == 0)
      {
        GetInitialEPGForGroup(myGroup);
        break;
      }
  }

  XBMC->Log(LOG_DEBUG, "%s initialEPG size is now '%d'", __FUNCTION__, myGroup.initialEPG.size());
  
  for (unsigned int i = 0;i<myGroup.initialEPG.size();  i++) 
  {
    VuEPGEntry &entry = myGroup.initialEPG.at(i);
    if (!channel.strServiceReference.compare(entry.strServiceReference)) 
    {
      EPG_TAG broadcast;
      memset(&broadcast, 0, sizeof(EPG_TAG));

      broadcast.iUniqueBroadcastId  = entry.iEventId;
      broadcast.strTitle            = entry.strTitle.c_str();
      broadcast.iUniqueChannelId    = channel.iUniqueId;
      broadcast.startTime           = entry.startTime;
      broadcast.endTime             = entry.endTime;
      broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
      broadcast.strPlot             = entry.strPlot.c_str();
      broadcast.strOriginalTitle    = NULL; // unused
      broadcast.strCast             = NULL; // unused
      broadcast.strDirector         = NULL; // unused
      broadcast.strWriter           = NULL; // unused
      broadcast.iYear               = 0;    // unused
      broadcast.strIMDBNumber       = NULL; // unused
      broadcast.strIconPath         = ""; // unused
      broadcast.iGenreType          = 0; // unused
      broadcast.iGenreSubType       = 0; // unused
      broadcast.strGenreDescription = "";
      broadcast.firstAired          = 0;  // unused
      broadcast.iParentalRating     = 0;  // unused
      broadcast.iStarRating         = 0;  // unused
      broadcast.bNotify             = false;
      broadcast.iSeriesNumber       = 0;  // unused
      broadcast.iEpisodeNumber      = 0;  // unused
      broadcast.iEpisodePartNumber  = 0;  // unused
      broadcast.strEpisodeName      = ""; // unused
      broadcast.iFlags              = EPG_TAG_FLAG_UNDEFINED;

      PVR->TransferEpgEntry(handle, &broadcast);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  if (channel.iUniqueId-1 > m_channels.size())
  {
    XBMC->Log(LOG_ERROR, "%s Could not fetch cannel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  VuChannel myChannel;
  myChannel = m_channels.at(channel.iUniqueId-1);

  // Check if the initial short import has already been done for this channel
  if (m_channels.at(channel.iUniqueId-1).bInitialEPG == true)
  {
    m_channels.at(channel.iUniqueId-1).bInitialEPG = false;
  
    // Check if all channels have completed the initial EPG import
    m_bInitialEPG = false;
    for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
    {
      if (m_channels.at(iChannelPtr).bInitialEPG == true) 
      {
        m_bInitialEPG = true;
      }
    }

    if (!m_bInitialEPG)
    {
      std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
      m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
      XBMC->WriteFile(m_writeHandle, "N", 1);
      XBMC->CloseFile(m_writeHandle);
    }
    return GetInitialEPGForChannel(handle, myChannel, iStart, iEnd);
  }

  std::string url;
  url = StringUtils::Format("%s%s%s",  m_strURL.c_str(), "web/epgservice?sRef=",  URLEncodeInline(myChannel.strServiceReference).c_str());
 
  std::string strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_NO_ERROR;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_SERVER_ERROR;
  }
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2event"))
  {
    std::string strTmp;

    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart)) 
      continue;

    // Skip unneccessary events
    if (iStart > iTmpStart)
      continue;
 
    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
      continue;

    if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
       continue;
    
    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventid", entry.iEventId))  
      continue;

    entry.iChannelId = channel.iUniqueId;
    
    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;
    
    entry.strServiceReference = myChannel.strServiceReference.c_str();

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
       entry.strPlotOutline = strTmp;

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));

    broadcast.iUniqueBroadcastId  = entry.iEventId;
    broadcast.strTitle            = entry.strTitle.c_str();
    broadcast.iUniqueChannelId    = channel.iUniqueId;
    broadcast.startTime           = entry.startTime;
    broadcast.endTime             = entry.endTime;
    broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
    broadcast.strPlot             = entry.strPlot.c_str();
    broadcast.strOriginalTitle    = NULL; // unused
    broadcast.strCast             = NULL; // unused
    broadcast.strDirector         = NULL; // unused
    broadcast.strWriter           = NULL; // unused
    broadcast.iYear               = 0;    // unused
    broadcast.strIMDBNumber       = NULL; // unused
    broadcast.strIconPath         = ""; // unused
    broadcast.iGenreType          = 0; // unused
    broadcast.iGenreSubType       = 0; // unused
    broadcast.strGenreDescription = "";
    broadcast.firstAired          = 0;  // unused
    broadcast.iParentalRating     = 0;  // unused
    broadcast.iStarRating         = 0;  // unused
    broadcast.bNotify             = false;
    broadcast.iSeriesNumber       = 0;  // unused
    broadcast.iEpisodeNumber      = 0;  // unused
    broadcast.iEpisodePartNumber  = 0;  // unused
    broadcast.strEpisodeName      = ""; // unused
    broadcast.iFlags              = EPG_TAG_FLAG_UNDEFINED;

    PVR->TransferEpgEntry(handle, &broadcast);

    iNumEPG++; 

    XBMC->Log(LOG_DEBUG, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

int Vu::GetChannelNumber(std::string strServiceReference)  
{
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    if (!strServiceReference.compare(m_channels[i].strServiceReference))
      return i+1;
  }
  return -1;
}

std::string Vu::GetChannelIconPath(std::string strChannelName)  
{
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    if (!strChannelName.compare(m_channels[i].strChannelName))
      return m_channels[i].strIconPath;
  }
  return "";
}

PVR_ERROR Vu::GetTimers(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    VuTimer &timer = m_timers.at(i);
    XBMC->Log(LOG_DEBUG, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    /* TODO: Implement own timer types to get support for the timer features introduced with PVR API 1.9.7 */
    tag.iTimerType = PVR_TIMER_TYPE_NONE;

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

    PVR->TransferTimerEntry(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

std::vector<VuTimer> Vu::LoadTimers()
{
  std::vector<VuTimer> timers;

  std::string url; 
  url = StringUtils::Format("%s%s", m_strURL.c_str(), "web/timerlist"); 

  std::string strXML;
  strXML = GetHttpXML(url);

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

    VuTimer timer;
    
    timer.strTitle          = strTmp;

    if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      timer.iChannelId = GetChannelNumber(strTmp.c_str());

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

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return timers; 
}

bool Vu::SendSimpleCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  std::string url; 
  url = StringUtils::Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 

  std::string strXML;
  strXML = GetHttpXML(url);
  
  if (!bIgnoreResult)
  {
    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);

    pElem = hDoc.FirstChildElement("e2simplexmlresult").Element();

    if (!pElem)
    {
      XBMC->Log(LOG_DEBUG, "%s Could not find <e2simplexmlresult> element!", __FUNCTION__);
      return false;
    }

    bool bTmp;

    if (!XMLUtils::GetBoolean(pElem, "e2state", bTmp)) 
    {
      XBMC->Log(LOG_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      strResultText = StringUtils::Format("Could not parse e2state!");
      return false;
    }

    if (!XMLUtils::GetString(pElem, "e2statetext", strResultText)) 
    {
      XBMC->Log(LOG_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
      XBMC->Log(LOG_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());

    return bTmp;
  }
  return true;
}


PVR_ERROR Vu::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  std::string strTmp;
  std::string strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();

  time_t startTime, endTime;
  startTime = timer.startTime - (timer.iMarginStart * 60);
  endTime = timer.endTime + (timer.iMarginEnd * 60);
  
  if (!g_strRecordingPath.compare(""))
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d&dirname=&s", URLEncodeInline(strServiceReference).c_str(), timer.iWeekdays, startTime, endTime, URLEncodeInline(timer.strTitle).c_str(), URLEncodeInline(timer.strSummary).c_str(), timer.iEpgUid, URLEncodeInline(g_strRecordingPath).c_str());
  else
    strTmp = StringUtils::Format("web/timeradd?sRef=%s&repeated=%d&begin=%d&end=%d&name=%s&description=%s&eit=%d", URLEncodeInline(strServiceReference).c_str(), timer.iWeekdays, startTime, endTime, URLEncodeInline(timer.strTitle).c_str(), URLEncodeInline(timer.strSummary).c_str(), timer.iEpgUid);

  std::string strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::DeleteTimer(const PVR_TIMER &timer) 
{
  std::string strTmp;
  std::string strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();

  time_t startTime, endTime;
  startTime = timer.startTime - (timer.iMarginStart * 60);
  endTime = timer.endTime + (timer.iMarginEnd * 60);
  
  strTmp = StringUtils::Format("web/timerdelete?sRef=%s&begin=%d&end=%d", URLEncodeInline(strServiceReference).c_str(), startTime, endTime);

  std::string strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_SERVER_ERROR;

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::GetRecordings(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  m_iNumRecordings = 0;
  m_recordings.clear();

  for (unsigned int i=0; i<m_locations.size(); i++)
  {
    if (!GetRecordingFromLocation(m_locations[i]))
    {
      XBMC->Log(LOG_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, m_locations[i].c_str());
    }
  }

  TransferRecordings(handle);

  return PVR_ERROR_NO_ERROR;
}

bool Vu::IsInRecordingFolder(std::string strRecordingFolder)
{
  int iMatches = 0;
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    if (strRecordingFolder.compare(m_recordings.at(i).strTitle) == 0)
    {
      iMatches++;
      XBMC->Log(LOG_DEBUG, "%s Found Recording title '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
      if (iMatches > 1)
      {
        XBMC->Log(LOG_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
        return true;    
      }
    }
  }

  return false;
}

void Vu::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i=0; i<m_recordings.size(); i++)
  {
    std::string strTmp;
    VuRecording &recording = m_recordings.at(i);
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId));
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot));
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strIconPath, recording.strIconPath.c_str(), sizeof(tag.strIconPath));

    if (!g_bKeepFolders)
    {
      if(IsInRecordingFolder(recording.strTitle))
        strTmp = StringUtils::Format("/%s/", recording.strTitle.c_str());
      else
        strTmp = StringUtils::Format("/");

      recording.strDirectory = strTmp;
    }

    strncpy(tag.strDirectory, recording.strDirectory.c_str(), sizeof(tag.strDirectory));
    tag.recordingTime     = recording.startTime;
    tag.iDuration         = recording.iDuration;

    /* TODO: PVR API 5.0.0: Implement this */
    tag.iChannelUid = PVR_CHANNEL_INVALID_UID;

    /* TODO: PVR API 5.1.0: Implement this */
    tag.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

    PVR->TransferRecordingEntry(handle, &tag);
  }
}

std::string Vu::GetRecordingURL(const PVR_RECORDING &recinfo)
{
  for (const auto& recording : m_recordings)
  {
    if (recinfo.strRecordingId == recording.strRecordingId)
      return recording.strStreamURL;
  }
  return "";
}

bool Vu::GetRecordingFromLocation(std::string strRecordingFolder)
{
  std::string url;
  std::string directory;

  if (!strRecordingFolder.compare("default"))
  {
    url = StringUtils::Format("%s%s", m_strURL.c_str(), "web/movielist"); 
    directory = StringUtils::Format("/");
  }
  else 
  {
    url = StringUtils::Format("%s%s?dirname=%s", m_strURL.c_str(), "web/movielist", URLEncodeInline(strRecordingFolder).c_str()); 
    directory = strRecordingFolder;
  }
 
  std::string strXML;
  strXML = GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_DEBUG, "%s Could not find <e2movielist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  if (!pNode)
  {
    XBMC->Log(LOG_DEBUG, "Could not find <e2movie> element");
    return false;
  }
  
  int iNumRecording = 0; 
  
  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2movie"))
  {
    std::string strTmp;
    int iTmp;

    VuRecording recording;

    recording.strDirectory = directory;

    recording.iLastPlayedPosition = 0;
    if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
      recording.strRecordingId = strTmp;

    if (XMLUtils::GetString(pNode, "e2title", strTmp))
      recording.strTitle = strTmp;
    
    if (XMLUtils::GetString(pNode, "e2description", strTmp))
      recording.strPlotOutline = strTmp;

    if (XMLUtils::GetString(pNode, "e2descriptionextended", strTmp))
      recording.strPlot = strTmp;
    
    if (XMLUtils::GetString(pNode, "e2servicename", strTmp))
      recording.strChannelName = strTmp;

    recording.strIconPath = GetChannelIconPath(strTmp.c_str());

    if (XMLUtils::GetInt(pNode, "e2time", iTmp)) 
      recording.startTime = iTmp;

    if (XMLUtils::GetString(pNode, "e2length", strTmp)) 
    {
      iTmp = TimeStringToSeconds(strTmp.c_str());
      recording.iDuration = iTmp;
    }
    else
      recording.iDuration = 0;

    if (XMLUtils::GetString(pNode, "e2filename", strTmp)) 
    {
      strTmp = StringUtils::Format("%sfile?file=%s", m_strURL.c_str(), URLEncodeInline(strTmp).c_str());
      recording.strStreamURL = strTmp;
    }

    m_iNumRecordings++; 
    iNumRecording++;

    m_recordings.push_back(recording);

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, recording.strTitle.c_str(), recording.startTime, recording.iDuration);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u Recording Entries from folder '%s'", __FUNCTION__, iNumRecording, strRecordingFolder.c_str());

  return true;
}

PVR_ERROR Vu::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  std::string strTmp;

  strTmp = StringUtils::Format("web/moviedelete?sRef=%s", URLEncodeInline(recinfo.strRecordingId).c_str());

  std::string strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_FAILED;

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Vu::UpdateTimer(const PVR_TIMER &timer)
{

  XBMC->Log(LOG_DEBUG, "%s timer channelid '%d'", __FUNCTION__, timer.iClientChannelUid);

  std::string strTmp;
  std::string strServiceReference = m_channels.at(timer.iClientChannelUid-1).strServiceReference.c_str();  

  unsigned int i=0;

  while (i<m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }

  VuTimer &oldTimer = m_timers.at(i);
  std::string strOldServiceReference = m_channels.at(oldTimer.iChannelId-1).strServiceReference.c_str();  
  XBMC->Log(LOG_DEBUG, "%s old timer channelid '%d'", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
    iDisabled = 1;

  strTmp = StringUtils::Format("web/timerchange?sRef=%s&begin=%d&end=%d&name=%s&eventID=&description=%s&tags=&afterevent=3&eit=0&disabled=%d&justplay=0&repeated=%d&channelOld=%s&beginOld=%d&endOld=%d&deleteOldOnSave=1", URLEncodeInline(strServiceReference).c_str(), timer.startTime, timer.endTime, URLEncodeInline(timer.strTitle).c_str(), URLEncodeInline(timer.strSummary).c_str(), iDisabled, timer.iWeekdays, URLEncodeInline(strOldServiceReference).c_str(), oldTimer.startTime, oldTimer.endTime  );
  
  std::string strResult;
  if(!SendSimpleCommand(strTmp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();

  return PVR_ERROR_NO_ERROR;
}

long Vu::TimeStringToSeconds(const std::string &timeString)
{
  std::vector<std::string> secs;
  SplitString(timeString, ":", secs);
  int timeInSecs = 0;
  for (unsigned int i = 0; i < secs.size(); i++)
  {
    timeInSecs *= 60;
    timeInSecs += atoi(secs[i].c_str());
  }
  return timeInSecs;
}

int Vu::SplitString(const std::string& input, const std::string& delimiter, std::vector<std::string> &results, unsigned int iMaxStrings)
{
  size_t iPos = -1;
  size_t newPos = -1;
  size_t sizeS2 = delimiter.length();
  size_t isize = input.length();

  results.clear();
  std::vector<unsigned int> positions;

  newPos = input.find (delimiter, 0);

  if ( newPos == std::string::npos )
  {
    results.push_back(input);
    return 1;
  }

  while ( newPos > iPos )
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.find (delimiter, iPos + sizeS2);
  }

  // numFound is the number of delimeters which is one less
  // than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
    numFound = iMaxStrings - 1;

  for ( unsigned int i = 0; i <= numFound; i++ )
  {
    std::string s;
    if ( i == 0 )
    {
      if ( i == numFound )
        s = input;
      else
        s = StringUtils::Mid(input, i, positions[i] );
    }
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == numFound )
          s = StringUtils::Mid(input, offset);
        else if ( i > 0 )
          s = StringUtils::Mid(input, positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    results.push_back(s);
  }
  // return the number of substrings
  return results.size();
}

PVR_ERROR Vu::GetChannelGroups(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for(unsigned int iTagPtr = 0; iTagPtr < m_groups.size(); iTagPtr++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

    tag.bIsRadio     = false;
    tag.iPosition = 0; // groups default order, unused
    strncpy(tag.strGroupName, m_groups[iTagPtr].strGroupName.c_str(), sizeof(tag.strGroupName));

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

unsigned int Vu::GetNumChannelGroups() 
{
  return m_iNumChannelGroups;
}

std::string Vu::GetGroupServiceReference(std::string strGroupName)  
{
  for (int i = 0;i<m_iNumChannelGroups;  i++) 
  {
    VuChannelGroup &myGroup = m_groups.at(i);
    if (!strGroupName.compare(myGroup.strGroupName))
      return myGroup.strServiceReference;
  }
  return "error";
}

PVR_ERROR Vu::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  XBMC->Log(LOG_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);
  std::string strTmp = group.strGroupName;
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    VuChannel &myChannel = m_channels.at(i);
    if (!strTmp.compare(myChannel.strGroupName)) 
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
      tag.iChannelUniqueId = myChannel.iUniqueId;
      tag.iChannelNumber   = myChannel.iChannelNumber;

      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, myChannel.strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName, myChannel.iChannelNumber);

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

bool Vu::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "%s Switching channels", __FUNCTION__);

  if ((int)channel.iUniqueId == m_iCurrentChannel)
    return true;

  m_iCurrentChannel = (int)channel.iUniqueId;

  if (g_bZap)
  {
    // Zapping is set to true, so send the zapping command to the PVR box
    std::string strServiceReference = m_channels.at(channel.iUniqueId-1).strServiceReference.c_str();

    std::string strTmp;
    strTmp = StringUtils::Format("web/zap?sRef=%s", URLEncodeInline(strServiceReference).c_str());

    std::string strResult;
    if(!SendSimpleCommand(strTmp, strResult))
      return false;

  }
  return true;
}

void Vu::SendPowerstate()
{
  if (!g_bSetPowerstate)
    return;
  
  CLockObject lock(m_mutex);
  std::string strTmp;
  strTmp = StringUtils::Format("web/powerstate?newstate=1");

  std::string strResult;
  SendSimpleCommand(strTmp, strResult, true); 
}

bool Vu::GetDeviceInfo()
{
  std::string url; 
  url = StringUtils::Format("%s%s", m_strURL.c_str(), "web/deviceinfo"); 

  std::string strXML;
  strXML = GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(LOG_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    XBMC->Log(LOG_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return false;
  }

  std::string strTmp;;

  XBMC->Log(LOG_NOTICE, "%s - DeviceInfo", __FUNCTION__);

  // Get EnigmaVersion
  if (!XMLUtils::GetString(pElem, "e2enigmaversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2enigmaversion from result!", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  // Get ImageVersion
  if (!XMLUtils::GetString(pElem, "e2imageversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, m_strImageVersion.c_str());

  // Get WebIfVersion
  if (!XMLUtils::GetString(pElem, "e2webifversion", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, m_strWebIfVersion.c_str());

  // Get DeviceName
  if (!XMLUtils::GetString(pElem, "e2devicename", strTmp)) 
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  m_strServerName = strTmp.c_str();
  XBMC->Log(LOG_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, m_strServerName.c_str());

  m_driveSpace.total = m_driveSpace.used = m_driveSpace.lastrefresh = -1;
  ParseDriveSpace(pElem->FirstChildElement("e2hdds"));
  if (m_driveSpace.total > 0) {
	 XBMC->Log(LOG_NOTICE, "%s - Hdd capacity: %d", __FUNCTION__,
	 m_driveSpace.total);
  } else {
	XBMC->Log(LOG_NOTICE, "%s - No hdd found", __FUNCTION__);
  }

  return true;
}

bool Vu::GetDriveSpace(long long *iTotal, long long *iUsed) {
	//initial check has not found a hdd => break
	if (m_driveSpace.total <= 0)
		return false;

	//kodi calls GetDriveSpace() every 3sec => 30sec are enough for a PVR
	if (difftime(time(NULL), m_driveSpace.lastrefresh)
			>= m_driveSpace.interval) {
		std::string url = StringUtils::Format("%sweb/deviceinfo",
				m_strURL.c_str());
		std::string strXML = GetHttpXML(url);

		TiXmlDocument xmlDoc;
		if (!xmlDoc.Parse(strXML.c_str())) {
			XBMC->Log(LOG_ERROR, "Unable to parse XML: %s at line %d",
					xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
			return false;
		}

		TiXmlHandle hDoc(&xmlDoc);
		TiXmlElement* pElem =
				hDoc.FirstChildElement("e2deviceinfo").FirstChildElement(
						"e2hdds").Element();
		if (!pElem) {
			XBMC->Log(LOG_ERROR, "%s Could not find <e2hdds> element",
					__FUNCTION__);
			return false;
		}

		ParseDriveSpace(pElem);
	}

	if (m_driveSpace.used < 0) {
		return false;
	} else {
		*iTotal = m_driveSpace.total;
		*iUsed = m_driveSpace.used;
	}
	return true;
}

void Vu::ParseDriveSpace(TiXmlElement* pElem) {
	if (pElem) {

		//some boxes use usb sticks for caching => multiple entries possible
		for (TiXmlElement* hddElement = pElem->FirstChildElement("e2hdd");
				hddElement != NULL;
				hddElement = hddElement->NextSiblingElement("e2hdd")) {
			std::string strTmp;
			try {
				if (!XMLUtils::GetString(hddElement, "e2capacity", strTmp))
					continue;

				long e2capacity = std::stod(strTmp.c_str()) * 1024 * 1024;

				if (!XMLUtils::GetString(hddElement, "e2free", strTmp))
					continue;

				long e2free = std::stod(strTmp.c_str()) * 1024 * 1024;

				//biggest capacity must be hdd
				if (e2capacity > 0 && e2free >= 0
						&& e2capacity >= m_driveSpace.total) {
					m_driveSpace.total = e2capacity;
					m_driveSpace.used = e2capacity - e2free;
					m_driveSpace.lastrefresh = time(NULL);
					XBMC->Log(LOG_DEBUG,
							"%s - Drivespace refreshed capacity: %d free: %d",
							__FUNCTION__, m_driveSpace.total,
							m_driveSpace.used);
				}
			} catch (...) {
				XBMC->Log(LOG_ERROR, "%s - Conversion error: %s", __FUNCTION__,
						strTmp);
			}
		}
	}
}

const char SAFE[256] =
{
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};


std::string Vu::URLEncodeInline(const std::string& sSrc) 
{
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
  const int SRC_LEN = sSrc.length();
  unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
  unsigned char * pEnd = pStart;
  const unsigned char * const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (SAFE[*pSrc])
      *pEnd++ = *pSrc;
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char *)pStart, (char *)pEnd);
  delete [] pStart;
  return sResult;
}

