#include "Epg.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <nlohmann/json.hpp>
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using json = nlohmann::json;

Epg::Epg (enigma2::Channels &channels, enigma2::ChannelGroups &channelGroups, enigma2::extract::EpgEntryExtractor &entryExtractor)
      : m_channels(channels), m_channelGroups(channelGroups), m_entryExtractor(entryExtractor) {}

bool Epg::IsInitialEpgCompleted()
{
  if (m_allChannelsHaveInitialEPG)
  {
    return m_allChannelsHaveInitialEPG;
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
    Logger::Log(LEVEL_DEBUG, "%s - Trigger EPG update for channel '%d'", __FUNCTION__, channel->GetUniqueId());
    PVR->TriggerEpgUpdate(channel->GetUniqueId());
  }
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!m_channels.IsValid(channel.iUniqueId))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not fetch channel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  std::shared_ptr<Channel> myChannel = m_channels.GetChannel(channel.iUniqueId);

  Logger::Log(LEVEL_DEBUG, "%s Getting EPG for channel '%s'", __FUNCTION__, myChannel->GetChannelName().c_str());

  // Check if the initial short import has already been done for this channel
  if (myChannel->RequiresInitialEPG())
  {
    myChannel->SetRequiresInitialEPG(false);

    if (!m_allChannelsHaveInitialEPG)
      m_allChannelsHaveInitialEPG = m_channels.CheckIfAllChannelsHaveInitialEPG();

    if (m_allChannelsHaveInitialEPG)
      Logger::Log(LEVEL_DEBUG, "%s - Intial EPG update COMPLETE!", __FUNCTION__);

    return GetInitialEPGForChannel(handle, myChannel, iStart, iEnd);
  }

  const std::string url = StringUtils::Format("%s%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/epgservice?sRef=",
                                              WebUtils::URLEncodeInline(myChannel->GetServiceReference()).c_str());

  const std::string strXML = WebUtils::GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2eventlist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_NOTICE, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return PVR_ERROR_NO_ERROR;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_NOTICE, "%s Could not find <e2event> element", __FUNCTION__);
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

    Logger::Log(LEVEL_TRACE, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.GetChannelId(), entry.GetStartTime(), entry.GetEndTime());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

bool Epg::GetInitialEPGForGroup(std::shared_ptr<ChannelGroup> group)
{
  const std::string url = StringUtils::Format("%s%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/epgnownext?bRef=",
                                                WebUtils::URLEncodeInline(group->GetServiceReference()).c_str());

  const std::string strXML = WebUtils::GetHttpXML(url);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2eventlist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_NOTICE, "%s could not find <e2eventlist> element!", __FUNCTION__);
    // Return "NO_ERROR" as the EPG could be empty for this channel
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2event> element", __FUNCTION__);
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

    group->GetInitialEPG().emplace_back(entry);
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %u EPG Entries for group '%s'", __FUNCTION__, iNumEPG, group->GetGroupName().c_str());
  return true;
}

PVR_ERROR Epg::GetInitialEPGForChannel(ADDON_HANDLE handle, const std::shared_ptr<Channel> &channel, time_t iStart, time_t iEnd)
{
  if (m_channelGroups.GetNumChannelGroups() < 1)
    return PVR_ERROR_SERVER_ERROR;

  if (channel->IsRadio())
  {
    Logger::Log(LEVEL_DEBUG, "%s Channel '%s' is a radio channel so no Initial EPG", __FUNCTION__, channel->GetChannelName().c_str());
    return PVR_ERROR_NO_ERROR;
  }

  std::shared_ptr<ChannelGroup> myGroupPtr = nullptr;
  for (auto& group : channel->GetChannelGroupList())
  {
    bool retrievedInitialEPGForGroup = false;

    Logger::Log(LEVEL_DEBUG, "%s Checking for initialEPG for group '%s', num groups %d, channel %s", __FUNCTION__, group->GetGroupName().c_str(), m_channelGroups.GetNumChannelGroups(), channel->GetChannelName().c_str());

    myGroupPtr = group;

    if (group->GetInitialEPG().size() == 0)
    {
      Logger::Log(LEVEL_DEBUG, "%s Fetching initialEPG for group '%s'", __FUNCTION__, group->GetGroupName().c_str());
      retrievedInitialEPGForGroup = GetInitialEPGForGroup(group);
    }

    if (retrievedInitialEPGForGroup)
      Logger::Log(LEVEL_DEBUG, "%s InitialEPG size for group '%s' is now '%d'", __FUNCTION__, group->GetGroupName().c_str(), myGroupPtr->GetInitialEPG().size());
    else
      Logger::Log(LEVEL_DEBUG, "%s Already have initialEPG for group '%s', it's size is '%d'", __FUNCTION__, group->GetGroupName().c_str(), myGroupPtr->GetInitialEPG().size());
  }

  if (!myGroupPtr)
  {
    Logger::Log(LEVEL_DEBUG, "%s No groups found for channel '%s' so no Initial EPG",  __FUNCTION__, channel->GetChannelName().c_str());
    return PVR_ERROR_NO_ERROR;
  }

  for (const auto& entry : myGroupPtr->GetInitialEPG())
  {
    if (channel->GetServiceReference() == entry.GetServiceReference())
    {
      EPG_TAG broadcast;
      memset(&broadcast, 0, sizeof(EPG_TAG));

      entry.UpdateTo(broadcast);

      PVR->TransferEpgEntry(handle, &broadcast);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

std::string Epg::LoadEPGEntryShortDescription(const std::string &serviceReference, unsigned int epgUid)
{
  std::string shortDescription;

  const std::string jsonUrl = StringUtils::Format("%sapi/event?sref=%s&idev=%u", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(serviceReference).c_str(), epgUid);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["event"].empty())
    {
      for (const auto& element : jsonDoc["event"].items())
      {
        if (element.key() == "shortdesc")
        {
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event short description for sref: %s, epgId: %u - '%s'", __FUNCTION__, serviceReference.c_str(), epgUid, element.value().get<std::string>().c_str());
          shortDescription = element.value().get<std::string>();
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load short descrption from OpenWebIf for sref: %s, epgId: %u - JSON parse error - message: %s, exception id: %d", __FUNCTION__, serviceReference.c_str(), epgUid, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return shortDescription;
}

EpgPartialEntry Epg::LoadEPGEntryPartialDetails(const std::string &serviceReference, time_t startTime)
{
  EpgPartialEntry partialEntry;

  const std::string jsonUrl = StringUtils::Format("%sapi/epgservice?sRef=%s&time=%ld&endTime=1", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(serviceReference).c_str(), startTime);

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["events"].empty())
    {
      for (const auto& event : jsonDoc["events"].items())
      {
        for (const auto& element : event.value().items())
        {
          if (element.key() == "shortdesc")
            partialEntry.shortDescription = element.value().get<std::string>();
          else if (element.key() == "title")
            partialEntry.title = element.value().get<std::string>();
          else if (element.key() == "id")
            partialEntry.epgUid = element.value().get<unsigned int>();
        }

        if (partialEntry.EntryFound())
        {
          Logger::Log(LEVEL_DEBUG, "%s Loaded EPG event partial details for sref: %s, time: %ld - title: %s, epgId: %u - '%s'", __FUNCTION__, serviceReference.c_str(), startTime, partialEntry.title.c_str(), partialEntry.epgUid, partialEntry.shortDescription.c_str());
        }

        break; //We only want first event
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot event details from OpenWebIf for sref: %s, time: %ld - JSON parse error - message: %s, exception id: %d", __FUNCTION__, serviceReference.c_str(), startTime, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
  }

  return partialEntry;
}