#include "Channels.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "Admin.h"
#include "ChannelGroups.h"
#include "utilities/json.hpp"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using json = nlohmann::json;

void Channels::GetChannels(std::vector<PVR_CHANNEL> &kodiChannels, bool bRadio) const
{
  for (const auto& channel : m_channels)
  {
    if (channel->IsRadio() == bRadio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelIndex '%d'", __FUNCTION__, channel->GetChannelName().c_str(), channel->GetUniqueId());
      PVR_CHANNEL kodiChannel;
      memset(&kodiChannel, 0, sizeof(PVR_CHANNEL));

      channel->UpdateTo(kodiChannel);

      kodiChannels.emplace_back(kodiChannel);
    }
  }
}

int Channels::GetChannelUniqueId(const std::string &channelServiceReference)
{
  std::shared_ptr<Channel> channel = GetChannel(channelServiceReference);
  int uniqueId = -1;

  if (channel)
    uniqueId = channel->GetUniqueId();

  return uniqueId;
}

std::shared_ptr<Channel> Channels::GetChannel(int uniqueId)
{
  return m_channels.at(uniqueId - 1);
}

std::shared_ptr<Channel> Channels::GetChannel(const std::string &channelServiceReference)
{
  std::shared_ptr<Channel> channel = nullptr;

  auto channelPair = m_channelsServiceReferenceMap.find(channelServiceReference);
  if (channelPair != m_channelsServiceReferenceMap.end()) 
  {
    channel = channelPair->second;
  } 

  return channel;
}

bool Channels::IsValid(int uniqueId) const
{
  return (uniqueId - 1) < m_channels.size();
}

bool Channels::IsValid(const std::string &channelServiceReference)
{
  return GetChannel(channelServiceReference) != nullptr;
}

int Channels::GetNumChannels() const
{
  return m_channels.size();
}

void Channels::ClearChannels()
{
  m_channels.clear();
}

void Channels::AddChannel(Channel &newChannel, std::shared_ptr<ChannelGroup> &channelGroup)
{
  std::shared_ptr<Channel> foundChannel = GetChannel(newChannel.GetServiceReference());

  if (!foundChannel)
  {
    newChannel.SetUniqueId(m_channels.size() + 1);
    newChannel.SetChannelNumber(m_channels.size() + 1);

    m_channels.emplace_back(new Channel(newChannel));

    std::shared_ptr<Channel> channel = m_channels.back();
    channel->AddChannelGroup(channelGroup);
    channelGroup->AddChannel(channel);

    m_channelsServiceReferenceMap.insert({channel->GetServiceReference(), channel});
  }
  else
  {
    foundChannel->AddChannelGroup(channelGroup);
    channelGroup->AddChannel(foundChannel);
  }
}

std::vector<std::shared_ptr<Channel>>& Channels::GetChannelsList()
{
  return m_channels;
}

bool Channels::CheckIfAllChannelsHaveInitialEPG() const
{
  bool someChannelsStillNeedInitialEPG = false;
  for (const auto& channel : m_channels)
  {
    if (channel->RequiresInitialEPG()) 
    {
      someChannelsStillNeedInitialEPG = true;
    }
  }

  return !someChannelsStillNeedInitialEPG;
}

std::string Channels::GetChannelIconPath(std::string strChannelName)  
{
  for (const auto& channel : m_channels)
  {
    if (strChannelName == channel->GetChannelName())
      return channel->GetIconPath();
  }
  return "";
}

bool Channels::LoadChannels(ChannelGroups &channelGroups)
{
  bool bOk = false;

  ClearChannels();
  // Load Channels
  for (auto& group : channelGroups.GetChannelGroupsList())
  {
    if (LoadChannels(group->GetServiceReference(), group->GetGroupName(), group))
      bOk = true;
  }

  return bOk;
}

bool Channels::LoadChannels(const std::string groupServiceReference, const std::string groupName, std::shared_ptr<ChannelGroup> &channelGroup)
{
  Logger::Log(LEVEL_INFO, "%s loading channel group: '%s'", __FUNCTION__, groupName.c_str());

  const std::string strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(groupServiceReference).c_str());

  const std::string strXML = WebUtils::GetHttpXML(strTmp);  
  
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
  
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    Channel newChannel;
    newChannel.SetRadio(channelGroup->IsRadio());

    if (!newChannel.UpdateFrom(pNode, Settings::GetInstance().GetConnectionURL()))
      continue;

    AddChannel(newChannel, channelGroup);
    Logger::Log(LEVEL_DEBUG, "%s Loaded channel: %s, Group: %s, Icon: %s, ID: %d", __FUNCTION__, newChannel.GetChannelName().c_str(), groupName.c_str(), newChannel.GetIconPath().c_str(), newChannel.GetUniqueId());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channels", __FUNCTION__, GetNumChannels());

  if (Admin::CanUseJsonApi())
  {
    //We can use the JSON API so let's supplement the data with provider information

    const std::string jsonURL = StringUtils::Format("%sapi/getservices?provider=1&picon=1&sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(groupServiceReference).c_str());
    const std::string strJson = WebUtils::GetHttpXML(jsonURL);  
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc)
    {
      Logger::Log(LEVEL_DEBUG, "%s Invalid JSON received, cannot load provider or picon paths from OpenWebIf", __FUNCTION__);
          
      return true;
    }

    if (!jsonDoc["services"].empty())
    {
      for (json::iterator it = jsonDoc["services"].begin(); it != jsonDoc["services"].end(); ++it) 
      {
        auto jsonChannel = it.value();

        auto channel = GetChannel(it.value()["servicereference"].get<std::string>());

        if (channel)
        {
          Logger::Log(LEVEL_DEBUG, "%s For Channel %s, set provider name to %s", __FUNCTION__, jsonChannel["servicename"].get<std::string>().c_str(), jsonChannel["provider"].get<std::string>().c_str());          
          channel->SetProviderlName(it.value()["provider"].get<std::string>());

          if (Settings::GetInstance().UseOpenWebIfPiconPath())
          {
            std::string connectionURL = Settings::GetInstance().GetConnectionURL();
            connectionURL = connectionURL.substr(0, connectionURL.size()-1);
            channel->SetIconPath(StringUtils::Format("%s%s", connectionURL.c_str(), it.value()["picon"].get<std::string>().c_str()));

            Logger::Log(LEVEL_DEBUG, "%s For Channel %s, using OpenWebPiconPath: %s", __FUNCTION__, jsonChannel["servicename"].get<std::string>().c_str(), channel->GetIconPath().c_str());
          }
        }
      }
    }
  }

  return true;
}