#include "ChannelGroups.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP> &kodiChannelGroups) const
{
  for (const auto& channelGroup : m_channelGroups)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupIndex '%d'", __FUNCTION__, channelGroup.GetGroupName().c_str(), channelGroup.GetUniqueId());
    PVR_CHANNEL_GROUP kodiChannelGroup;
    memset(&kodiChannelGroup, 0 , sizeof(PVR_CHANNEL_GROUP));

    channelGroup.UpdateTo(kodiChannelGroup);

    kodiChannelGroups.emplace_back(kodiChannelGroup);
  }
}

int ChannelGroups::GetChannelGroupUniqueId(const std::string &groupName) const
{
  for (const auto& channelGroup : m_channelGroups)
  {
    if (groupName == channelGroup.GetGroupName())
      return channelGroup.GetUniqueId();
  }
  return -1;
}

std::string ChannelGroups::GetChannelGroupServiceReference(const std::string &groupName)  
{
  for (const auto& channelGroup : m_channelGroups)
  {
    if (groupName == channelGroup.GetGroupName())
      return channelGroup.GetServiceReference();
  }
  return "error";
}

enigma2::data::ChannelGroup& ChannelGroups::GetChannelGroup(int uniqueId)
{
  return m_channelGroups.at(uniqueId - 1);
}

bool ChannelGroups::IsValid(int uniqueId) const
{
  return (uniqueId - 1) < m_channelGroups.size();
}

int ChannelGroups::GetNumChannelGroups() const
{
  return m_channelGroups.size();
}

void ChannelGroups::ClearChannelGroups()
{
  m_channelGroups.clear();
}

void ChannelGroups::AddChannelGroup(ChannelGroup& newChannelGroup)
{
  newChannelGroup.SetUniqueId(m_channelGroups.size() + 1);

  m_channelGroups.emplace_back(newChannelGroup);
}

std::vector<enigma2::data::ChannelGroup>& ChannelGroups::GetChannelGroupsList()
{
  return m_channelGroups;
}

bool ChannelGroups::LoadChannelGroups()
{
  std::string strTmp; 

  strTmp = StringUtils::Format("%sweb/getservices", Settings::GetInstance().GetConnectionURL().c_str());

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
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2service> element", __FUNCTION__);
    return false;
  }

  m_channelGroups.clear();

  std::string serviceReference;
  std::string groupName;

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
  {
    ChannelGroup newChannelGroup;

    if (!newChannelGroup.UpdateFrom(pNode))
      continue;
 
    AddChannelGroup(newChannelGroup);

    Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Channelgroups", __FUNCTION__, m_channelGroups.size());
  return true;
}