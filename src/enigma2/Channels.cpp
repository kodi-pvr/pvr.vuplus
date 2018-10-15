#include "Channels.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "ChannelGroups.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void Channels::GetChannels(std::vector<PVR_CHANNEL> &kodiChannels, bool bRadio) const
{
  for (const auto& channel : m_channels)
  {
    if (channel.IsRadio() == bRadio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelIndex '%d'", __FUNCTION__, channel.GetChannelName().c_str(), channel.GetUniqueId());
      PVR_CHANNEL kodiChannel;
      memset(&kodiChannel, 0, sizeof(PVR_CHANNEL));

      channel.UpdateTo(kodiChannel);

      kodiChannels.emplace_back(kodiChannel);
    }
  }
}

int Channels::GetChannelUniqueId(const std::string strServiceReference) const
{
  for (const auto& channel : m_channels)
  {
    if (strServiceReference == channel.GetServiceReference())
      return channel.GetUniqueId();
  }
  return -1;
}

enigma2::data::Channel& Channels::GetChannel(int uniqueId)
{
  return m_channels.at(uniqueId - 1);
}

bool Channels::IsValid(int uniqueId) const
{
  return (uniqueId - 1) < m_channels.size();
}

int Channels::GetNumChannels() const
{
  return m_channels.size();
}

void Channels::ClearChannels()
{
  m_channels.clear();
}

void Channels::AddChannel(Channel& newChannel)
{
  newChannel.SetUniqueId(m_channels.size() + 1);
  newChannel.SetChannelNumber(m_channels.size() + 1);

  m_channels.emplace_back(newChannel);
}

std::vector<enigma2::data::Channel>& Channels::GetChannelsList()
{
  return m_channels;
}

bool Channels::CheckIfAllChannelsHaveInitialEPG() const
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

std::string Channels::GetChannelIconPath(std::string strChannelName)  
{
  for (const auto& channel : m_channels)
  {
    if (strChannelName == channel.GetChannelName())
      return channel.GetIconPath();
  }
  return "";
}

bool Channels::LoadChannels(ChannelGroups &channelGroups)
{
  bool bOk = false;

  ClearChannels();
  // Load Channels
  for (const auto& group : channelGroups.GetChannelGroupsList())
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

bool Channels::LoadChannels(std::string groupServiceReference, std::string groupName)
{
  Logger::Log(LEVEL_INFO, "%s loading channel group: '%s'", __FUNCTION__, groupName.c_str());

  std::string strTmp;
  strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(groupServiceReference).c_str());

  std::string strXML = WebUtils::GetHttpXML(strTmp);  
  
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

  isRadio = (groupName == "radio");

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    Channel newChannel;
    newChannel.SetRadio(isRadio);
    newChannel.SetGroupName(groupName);

    if (!newChannel.UpdateFrom(pNode, Settings::GetInstance().GetConnectionURL()))
      continue;

    AddChannel(newChannel);
    Logger::Log(LEVEL_INFO, "%s Loaded channel: %s, Icon: %s", __FUNCTION__, newChannel.GetChannelName().c_str(), newChannel.GetIconPath().c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channels", __FUNCTION__, GetNumChannels());
  return true;
}