#include "Channels.h"

#include "../Enigma2.h"
#include "../client.h"
#include "Admin.h"
#include "ChannelGroups.h"
#include "p8-platform/util/StringUtils.h"
#include "util/XMLUtils.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <regex>

#include <nlohmann/json.hpp>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using json = nlohmann::json;

void Channels::GetChannels(std::vector<PVR_CHANNEL>& kodiChannels, bool bRadio) const
{
  for (const auto& channel : m_channels)
  {
    if (channel->IsRadio() == bRadio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelIndex '%d'", __FUNCTION__, channel->GetChannelName().c_str(),
                  channel->GetUniqueId());
      PVR_CHANNEL kodiChannel = {0};

      channel->UpdateTo(kodiChannel);

      kodiChannels.emplace_back(kodiChannel);
    }
  }
}

int Channels::GetChannelUniqueId(const std::string& channelServiceReference)
{
  std::shared_ptr<Channel> channel = GetChannel(channelServiceReference);
  int uniqueId = PVR_CHANNEL_INVALID_UID;

  if (channel)
    uniqueId = channel->GetUniqueId();

  return uniqueId;
}

std::shared_ptr<Channel> Channels::GetChannel(int uniqueId)
{
  return m_channels.at(uniqueId - 1);
}

std::shared_ptr<Channel> Channels::GetChannel(const std::string& channelServiceReference)
{
  std::shared_ptr<Channel> channel = nullptr;

  auto channelPair = m_channelsServiceReferenceMap.find(channelServiceReference);
  if (channelPair != m_channelsServiceReferenceMap.end())
  {
    channel = channelPair->second;
  }

  return channel;
}

std::shared_ptr<Channel> Channels::GetChannel(const std::string& channelName, bool isRadio)
{
  for (const auto& channel : m_channels)
  {
    if (channelName == channel->GetChannelName() && isRadio == channel->IsRadio())
      return channel;
  }

  return nullptr;
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
  m_channelsServiceReferenceMap.clear();
}

void Channels::AddChannel(Channel& newChannel, std::shared_ptr<ChannelGroup>& channelGroup)
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

std::string Channels::GetChannelIconPath(std::string& channelName)
{
  for (const auto& channel : m_channels)
  {
    if (channelName == channel->GetChannelName())
      return channel->GetIconPath();
  }
  return "";
}

bool Channels::LoadChannels(ChannelGroups& channelGroups)
{
  m_channelGroups = channelGroups;

  bool bOk = false;

  ClearChannels();
  // Load Channels
  for (auto& group : channelGroups.GetChannelGroupsList())
  {
    if (LoadChannels(group->GetServiceReference(), group->GetGroupName(), group))
      bOk = true;
  }

  // Load Channels extra data for groups
  int tvChannelNumberOffset = 0;
  int radioChannelNumberOffset = 0;
  for (const auto& group : channelGroups.GetChannelGroupsList())
  {
    if (group->IsRadio())
      radioChannelNumberOffset = LoadChannelsExtraData(group, radioChannelNumberOffset);
    else
      tvChannelNumberOffset = LoadChannelsExtraData(group, tvChannelNumberOffset);
  }

  return bOk;
}

bool Channels::LoadChannels(const std::string groupServiceReference, const std::string groupName, std::shared_ptr<ChannelGroup>& channelGroup)
{
  Logger::Log(LEVEL_INFO, "%s loading channel group: '%s'", __FUNCTION__, groupName.c_str());

  const std::string strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(groupServiceReference).c_str());

  const std::string strXML = WebUtils::GetHttpXML(strTmp);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2servicelist> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2service> element", __FUNCTION__);
    return false;
  }

  bool emptyGroup = true;

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    Channel newChannel;
    newChannel.SetRadio(channelGroup->IsRadio());

    if (!newChannel.UpdateFrom(pNode))
      continue;
    else
      emptyGroup = false;

    AddChannel(newChannel, channelGroup);
    Logger::Log(LEVEL_DEBUG, "%s Loaded channel: %s, Group: %s, Icon: %s, ID: %d", __FUNCTION__, newChannel.GetChannelName().c_str(), groupName.c_str(), newChannel.GetIconPath().c_str(), newChannel.GetUniqueId());
  }

  channelGroup->SetEmptyGroup(emptyGroup);

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channels", __FUNCTION__, GetNumChannels());

  return true;
}

int Channels::LoadChannelsExtraData(const std::shared_ptr<enigma2::data::ChannelGroup> channelGroup, int lastGroupLatestChannelPosition)
{
  int newChannelPositionOffset = channelGroup->GetStartChannelNumber();

  // In case we don't have a start channel number for this group just use the latest
  if (!channelGroup->HasStartChannelNumber())
    newChannelPositionOffset = lastGroupLatestChannelPosition;

  if (Settings::GetInstance().SupportsProviderNumberAndPiconForChannels())
  {
    Logger::Log(LEVEL_INFO, "%s loading channel group extra data: '%s'", __FUNCTION__, channelGroup->GetGroupName().c_str());

    //We can use the JSON API so let's supplement the data with extra information

    const std::string jsonURL = StringUtils::Format("%sapi/getservices?provider=1&picon=1&sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(channelGroup->GetServiceReference()).c_str());
    const std::string strJson = WebUtils::GetHttpXML(jsonURL);

    try
    {
      auto jsonDoc = json::parse(strJson);

      if (!jsonDoc["services"].empty())
      {
        for (const auto& it : jsonDoc["services"].items())
        {
          auto jsonChannel = it.value();

          std::string serviceReference = jsonChannel["servicereference"].get<std::string>();

          // Check whether the current element is not just a label or that it's not a hidden entry
          if (serviceReference.compare(0, 5, "1:64:") == 0 || serviceReference.compare(0, 6, "1:320:") == 0)
            continue;

          if (Settings::GetInstance().UseStandardServiceReference())
          {
            serviceReference = Channel::CreateStandardServiceReference(serviceReference);
          }

          auto channel = GetChannel(serviceReference);

          if (channel)
          {
            if (!jsonChannel["provider"].empty())
            {
              Logger::Log(LEVEL_DEBUG, "%s For Channel %s, set provider name to %s", __FUNCTION__, jsonChannel["servicename"].get<std::string>().c_str(), jsonChannel["provider"].get<std::string>().c_str());
              channel->SetProviderlName(jsonChannel["provider"].get<std::string>());
            }

            if (!jsonChannel["pos"].empty() && channel->UsingDefaultChannelNumber() && Settings::GetInstance().SupportsChannelNumberGroupStartPos())
            {
              Logger::Log(LEVEL_DEBUG, "%s For Channel %s, set backend channel number to %d", __FUNCTION__, jsonChannel["servicename"].get<std::string>().c_str(), jsonChannel["pos"].get<int>());
              channel->SetChannelNumber(jsonChannel["pos"].get<int>() + lastGroupLatestChannelPosition);
              channel->SetUsingDefaultChannelNumber(false);
            }

            if (Settings::GetInstance().UseOpenWebIfPiconPath())
            {
              if (!jsonChannel["picon"].empty())
              {
                std::string connectionURL = Settings::GetInstance().GetConnectionURL();
                connectionURL = connectionURL.substr(0, connectionURL.size() - 1);
                channel->SetIconPath(StringUtils::Format("%s%s", connectionURL.c_str(), jsonChannel["picon"].get<std::string>().c_str()));

                Logger::Log(LEVEL_DEBUG, "%s For Channel %s, using OpenWebPiconPath: %s", __FUNCTION__, jsonChannel["servicename"].get<std::string>().c_str(), channel->GetIconPath().c_str());
              }
            }
          }
        }
      }

      if (!jsonDoc["pos"].empty())
      {
        newChannelPositionOffset += jsonDoc["pos"].get<int>();

        Logger::Log(LEVEL_DEBUG, "%s For groupName %s, highest  backend channel number offset is %d", __FUNCTION__, channelGroup->GetGroupName().c_str(), newChannelPositionOffset);
      }
    }
    catch (nlohmann::detail::parse_error& e)
    {
      Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load provider or picon paths from OpenWebIf - JSON parse error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
    }
    catch (nlohmann::detail::type_error& e)
    {
      Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
    }
  }

  return newChannelPositionOffset;
}

ChannelsChangeState Channels::CheckForChannelAndGroupChanges(enigma2::ChannelGroups& latestChannelGroups, enigma2::Channels& latestChannels)
{
  if (GetNumChannels() != latestChannels.GetNumChannels())
    return ChannelsChangeState::CHANNELS_CHANGED;

  int foundCount = 0;
  for (const auto& channel : m_channels)
  {
    const std::shared_ptr<Channel> channelPtr = latestChannels.GetChannel(channel->GetServiceReference());

    if (channelPtr)
    {
      foundCount++;

      if (*channelPtr != *channel)
      {
        return ChannelsChangeState::CHANNELS_CHANGED;
      }
    }
  }

  if (foundCount != GetNumChannels())
    return ChannelsChangeState::CHANNELS_CHANGED;

  // Now check the groups
  if (m_channelGroups.GetNumChannelGroups() != latestChannelGroups.GetNumChannelGroups())
    return ChannelsChangeState::CHANNEL_GROUPS_CHANGED;

  foundCount = 0;
  for (const auto& group : m_channelGroups.GetChannelGroupsList())
  {
    const std::shared_ptr<ChannelGroup> channelGroupPtr = latestChannelGroups.GetChannelGroupUsingName(group->GetGroupName());

    if (channelGroupPtr)
    {
      foundCount++;

      if (*channelGroupPtr != *group)
      {
        return ChannelsChangeState::CHANNEL_GROUPS_CHANGED;
      }
    }
  }

  if (foundCount != m_channelGroups.GetNumChannelGroups())
    return ChannelsChangeState::CHANNEL_GROUPS_CHANGED;

  return ChannelsChangeState::NO_CHANGE;
}