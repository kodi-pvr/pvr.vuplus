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

#include "Enigma2.h"

#include <algorithm>
#include <iostream> 
#include <fstream> 
#include <string>
#include <regex>
#include <stdlib.h>

#include "client.h" 
#include "enigma2/utilities/CurlFile.h"
#include "enigma2/utilities/LocalizedString.h"
#include "enigma2/utilities/Logger.h"

#include "util/XMLUtils.h"
#include <p8-platform/util/StringUtils.h>

#if defined(_WIN32)
#include <Bits.h>
#endif

using namespace ADDON;
using namespace P8PLATFORM;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

Enigma2::Enigma2(const Settings &settings) 
  : m_entryExtractor(std::unique_ptr<EpgEntryExtractor>(new EpgEntryExtractor()))
{
  std::string strURL = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((m_settings.GetUsername().length() > 0) && (m_settings.GetPassword().length() > 0))
  {
    strURL = StringUtils::Format("%s:%s@", m_settings.GetUsername().c_str(), m_settings.GetPassword().c_str());
  }
  
  if (!m_settings.GetUseSecureConnection())
    strURL = StringUtils::Format("http://%s%s:%u/", strURL.c_str(), m_settings.GetHostname().c_str(), m_settings.GetWebPortNum());
  else
    strURL = StringUtils::Format("https://%s%s:%u/", strURL.c_str(), m_settings.GetHostname().c_str(), m_settings.GetWebPortNum());
  
  m_strURL = strURL.c_str();

  std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
  m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
  XBMC->WriteFile(m_writeHandle, "Y", 1);
  XBMC->CloseFile(m_writeHandle);
}

Enigma2::~Enigma2() 
{
  CLockObject lock(m_mutex);
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.clear();  
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal timers list...", __FUNCTION__);
  my_timers.ClearTimers();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal recordings list...", __FUNCTION__);
  m_recordings.clear();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal group list...", __FUNCTION__);
  m_groups.clear();
  m_bIsConnected = false;
}

/***************************************************************************
 * Device and helpers
 **************************************************************************/

bool Enigma2::Open()
{
  CLockObject lock(m_mutex);

  Logger::Log(LEVEL_NOTICE, "%s - VU+ Addon Configuration options", __FUNCTION__);
  Logger::Log(LEVEL_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, m_settings.GetHostname().c_str());
  Logger::Log(LEVEL_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, m_settings.GetWebPortNum());
  Logger::Log(LEVEL_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, m_settings.GetStreamPortNum());
  if (!m_settings.GetUseSecureConnection())
    Logger::Log(LEVEL_NOTICE, "%s Use HTTPS: 'false'", __FUNCTION__);
  else
    Logger::Log(LEVEL_NOTICE, "%s Use HTTPS: 'true'", __FUNCTION__);
  
  if ((m_settings.GetUsername().length() > 0) && (m_settings.GetPassword().length() > 0))
  {
    if ((m_settings.GetUsername().find("@") != std::string::npos) || (m_settings.GetPassword().find("@") != std::string::npos))
    {
      Logger::Log(LEVEL_ERROR, "%s - You cannot use the '@' character in either the username or the password with this addon. Please change your configuraton!", __FUNCTION__);
      return false;
    }
  } 
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
  {
    Logger::Log(LEVEL_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
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
  my_timers.TimerUpdates();

  Logger::Log(LEVEL_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 

  return IsRunning(); 
}

void  *Enigma2::Process()
{
  Logger::Log(LEVEL_DEBUG, "%s - starting", __FUNCTION__);

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
      Logger::Log(LEVEL_DEBUG, "%s - Intial EPG update COMPLETE!", __FUNCTION__);
    }
    else
    {
      Logger::Log(LEVEL_DEBUG, "%s - Intial EPG update not completed yet.", __FUNCTION__);
      Sleep(5 * 1000);
    }
  }

  // Trigger "Real" EPG updates 
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Trigger EPG update for channel '%d'", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_channels.at(iChannelPtr).GetUniqueId());
  }

  while(!IsStopped())
  {
    Sleep(5 * 1000);
    m_iUpdateTimer += 5;

    if ((int)m_iUpdateTimer > (m_settings.GetUpdateIntervalMins() * 60)) 
    {
      m_iUpdateTimer = 0;
 
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      Logger::Log(LEVEL_INFO, "%s Perform Updates!", __FUNCTION__);

      if (m_settings.GetAutoTimerListCleanupEnabled()) 
      {
        std::string strTmp;
        strTmp = StringUtils::Format("web/timercleanup?cleanup=true");
        std::string strResult;
        if(!SendSimpleCommand(strTmp, strResult))
          Logger::Log(LEVEL_ERROR, "%s - AutomaticTimerlistCleanup failed!", __FUNCTION__);
      }
      my_timers.TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }

  }

  //CLockObject lock(m_mutex);
  m_started.Broadcast();

  return nullptr;
}

void Enigma2::SendPowerstate()
{
  if (!m_settings.GetDeepStandbyOnAddonExit())
    return;
  
  CLockObject lock(m_mutex);
  std::string strTmp;
  strTmp = StringUtils::Format("web/powerstate?newstate=1");

  std::string strResult;
  SendSimpleCommand(strTmp, strResult, true); 
}

const char * Enigma2::GetServerName() const
{
  return m_strServerName.c_str();  
}

unsigned int Enigma2::GetWebIfVersion() const
{
  return m_iWebIfVersion;
}

bool Enigma2::IsConnected() const
{
  return m_bIsConnected;
}

std::string Enigma2::GetConnectionURL() const
{
  return m_strURL;
}

std::vector<std::string> Enigma2::GetLocations() const
{
  return m_locations;
}

std::string Enigma2::GetHttpXML(const std::string& url) const
{
  Logger::Log(LEVEL_INFO, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  std::string strTmp;

  CurlFile http;
  if(!http.Get(url, strTmp))
  {
    Logger::Log(LEVEL_DEBUG, "%s - Could not open webAPI.", __FUNCTION__);
    return "";
  }

  // If there is no newline add it as it not being there will cause a parse error
  // TODO: Remove once bug is fixed in Open WebIf
  if (strTmp.back() != '\n')
    strTmp += "\n";

  Logger::Log(LEVEL_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());
  
  return strTmp;
}

bool Enigma2::SendSimpleCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult) const
{
  const std::string url = StringUtils::Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 

  std::string strXML = GetHttpXML(url);
  
  if (!bIgnoreResult)
  {

    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);

    pElem = hDoc.FirstChildElement("e2simplexmlresult").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_DEBUG, "%s Could not find <e2simplexmlresult> element!", __FUNCTION__);
      return false;
    }

    bool bTmp;

    if (!XMLUtils::GetBoolean(pElem, "e2state", bTmp)) 
    {
      Logger::Log(LEVEL_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      strResultText = StringUtils::Format("Could not parse e2state!");
      return false;
    }

    if (!XMLUtils::GetString(pElem, "e2statetext", strResultText)) 
    {
      Logger::Log(LEVEL_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
      Logger::Log(LEVEL_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());

    return bTmp;
  }
  return true;
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

std::string Enigma2::URLEncodeInline(const std::string& sSrc) const
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

bool Enigma2::GetGenRepeatTimersEnabled() const
{
  return m_settings.GetGenRepeatTimersEnabled();
}

int Enigma2::GetNumGenRepeatTimers() const
{
  return m_settings.GetNumGenRepeatTimers();
}

bool Enigma2::GetAutoTimersEnabled() const
{
  return m_settings.GetAutotimersEnabled();
}

/***************************************************************************
 * Private Functions
 **************************************************************************/

bool Enigma2::GetDeviceInfo()
{
  const std::string url = StringUtils::Format("%s%s", m_strURL.c_str(), "web/deviceinfo"); 

  const std::string strXML = GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return false;
  }

  std::string strTmp;;

  Logger::Log(LEVEL_NOTICE, "%s - DeviceInfo", __FUNCTION__);

  // Get EnigmaVersion
  if (!XMLUtils::GetString(pElem, "e2enigmaversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2enigmaversion from result!", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  // Get ImageVersion
  if (!XMLUtils::GetString(pElem, "e2imageversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, m_strImageVersion.c_str());

  // Get WebIfVersion
  if (!XMLUtils::GetString(pElem, "e2webifversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  else
  {
    m_strWebIfVersion = strTmp.c_str();
    Logger::Log(LEVEL_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, m_strWebIfVersion.c_str());

    m_iWebIfVersion = GetWebIfVersion(m_strWebIfVersion);
  }

  // Get DeviceName
  if (!XMLUtils::GetString(pElem, "e2devicename", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  m_strServerName = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, m_strServerName.c_str());

  return true;
}

unsigned int Enigma2::GetWebIfVersion(std::string versionString)
{
  unsigned int webIfVersion = 0;

  std::regex regex ("^.*[0-9]+\\.[0-9]+\\.[0-9].*$");
  if (regex_match(versionString, regex))
  {
    int count = 0;
    unsigned int versionPart = 0;
    std::regex pattern("([0-9]+)");
    for (auto i = std::sregex_iterator(versionString.begin(), versionString.end(), pattern); i != std::sregex_iterator(); ++i) 
    {
        switch (count)
        {
          case 0:
            versionPart = atoi(i->str().c_str());
            webIfVersion = versionPart << 16;
            break;
          case 1:
              versionPart = atoi(i->str().c_str());
              webIfVersion |= versionPart << 8;
            break;     
          case 2:
              versionPart = atoi(i->str().c_str());
              webIfVersion |= versionPart;
            break;      
        }      

        count++;
    }
  }

  return webIfVersion;
}

bool Enigma2::LoadChannelGroups() 
{
  std::string strTmp; 

  strTmp = StringUtils::Format("%sweb/getservices", m_strURL.c_str());

  std::string strXML = GetHttpXML(strTmp);  

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2service> element", __FUNCTION__);
    return false;
  }

  m_groups.clear();

  std::string serviceReference;
  std::string groupName;

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    if (!XMLUtils::GetString(pNode, "e2servicereference", serviceReference))
      continue;
    
    // Check whether the current element is not just a label
    if (serviceReference.compare(0,5,"1:64:") == 0)
      continue;

    if (!XMLUtils::GetString(pNode, "e2servicename", groupName)) 
      continue;

    if (m_settings.GetOneGroupOnly() && m_settings.GetOneGroupName() != groupName) 
    {
        Logger::Log(LEVEL_INFO, "%s Only one group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, strTmp.c_str(), m_settings.GetOneGroupName().c_str());
        continue;
    }

    ChannelGroup newGroup;
    newGroup.SetServiceReference(serviceReference);
    newGroup.SetGroupName(groupName);
 
    m_groups.emplace_back(newGroup);

    Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newGroup.GetGroupName().c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channelsgroups", __FUNCTION__, m_groups.size());
  return true;
}

std::string Enigma2::GetGroupServiceReference(std::string strGroupName)  
{
  for (const auto& group : m_groups)
  {
    if (strGroupName == group.GetGroupName())
      return group.GetServiceReference();
  }
  return "error";
}

bool Enigma2::LoadChannels() 
{    
  bool bOk = false;

  m_channels.clear();
  // Load Channels
  for (const auto& group : m_groups)
  {
    if (LoadChannels(group.GetServiceReference(), group.GetGroupName()))
      bOk = true;
  }

  // Load the radio channels - continue if no channels are found 
  std::string strTmp;
  strTmp = StringUtils::Format("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet");
  LoadChannels(strTmp, "radio");

  return bOk;
}

bool Enigma2::LoadChannels(std::string groupServiceReference, std::string groupName) 
{
  Logger::Log(LEVEL_INFO, "%s loading channel group: '%s'", __FUNCTION__, groupName.c_str());

  std::string strTmp;
  strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", m_strURL.c_str(), URLEncodeInline(groupServiceReference).c_str());

  std::string strXML = GetHttpXML(strTmp);  
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2service> element");
    return false;
  }
  
  bool isRadio;
  std::string channelName;
  std::string channelServiceReference;
  std::string iconPath;
  std::string m3uURL;
  std::string streamURL;

  isRadio = (groupName == "radio");

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    if (!XMLUtils::GetString(pNode, "e2servicereference", channelServiceReference))
      continue;
    
    // Check whether the current element is not just a label
    if (channelServiceReference.compare(0,5,"1:64:") == 0)
      continue;

    if (!XMLUtils::GetString(pNode, "e2servicename", channelName)) 
      continue;

    Channel newChannel;
    newChannel.SetRadio(isRadio);
    newChannel.SetServiceReference(channelServiceReference);
    newChannel.SetChannelName(channelName);
    newChannel.SetUniqueId(m_channels.size() + 1);
    newChannel.SetChannelNumber(m_channels.size() + 1);

    Logger::Log(LEVEL_DEBUG, "%s: Loaded Channel: %s, sRef=%s", __FUNCTION__, channelName.c_str(), channelServiceReference.c_str());

    iconPath = channelServiceReference;

    int j = 0;
    std::string::iterator it = iconPath.begin();

    while (j < 10 && it != iconPath.end())
    {
      if (*it == ':')
        j++;

      it++;
    }
    std::string::size_type index = it-iconPath.begin();

    iconPath = iconPath.substr(0, index);

    it = iconPath.end() - 1;
    if (*it == ':')
    {
      iconPath.erase(it);
    }

    if (m_settings.GetUsePiconsEuFormat())
    {
      //Extract the unique part of the icon name and apply the standard pre and post-fix
      std::regex startPrefixRegex ("^\\d+:\\d+:\\d+:");
      std::string replaceWith = "";
      iconPath = regex_replace(iconPath, startPrefixRegex, replaceWith);
      std::regex endPostfixRegex (":\\d+:\\d+:\\d+$");
      iconPath = regex_replace(iconPath, endPostfixRegex, replaceWith);
      iconPath = SERVICE_REF_ICON_PREFIX + iconPath + SERVICE_REF_ICON_POSTFIX;
    }
    
    std::string tempString = StringUtils::Format("%s", iconPath.c_str());

    std::replace(iconPath.begin(), iconPath.end(), ':', '_');
    iconPath = m_settings.GetIconPath().c_str() + iconPath + ".png";

    m3uURL = StringUtils::Format("%s/web/stream.m3u?ref=%s", m_strURL.c_str(), URLEncodeInline(channelServiceReference).c_str());
    newChannel.SetM3uURL(m3uURL);

    streamURL = StringUtils::Format("http://%s:%d/%s", m_settings.GetHostname().c_str(), m_settings.GetStreamPortNum(), tempString.c_str());
    newChannel.SetStreamURL(streamURL);

    if (m_settings.GetUseOnlinePicons())
    {
      std::replace(tempString.begin(), tempString.end(), ':', '_');
      iconPath = StringUtils::Format("%spicon/%s.png", m_strURL.c_str(), tempString.c_str());
    }
    newChannel.SetIconPath(iconPath);

    m_channels.emplace_back(newChannel);
    Logger::Log(LEVEL_INFO, "%s Loaded channel: %s, Icon: %s", __FUNCTION__, newChannel.GetChannelName().c_str(), newChannel.GetIconPath().c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channels", __FUNCTION__, m_channels.size());
  return true;
}

std::string Enigma2::GetChannelIconPath(std::string strChannelName)  
{
  for (const auto& channel : m_channels)
  {
    if (strChannelName == channel.GetChannelName())
      return channel.GetIconPath();
  }
  return "";
}

bool Enigma2::LoadLocations() 
{
  std::string url;
  if (m_settings.GetRecordingsFromCurrentLocationOnly())
    url = StringUtils::Format("%s%s",  m_strURL.c_str(), "web/getcurrlocation"); 
  else 
    url = StringUtils::Format("%s%s",  m_strURL.c_str(), "web/getlocations"); 
 
  const std::string strXML = GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2locations> element");
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2location> element");
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2location"))
  {
    std::string strTmp;
    strTmp = pNode->GetText();

    m_locations.emplace_back(strTmp);

    Logger::Log(LEVEL_DEBUG, "%s Added '%s' as a recording location", __FUNCTION__, strTmp.c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded '%d' recording locations", __FUNCTION__, m_locations.size());

  return true;
}

/**
  * GetStreamURL() reads out a stream-URL from a M3U-file.
  *
  * This method downloads a M3U-file from the address that is given by strM3uURL.
  * It returns the first line that starts with "http".
  * If no line starts with "http" the last line is returned.
  */
std::string Enigma2::GetStreamURL(const std::string& strM3uURL)
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

long Enigma2::TimeStringToSeconds(const std::string &timeString)
{
  std::vector<std::string> tokens;

  std::string s = timeString;
  std::string delimiter = ":";

  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) 
  {
    token = s.substr(0, pos);
    tokens.emplace_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.emplace_back(s);

  int timeInSecs = 0;

  if (tokens.size() == 2)
  {
    timeInSecs += atoi(tokens[0].c_str()) * 60;
    timeInSecs += atoi(tokens[1].c_str());
  }

  return timeInSecs;
}

std::string& Enigma2::Escape(std::string &s, std::string from, std::string to)
{ 
  std::string::size_type pos = -1;
  while ( (pos = s.find(from, pos+1) ) != std::string::npos)         
    s.erase(pos, from.length()).insert(pos, to);        

  return s;     
} 

bool Enigma2::IsInRecordingFolder(std::string strRecordingFolder)
{
  int iMatches = 0;
  for (const auto& recording : m_recordings)
  {
    if (strRecordingFolder == recording.GetTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found Recording title '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __FUNCTION__, strRecordingFolder.c_str());
        return true;    
      }
    }
  }

  return false;
}

void Enigma2::TransferRecordings(ADDON_HANDLE handle)
{
  for (auto& recording : m_recordings)
  {
    std::string strTmp;
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.GetRecordingId().c_str(), sizeof(tag.strRecordingId));
    strncpy(tag.strTitle, recording.GetTitle().c_str(), sizeof(tag.strTitle));
    strncpy(tag.strPlotOutline, recording.GetPlotOutline().c_str(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, recording.GetPlot().c_str(), sizeof(tag.strPlot));
    strncpy(tag.strChannelName, recording.GetChannelName().c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strIconPath, recording.GetIconPath().c_str(), sizeof(tag.strIconPath));

    if (!m_settings.GetKeepRecordingsFolders())
    {
      if(IsInRecordingFolder(recording.GetTitle()))
        strTmp = StringUtils::Format("/%s/", recording.GetTitle().c_str());
      else
        strTmp = StringUtils::Format("/");

      recording.SetDirectory(strTmp);
    }

    strncpy(tag.strDirectory, recording.GetDirectory().c_str(), sizeof(tag.strDirectory));
    tag.recordingTime     = recording.GetStartTime();
    tag.iDuration         = recording.GetDuration();

    
    tag.iChannelUid = PVR_CHANNEL_INVALID_UID;
    tag.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

    for (const auto& channel : m_channels)
    {
      if (recording.GetChannelName() == channel.GetChannelName())
      {
        /* PVR API 5.0.0: iChannelUid in recordings */
        tag.iChannelUid = channel.GetUniqueId();

        /* PVR API 5.1.0: Support channel type in recordings */
        if (channel.IsRadio())
          tag.channelType = PVR_RECORDING_CHANNEL_TYPE_RADIO;
        else
          tag.channelType = PVR_RECORDING_CHANNEL_TYPE_TV;
      }
    }

    tag.iSeriesNumber = recording.GetSeasonNumber();
    tag.iEpisodeNumber = recording.GetEpisodeNumber();
    tag.iYear = recording.GetYear();

    PVR->TransferRecordingEntry(handle, &tag);
  }
}

/***************************************************************************
 * Channel Groups
 **************************************************************************/

unsigned int Enigma2::GetNumChannelGroups() const
{
  return m_groups.size();
}

PVR_ERROR Enigma2::GetChannelGroups(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for (const auto& group : m_groups)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

    tag.bIsRadio     = false;
    tag.iPosition = 0; // groups default order, unused
    strncpy(tag.strGroupName, group.GetGroupName().c_str(), sizeof(tag.strGroupName));

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  Logger::Log(LEVEL_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);
  std::string strTmp = group.strGroupName;
  for (const auto& channel : m_channels)
  {
    if (strTmp == channel.GetGroupName()) 
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
      tag.iChannelUniqueId = channel.GetUniqueId();
      tag.iChannelNumber   = channel.GetChannelNumber();

      Logger::Log(LEVEL_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, channel.GetChannelName().c_str(), tag.iChannelUniqueId, group.strGroupName, channel.GetChannelNumber());

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Channels
 **************************************************************************/

int Enigma2::GetChannelsAmount() const
{
  return m_channels.size();
}

int Enigma2::GetChannelNumber(std::string strServiceReference) const
{
  for (const auto& channel : m_channels)
  {
    if (strServiceReference == channel.GetServiceReference())
      return channel.GetChannelNumber();
  }
  return -1;
}

std::vector<Channel> Enigma2::GetChannels() const
{
  return m_channels;
}

PVR_ERROR Enigma2::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  for (const auto& channel : m_channels)
  {
    if (channel.IsRadio() == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.GetUniqueId();
      xbmcChannel.bIsRadio          = channel.IsRadio();
      xbmcChannel.iChannelNumber    = channel.GetChannelNumber();
      strncpy(xbmcChannel.strChannelName, channel.GetChannelName().c_str(), sizeof(xbmcChannel.strChannelName));
      strncpy(xbmcChannel.strInputFormat, "", 0); // unused
      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.bIsHidden         = false;
      strncpy(xbmcChannel.strIconPath, channel.GetIconPath().c_str(), sizeof(xbmcChannel.strIconPath));

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * EPG
 **************************************************************************/

bool Enigma2::GetInitialEPGForGroup(ChannelGroup &group)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  const std::string url = StringUtils::Format("%s%s%s",  m_strURL.c_str(), "web/epgnownext?bRef=",  URLEncodeInline(group.GetServiceReference()).c_str());
 
  const std::string strXML = GetHttpXML(url);

  int iNumEPG = 0;
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }
  
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2event"))
  {
    std::string strTmp;

    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart)) 
      continue;

    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
      continue;

    EPGEntry entry;
    entry.SetStartTime(iTmpStart);
    entry.SetEndTime(iTmpStart + iTmp);

    if (!XMLUtils::GetInt(pNode, "e2eventid", iTmp))  
      continue;
    entry.SetEventId(iTmp);

    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.SetTitle(strTmp);

    if(!XMLUtils::GetString(pNode, "e2eventservicereference", strTmp))
      continue;

    // Check whether the current element is not just a label or that it's not an empty record
    if (strTmp.compare(0,5,"1:64:") == 0 || (entry.GetEventId() == 0 && entry.GetTitle() == "None"))
      continue;

    entry.SetServiceReference(strTmp);
    
    entry.SetChannelId(GetChannelNumber(entry.GetServiceReference().c_str()));

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.SetPlot(strTmp);

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
      entry.SetPlotOutline(strTmp);

    // Some providers only use PlotOutline (e.g. freesat) and Kodi does not display it, if this is the case swap them
    if (entry.GetPlot().empty())
    {
      entry.SetPlot(entry.GetPlotOutline());
      entry.SetPlotOutline("");
    }
    else if ((m_settings.GetPrependOutline() == PrependOutline::IN_EPG || m_settings.GetPrependOutline() == PrependOutline::ALWAYS)
              && !entry.GetPlotOutline().empty())
    {
      entry.SetPlot(entry.GetPlotOutline() + "\n" + entry.GetPlot());
      entry.SetPlotOutline("");
    }    

    if (m_settings.GetExtractExtraEpgInfo())
      m_entryExtractor->ExtractFromEntry(entry);

    iNumEPG++; 
    
    group.GetInitialEPG().emplace_back(entry);
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group.GetGroupName().c_str());
  return true;
}

PVR_ERROR Enigma2::GetInitialEPGForChannel(ADDON_HANDLE handle, const Channel &channel, time_t iStart, time_t iEnd)
{
  if (m_groups.size() < 1)
    return PVR_ERROR_SERVER_ERROR;

  if (channel.IsRadio())
  {
    Logger::Log(LEVEL_DEBUG, "%s Channel '%s' is a radio channel so no Initial EPG", __FUNCTION__, channel.GetChannelName().c_str());
    return PVR_ERROR_NO_ERROR;
  }

  Logger::Log(LEVEL_DEBUG, "%s Checking for initialEPG for group '%s', num groups %d, channel %s", __FUNCTION__, channel.GetGroupName().c_str(), m_groups.size(), channel.GetChannelName().c_str());

  bool retrievedInitialEPGForGroup = false;
  ChannelGroup *myGroupPtr = nullptr;
  for (auto& group : m_groups)
  {
    Logger::Log(LEVEL_DEBUG, "%s Looking for channel %s group %s",  __FUNCTION__, channel.GetChannelName().c_str(), channel.GetGroupName().c_str());
    if (group.GetGroupName() == channel.GetGroupName())
    {
      myGroupPtr = &group;

      if (myGroupPtr->GetInitialEPG().size() == 0)
      {
        Logger::Log(LEVEL_DEBUG, "%s Fetching initialEPG for group '%s'", __FUNCTION__, channel.GetGroupName().c_str());
        retrievedInitialEPGForGroup = GetInitialEPGForGroup(group);
      }
      break;
    }
  }

  if (!myGroupPtr)
  {
    Logger::Log(LEVEL_DEBUG, "%s No group found for channel '%s' group '%s' sp no Initial EPG",  __FUNCTION__, channel.GetChannelName().c_str(), channel.GetGroupName().c_str());
    return PVR_ERROR_NO_ERROR;
  }

  if (retrievedInitialEPGForGroup)
    Logger::Log(LEVEL_DEBUG, "%s InitialEPG size for group '%s' is now '%d'", __FUNCTION__, channel.GetGroupName().c_str(), myGroupPtr->GetInitialEPG().size());
  else
    Logger::Log(LEVEL_DEBUG, "%s Already have initialEPG for group '%s', it's size is '%d'", __FUNCTION__, channel.GetGroupName().c_str(), myGroupPtr->GetInitialEPG().size());

  for (const auto& entry : myGroupPtr->GetInitialEPG())
  {
    if (channel.GetServiceReference() == entry.GetServiceReference()) 
    {
      EPG_TAG broadcast;
      memset(&broadcast, 0, sizeof(EPG_TAG));

      entry.UpdateTo(broadcast);

      PVR->TransferEpgEntry(handle, &broadcast);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

bool Enigma2::CheckIfAllChannelsHaveInitialEPG() const
{
  bool someChannelsStillNeedInitialEPG = false;
  for (const auto& channel : m_channels)
  {
    if (channel.IsRequiresInitialEPG()) 
    {
      someChannelsStillNeedInitialEPG = true;
    }
  }

  return !someChannelsStillNeedInitialEPG;
}

PVR_ERROR Enigma2::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
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
    Logger::Log(LEVEL_ERROR, "%s Could not fetch channel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  Channel& myChannel = m_channels.at(channel.iUniqueId-1);

  Logger::Log(LEVEL_DEBUG, "%s Getting EPG for channel '%s'", __FUNCTION__, myChannel.GetChannelName().c_str());

  // Check if the initial short import has already been done for this channel
  if (myChannel.IsRequiresInitialEPG())
  {
    myChannel.SetRequiresInitialEPG(false);

    if (!m_bAllChannelsHaveInitialEPG)
      m_bAllChannelsHaveInitialEPG = CheckIfAllChannelsHaveInitialEPG();

    if (m_bAllChannelsHaveInitialEPG)
    {
      std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
      m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
      XBMC->WriteFile(m_writeHandle, "N", 1);
      XBMC->CloseFile(m_writeHandle);
    }

    return GetInitialEPGForChannel(handle, myChannel, iStart, iEnd);
  }

  const std::string url = StringUtils::Format("%s%s%s",  m_strURL.c_str(), "web/epgservice?sRef=",  URLEncodeInline(myChannel.GetServiceReference()).c_str());
 
  const std::string strXML = GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2eventlist").Element();
 
  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_NO_ERROR;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2event> element");
    // RETURN "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_SERVER_ERROR;
  }
  
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2event"))
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
    
    EPGEntry entry;
    entry.SetStartTime(iTmpStart);
    entry.SetEndTime(iTmpStart + iTmp);

    if (!XMLUtils::GetInt(pNode, "e2eventid", iTmp))  
      continue;

    entry.SetEventId(iTmp);
    entry.SetChannelId(channel.iUniqueId);
    
    if(!XMLUtils::GetString(pNode, "e2eventtitle", strTmp))
      continue;

    entry.SetTitle(strTmp);
    
    entry.SetServiceReference(myChannel.GetServiceReference().c_str());

    // Check that it's not an empty record
    if (entry.GetEventId() == 0 && entry.GetTitle() == "None")
      continue;

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTmp))
      entry.SetPlot(strTmp);

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTmp))
       entry.SetPlotOutline(strTmp);

    // Some providers only use PlotOutline (e.g. freesat) and Kodi does not display it, if this is the case swap them
    if (entry.GetPlot().empty())
    {
      entry.SetPlot(entry.GetPlotOutline());
      entry.SetPlotOutline("");
    }
    else if ((m_settings.GetPrependOutline() == PrependOutline::IN_EPG || m_settings.GetPrependOutline() == PrependOutline::ALWAYS)
              && !entry.GetPlotOutline().empty())
    {
      entry.SetPlot(entry.GetPlotOutline() + "\n" + entry.GetPlot());
      entry.SetPlotOutline("");
    }    

    if (m_settings.GetExtractExtraEpgInfo())
      m_entryExtractor->ExtractFromEntry(entry);

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));

    entry.UpdateTo(broadcast);

    PVR->TransferEpgEntry(handle, &broadcast);

    iNumEPG++; 

    Logger::Log(LEVEL_DEBUG, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.GetChannelId(), entry.GetStartTime(), entry.GetEndTime());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Livestream
 **************************************************************************/
bool Enigma2::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  Logger::Log(LEVEL_DEBUG, "%s: channel=%u", __FUNCTION__, channelinfo.iUniqueId);
  CLockObject lock(m_mutex);

  if (channelinfo.iUniqueId != m_iCurrentChannel)
  {
    m_iCurrentChannel = channelinfo.iUniqueId;

    if (m_settings.GetZapBeforeChannelSwitch())
    {
      // Zapping is set to true, so send the zapping command to the PVR box
      std::string strServiceReference = m_channels.at(channelinfo.iUniqueId-1).GetServiceReference().c_str();

      std::string strTmp;
      strTmp = StringUtils::Format("web/zap?sRef=%s", URLEncodeInline(strServiceReference).c_str());

      std::string strResult;
      if(!SendSimpleCommand(strTmp, strResult))
        return false;

    }
  }
  return true;
}

void Enigma2::CloseLiveStream(void)
{
  CLockObject lock(m_mutex);
  m_iCurrentChannel = -1;
}

const std::string Enigma2::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  if (m_settings.GetAutoConfigLiveStreamsEnabled())
  {
    // we need to download the M3U file that contains the URL for the stream...
    // we do it here for 2 reasons:
    //  1. This is faster than doing it during initialization
    //  2. The URL can change, so this is more up-to-date.
    return GetStreamURL(m_channels.at(channelinfo.iUniqueId - 1).GetM3uURL());
  }

  return m_channels.at(channelinfo.iUniqueId - 1).GetStreamURL();
}

/***************************************************************************
 * Recordings
 **************************************************************************/

unsigned int Enigma2::GetRecordingsAmount() {
  return m_recordings.size();
}

PVR_ERROR Enigma2::GetRecordings(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }

  m_recordings.clear();

  for (const auto& location : m_locations)
  {
    if (!GetRecordingFromLocation(location))
    {
      Logger::Log(LEVEL_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, location.c_str());
    }
  }

  TransferRecordings(handle);

  return PVR_ERROR_NO_ERROR;
}

bool Enigma2::GetRecordingFromLocation(std::string strRecordingFolder)
{
  std::string url;
  std::string directory;

  if (strRecordingFolder == "default")
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
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2movielist> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  int iNumRecording = 0; 

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2movie> element, no movies at location: %s", directory.c_str());
  }  
  else
  {  
    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2movie"))
    {
      std::string strTmp;
      int iTmp;

      RecordingEntry recording;

      recording.SetDirectory(directory);

      recording.SetLastPlayedPosition(0);
      if (XMLUtils::GetString(pNode, "e2servicereference", strTmp))
        recording.SetRecordingId(strTmp);

      if (XMLUtils::GetString(pNode, "e2title", strTmp))
        recording.SetTitle(strTmp);
      
      if (XMLUtils::GetString(pNode, "e2description", strTmp))
        recording.SetPlotOutline(strTmp);

      if (XMLUtils::GetString(pNode, "e2descriptionextended", strTmp))
        recording.SetPlot(strTmp);
      
      if (XMLUtils::GetString(pNode, "e2servicename", strTmp))
        recording.SetChannelName(strTmp);

      recording.SetIconPath(GetChannelIconPath(strTmp.c_str()));

      if (XMLUtils::GetInt(pNode, "e2time", iTmp)) 
        recording.SetStartTime(iTmp);

      if (XMLUtils::GetString(pNode, "e2length", strTmp)) 
      {
        iTmp = TimeStringToSeconds(strTmp.c_str());
        recording.SetDuration(iTmp);
      }
      else
        recording.SetDuration(0);

      if (XMLUtils::GetString(pNode, "e2filename", strTmp)) 
      {
        strTmp = StringUtils::Format("%sfile?file=%s", m_strURL.c_str(), URLEncodeInline(strTmp).c_str());
        recording.SetStreamURL(strTmp);
      }

      // Some providers only use PlotOutline (e.g. freesat) and Kodi does not display it, if this is the case swap them
      if (recording.GetPlot().empty())
      {
        recording.SetPlot(recording.GetPlotOutline());
        recording.SetPlotOutline("");
      }
      else if ((m_settings.GetPrependOutline() == PrependOutline::IN_RECORDINGS || m_settings.GetPrependOutline() == PrependOutline::ALWAYS)
                && !recording.GetPlotOutline().empty())
      {
        recording.SetPlot(recording.GetPlotOutline() + "\n" + recording.GetPlot());
        recording.SetPlotOutline("");
      }    


      if (m_settings.GetExtractExtraEpgInfo())
        m_entryExtractor->ExtractFromEntry(recording);

      iNumRecording++;

      m_recordings.emplace_back(recording);

      Logger::Log(LEVEL_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, recording.GetTitle().c_str(), recording.GetStartTime(), recording.GetDuration());
    }

    Logger::Log(LEVEL_INFO, "%s Loaded %u Recording Entries from folder '%s'", __FUNCTION__, iNumRecording, strRecordingFolder.c_str());
  }
  return true;
}

std::string Enigma2::GetRecordingURL(const PVR_RECORDING &recinfo)
{
  for (const auto& recording : m_recordings)
  {
    if (recinfo.strRecordingId == recording.GetRecordingId())
      return recording.GetStreamURL();
  }
  return "";
}

std::string Enigma2::GetRecordingPath() const
{
  return m_settings.GetRecordingPath();
}

PVR_ERROR Enigma2::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  std::string strTmp;

  strTmp = StringUtils::Format("web/moviedelete?sRef=%s", URLEncodeInline(recinfo.strRecordingId).c_str());

  std::string strResult;
  if(!SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_FAILED;

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

RecordingReader *Enigma2::OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  CLockObject lock(m_mutex);
  std::time_t now = std::time(nullptr), end = 0;
  std::string channelName = recinfo.strChannelName;
  auto timer = my_timers.GetTimer([&](const Timer &timer)
      {
        return timer.isRunning(&now, &channelName);
      });
  if (timer)
    end = timer->GetEndTime();

  return new RecordingReader(GetRecordingURL(recinfo).c_str(), end);
}

/***************************************************************************
 * Timers
 **************************************************************************/
void Enigma2::GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  std::vector<PVR_TIMER_TYPE> timerTypes;
  {
    CLockObject lock(m_mutex);
    my_timers.GetTimerTypes(timerTypes);
  }

  int i = 0;
  for (auto &timerType : timerTypes)
    types[i++] = timerType;
  *size = timerTypes.size();
  Logger::Log(LEVEL_NOTICE, "Transfered %u timer types", *size);
}

int Enigma2::GetTimersAmount()
{
  CLockObject lock(m_mutex);
  return my_timers.GetTimerCount();
}

PVR_ERROR Enigma2::GetTimers(ADDON_HANDLE handle)
{
  // is the addon is currently updating the channels, then delay the call
  unsigned int iTimer = 0;
  while(m_bUpdating == true && iTimer < 120)
  {
    Sleep(1000);
    iTimer++;
  }
  
  std::vector<PVR_TIMER> timers;
  {
    CLockObject lock(m_mutex);
    my_timers.GetTimers(timers);
    my_timers.GetAutoTimers(timers);
  }

  Logger::Log(LEVEL_DEBUG, "%s - timers available '%d'", __FUNCTION__, timers.size());

  for (auto &timer : timers)
    PVR->TransferTimerEntry(handle, &timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::AddTimer(const PVR_TIMER &timer)
{
  return my_timers.AddTimer(timer);
}

PVR_ERROR Enigma2::UpdateTimer(const PVR_TIMER &timer)
{
  return my_timers.UpdateTimer(timer);
}

PVR_ERROR Enigma2::DeleteTimer(const PVR_TIMER &timer)
{
  return my_timers.DeleteTimer(timer);
}

PVR_ERROR Enigma2::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  long long totalKb = 0;
  long long freeKb = 0;

  const std::string url = StringUtils::Format("%s%s", m_strURL.c_str(), "web/deviceinfo"); 

  const std::string strXML = GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2hdds").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2hdds> element");
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlElement* hddNode = pNode->FirstChildElement("e2hdd");

  if (!hddNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2hdd> element");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (; hddNode != nullptr; hddNode = hddNode->NextSiblingElement("e2hdd"))
  {
    std::string capacity;
    std::string freeSpace;

    XMLUtils::GetString(hddNode, "e2capacity", capacity);
    XMLUtils::GetString(hddNode, "e2free", freeSpace);

    totalKb += GetKbFromString(capacity);
    freeKb += GetKbFromString(freeSpace);
  }

  *iTotal = totalKb;
  *iUsed = totalKb - freeKb;

  Logger::Log(LEVEL_INFO, "GetDriveSpace Total: %lld, Used %lld", *iTotal, *iUsed);

  return PVR_ERROR_NO_ERROR;
}

long long Enigma2::GetKbFromString(const std::string &stringInMbGbTb) const
{
  long long sizeInKb = 0;

  static const std::vector<std::string> sizes = {"MB", "GB", "TB"};
  long multiplier = 1024 * 1024;
  std::string replaceWith = "";
  for (const std::string& size : sizes)
  {
    std::regex regexSize ("^.* " + size);
    std::regex regexReplaceSize (" " + size);

    if (regex_match(stringInMbGbTb, regexSize))
    {
      double sizeValue = atof(regex_replace(stringInMbGbTb, regexReplaceSize, replaceWith).c_str());

      sizeInKb += static_cast<long long>(sizeValue * multiplier);

      break;
    }

    multiplier = multiplier * 1024;
  }

  return sizeInKb;
}