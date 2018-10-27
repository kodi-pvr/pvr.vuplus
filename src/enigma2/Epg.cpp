#include "Epg.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

Epg::Epg (enigma2::Channels &channels, enigma2::ChannelGroups &channelGroups, enigma2::extract::EpgEntryExtractor &entryExtractor)
      : m_channels(channels), m_channelGroups(channelGroups), m_entryExtractor(entryExtractor)
{
  InitialiseEpgReadyFile();
}

void Epg::InitialiseEpgReadyFile()
{
  m_writeHandle = XBMC->OpenFileForWrite(INITIAL_EPG_READY_FILE.c_str(), true);
  XBMC->WriteFile(m_writeHandle, "Y", 1);
  XBMC->CloseFile(m_writeHandle);  
}

bool Epg::IsInitialEpgCompleted()
{
  m_readHandle = XBMC->OpenFile(INITIAL_EPG_READY_FILE.c_str(), 0);
  char buf[1];
  XBMC->ReadFile(m_readHandle, buf, 1);
  XBMC->CloseFile(m_readHandle);
  char buf2[] = { "N" };
  if (buf[0] == buf2[0])
  {
    Logger::Log(LEVEL_DEBUG, "%s - Intial EPG update COMPLETE!", __FUNCTION__);
    return true;
  }
  else
  {
    Logger::Log(LEVEL_DEBUG, "%s - Intial EPG update not completed yet.", __FUNCTION__);
    return false;
  }
}

void Epg::TriggerEpgUpdatesForChannels()
{
  for (auto& channel : m_channels.GetChannelsList())
  {
    Logger::Log(LEVEL_DEBUG, "%s - Trigger EPG update for channel '%d'", __FUNCTION__, channel.GetUniqueId());
    PVR->TriggerEpgUpdate(channel.GetUniqueId());
  }  
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!m_channels.IsValid(channel.iUniqueId))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not fetch channel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  Channel& myChannel = m_channels.GetChannel(channel.iUniqueId);

  Logger::Log(LEVEL_DEBUG, "%s Getting EPG for channel '%s'", __FUNCTION__, myChannel.GetChannelName().c_str());

  // Check if the initial short import has already been done for this channel
  if (myChannel.IsRequiresInitialEPG())
  {
    myChannel.SetRequiresInitialEPG(false);

    if (!m_allChannelsHaveInitialEPG)
      m_allChannelsHaveInitialEPG = m_channels.CheckIfAllChannelsHaveInitialEPG();

    if (m_allChannelsHaveInitialEPG)
    {
      std::string initialEPGReady = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";
      m_writeHandle = XBMC->OpenFileForWrite(initialEPGReady.c_str(), true);
      XBMC->WriteFile(m_writeHandle, "N", 1);
      XBMC->CloseFile(m_writeHandle);
    }

    return GetInitialEPGForChannel(handle, myChannel, iStart, iEnd);
  }

  const std::string url = StringUtils::Format("%s%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/epgservice?sRef=",  
                                              WebUtils::URLEncodeInline(myChannel.GetServiceReference()).c_str());
 
  const std::string strXML = WebUtils::GetHttpXML(url);

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
    EpgEntry entry;

    if (!entry.UpdateFrom(pNode, myChannel, iStart, iEnd))
      continue;

    if (m_entryExtractor.IsEnabled())
      m_entryExtractor.ExtractFromEntry(entry);

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

bool Epg::GetInitialEPGForGroup(ChannelGroup &group)
{
  const std::string url = StringUtils::Format("%s%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/epgnownext?bRef=",  
                                                WebUtils::URLEncodeInline(group.GetServiceReference()).c_str());
 
  const std::string strXML = WebUtils::GetHttpXML(url);

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
    EpgEntry entry;

    if (!entry.UpdateFrom(pNode, m_channels))
      continue;

    if (m_entryExtractor.IsEnabled())
      m_entryExtractor.ExtractFromEntry(entry);

    iNumEPG++; 
    
    group.GetInitialEPG().emplace_back(entry);
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group.GetGroupName().c_str());
  return true;
}

PVR_ERROR Epg::GetInitialEPGForChannel(ADDON_HANDLE handle, const Channel &channel, time_t iStart, time_t iEnd)
{
  if (m_channelGroups.GetNumChannelGroups() < 1)
    return PVR_ERROR_SERVER_ERROR;

  if (channel.IsRadio())
  {
    Logger::Log(LEVEL_DEBUG, "%s Channel '%s' is a radio channel so no Initial EPG", __FUNCTION__, channel.GetChannelName().c_str());
    return PVR_ERROR_NO_ERROR;
  }

  Logger::Log(LEVEL_DEBUG, "%s Checking for initialEPG for group '%s', num groups %d, channel %s", __FUNCTION__, channel.GetGroupName().c_str(), m_channelGroups.GetNumChannelGroups(), channel.GetChannelName().c_str());

  bool retrievedInitialEPGForGroup = false;
  ChannelGroup *myGroupPtr = nullptr;
  for (auto& group : m_channelGroups.GetChannelGroupsList())
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
