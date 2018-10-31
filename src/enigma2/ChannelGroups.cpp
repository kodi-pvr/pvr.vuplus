#include "ChannelGroups.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/LocalizedString.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP> &kodiChannelGroups, bool radio) const
{
  for (const auto& channelGroup : m_channelGroups)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupIndex '%d'", __FUNCTION__, channelGroup->GetGroupName().c_str(), channelGroup->GetUniqueId());

    if (channelGroup->IsRadio() == radio)
    {
      PVR_CHANNEL_GROUP kodiChannelGroup;
      memset(&kodiChannelGroup, 0 , sizeof(PVR_CHANNEL_GROUP));

      channelGroup->UpdateTo(kodiChannelGroup);

      kodiChannelGroups.emplace_back(kodiChannelGroup);
    }
  }
}

PVR_ERROR ChannelGroups::GetChannelGroupMembers(std::vector<PVR_CHANNEL_GROUP_MEMBER> &channelGroupMembers, const std::string &groupName)
{
  ChannelGroupPtr channelGroup = GetChannelGroup(groupName);

  if (!channelGroup)
    return PVR_ERROR_NO_ERROR;

  for (const auto& channel : channelGroup->GetChannelList())
  {
    PVR_CHANNEL_GROUP_MEMBER tag;
    memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

    strncpy(tag.strGroupName, groupName.c_str(), sizeof(tag.strGroupName));
    tag.iChannelUniqueId = channel->GetUniqueId();
    tag.iChannelNumber   = channel->GetChannelNumber();

    Logger::Log(LEVEL_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
        __FUNCTION__, channel->GetChannelName().c_str(), tag.iChannelUniqueId, groupName.c_str(), channel->GetChannelNumber());

    channelGroupMembers.emplace_back(tag);
  }

  return PVR_ERROR_NO_ERROR;
}

int ChannelGroups::GetChannelGroupUniqueId(const std::string &groupName) const
{
  for (const auto& channelGroup : m_channelGroups)
  {
    if (groupName == channelGroup->GetGroupName())
      return channelGroup->GetUniqueId();
  }
  return -1;
}

std::string ChannelGroups::GetChannelGroupServiceReference(const std::string &groupName)  
{
  for (const auto& channelGroup : m_channelGroups)
  {
    if (groupName == channelGroup->GetGroupName())
      return channelGroup->GetServiceReference();
  }
  return "error";
}

ChannelGroupPtr ChannelGroups::GetChannelGroup(int uniqueId)
{
  return m_channelGroups.at(uniqueId - 1);
}

ChannelGroupPtr ChannelGroups::GetChannelGroup(std::string groupName)
{
  ChannelGroupPtr channelGroup = nullptr;

  auto channelGroupPair = m_channelGroupsNameMap.find(groupName);
  if (channelGroupPair != m_channelGroupsNameMap.end()) 
  {
    channelGroup = channelGroupPair->second;
  } 

  return channelGroup;
}

bool ChannelGroups::IsValid(int uniqueId) const
{
  return (uniqueId - 1) < m_channelGroups.size();
}

bool ChannelGroups::IsValid(std::string groupName)
{
  return GetChannelGroup(groupName) != nullptr;
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
  ChannelGroupPtr foundChannelGroup = GetChannelGroup(newChannelGroup.GetGroupName());

  if (!foundChannelGroup)
  {  
    newChannelGroup.SetUniqueId(m_channelGroups.size() + 1);

    m_channelGroups.emplace_back(new ChannelGroup(newChannelGroup));

    ChannelGroupPtr channelGroup = m_channelGroups.back();
    m_channelGroupsNameMap.insert({channelGroup->GetGroupName(), channelGroup});
  }
}

std::vector<ChannelGroupPtr>& ChannelGroups::GetChannelGroupsList()
{
  return m_channelGroups;
}

bool ChannelGroups::LoadChannelGroups()
{
  m_channelGroups.clear();

  bool successful = LoadTVChannelGroups();

  if (successful)
    LoadRadioChannelGroups();

  return successful;
}

bool ChannelGroups::LoadTVChannelGroups()
{
  if ((Settings::GetInstance().GetTVFavouritesMode() == FavouritesGroupMode::AS_FIRST_GROUP &&
      Settings::GetInstance().GetTVChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP) ||
      Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddTVFavouritesChannelGroup();
  }

  if (Settings::GetInstance().GetTVChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
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

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
    {
      ChannelGroup newChannelGroup;

      if (!newChannelGroup.UpdateFrom(pNode, false))
        continue;
  
      AddChannelGroup(newChannelGroup);

      Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
    }
  }

  if (Settings::GetInstance().GetTVFavouritesMode() == FavouritesGroupMode::AS_LAST_GROUP &&
      Settings::GetInstance().GetTVChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddTVFavouritesChannelGroup();
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d TV Channelgroups", __FUNCTION__, m_channelGroups.size());
  return true;
}

bool ChannelGroups::LoadRadioChannelGroups()
{
  if ((Settings::GetInstance().GetRadioFavouritesMode() == FavouritesGroupMode::AS_FIRST_GROUP &&
      Settings::GetInstance().GetRadioChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP) ||
      Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddRadioFavouritesChannelGroup();
  }

  if (Settings::GetInstance().GetRadioChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
  {  
    std::string strTmp = StringUtils::Format("%sweb/getallservices?type=radio&renameserviceforxbmc=yes", Settings::GetInstance().GetConnectionURL().c_str());

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

    pElem = hDoc.FirstChildElement("e2servicelistrecursive").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_DEBUG, "%s Could not find <e2servicelistrecursive> element!", __FUNCTION__);
      return false;
    }

    hRoot=TiXmlHandle(pElem);

    TiXmlElement* pNode = hRoot.FirstChildElement("e2bouquet").Element();

    if (!pNode)
    {
      Logger::Log(LEVEL_DEBUG, "%s Could not find <e2bouquet> element", __FUNCTION__);
      return false;
    }

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2bouquet"))
    {
      ChannelGroup newChannelGroup;

      if (!newChannelGroup.UpdateFrom(pNode, true))
        continue;
  
      AddChannelGroup(newChannelGroup);

      Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
    }
  }

  if (Settings::GetInstance().GetRadioFavouritesMode() == FavouritesGroupMode::AS_LAST_GROUP &&
      Settings::GetInstance().GetRadioChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddRadioFavouritesChannelGroup();
  }

  Logger::Log(LEVEL_INFO, "%s Loaded %d Radio Channelgroups", __FUNCTION__, m_channelGroups.size());
  return true;
}

void ChannelGroups::AddTVFavouritesChannelGroup()
{
  ChannelGroup newChannelGroup;
  newChannelGroup.SetRadio(false);
  newChannelGroup.SetGroupName(LocalizedString(30079)); //Favourites (TV)
  newChannelGroup.SetServiceReference("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.tv\" ORDER BY bouquet");
  AddChannelGroup(newChannelGroup);
  Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());  
}

void ChannelGroups::AddRadioFavouritesChannelGroup()
{
  ChannelGroup newChannelGroup;
  newChannelGroup.SetRadio(true);
  newChannelGroup.SetGroupName(LocalizedString(30080)); //Favourites (Radio)
  newChannelGroup.SetServiceReference("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet");
  AddChannelGroup(newChannelGroup);
  Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());  
}