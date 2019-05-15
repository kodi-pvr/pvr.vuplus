#include "ChannelGroups.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/LocalizedString.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"
#include "utilities/FileUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP> &kodiChannelGroups, bool radio) const
{
  Logger::Log(LEVEL_DEBUG, "%s - Starting to get ChannelGroups for PVR", __FUNCTION__);

  for (const auto& channelGroup : m_channelGroups)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupIndex '%d'", __FUNCTION__, channelGroup->GetGroupName().c_str(), channelGroup->GetUniqueId());

    if (channelGroup->IsRadio() == radio && !channelGroup->IsEmptyGroup())
    {
      PVR_CHANNEL_GROUP kodiChannelGroup = {0};

      channelGroup->UpdateTo(kodiChannelGroup);

      kodiChannelGroups.emplace_back(kodiChannelGroup);
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s - Finished getting ChannelGroups for PVR", __FUNCTION__);
}

PVR_ERROR ChannelGroups::GetChannelGroupMembers(std::vector<PVR_CHANNEL_GROUP_MEMBER> &channelGroupMembers, const std::string &groupName)
{
  std::shared_ptr<ChannelGroup> channelGroup = GetChannelGroup(groupName);

  if (!channelGroup)
  {
    Logger::Log(LEVEL_NOTICE, "%s - Channel Group not found, could not get ChannelGroupsMembers for PVR for group: %s", __FUNCTION__, groupName.c_str());

    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    Logger::Log(LEVEL_DEBUG, "%s - Starting to get ChannelGroupsMembers for PVR for group: %s", __FUNCTION__, groupName.c_str());
  }

  int channelNumberInGroup = 1;

  for (const auto& channel : channelGroup->GetChannelList())
  {
    PVR_CHANNEL_GROUP_MEMBER tag = {0};

    strncpy(tag.strGroupName, groupName.c_str(), sizeof(tag.strGroupName));
    tag.iChannelUniqueId = channel->GetUniqueId();
    tag.iChannelNumber   = channelNumberInGroup; //Keep the channels in list order as per the groups on the STB

    Logger::Log(LEVEL_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
        __FUNCTION__, channel->GetChannelName().c_str(), tag.iChannelUniqueId, groupName.c_str(), channel->GetChannelNumber());

    channelGroupMembers.emplace_back(tag);

    channelNumberInGroup++;
  }

  Logger::Log(LEVEL_DEBUG, "%s - Finished getting ChannelGroupsMembers for PVR for group: %s", __FUNCTION__, groupName.c_str());

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

std::shared_ptr<ChannelGroup> ChannelGroups::GetChannelGroup(int uniqueId)
{
  return m_channelGroups.at(uniqueId - 1);
}

std::shared_ptr<ChannelGroup> ChannelGroups::GetChannelGroup(std::string groupName)
{
  std::shared_ptr<ChannelGroup> channelGroup = nullptr;

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
  m_channelGroupsNameMap.clear();

  m_extraDataChannelGroups.clear();

  Settings::GetInstance().SetUsesLastScannedChannelGroup(false);
}

void ChannelGroups::AddChannelGroup(ChannelGroup& newChannelGroup)
{
  std::shared_ptr<ChannelGroup> foundChannelGroup = GetChannelGroup(newChannelGroup.GetGroupName());

  if (!foundChannelGroup)
  {
    newChannelGroup.SetUniqueId(m_channelGroups.size() + 1);

    m_channelGroups.emplace_back(new ChannelGroup(newChannelGroup));

    std::shared_ptr<ChannelGroup> channelGroup = m_channelGroups.back();
    m_channelGroupsNameMap.insert({channelGroup->GetGroupName(), channelGroup});
  }
}

std::vector<std::shared_ptr<ChannelGroup>>& ChannelGroups::GetChannelGroupsList()
{
  return m_channelGroups;
}

bool ChannelGroups::LoadChannelGroups()
{
  ClearChannelGroups();

  bool successful = LoadTVChannelGroups();

  if (successful)
    LoadRadioChannelGroups();

  return successful;
}

bool ChannelGroups::LoadTVChannelGroups()
{
  int tempNumChannelGroups = m_channelGroups.size();

  if ((Settings::GetInstance().GetTVFavouritesMode() == FavouritesGroupMode::AS_FIRST_GROUP &&
      Settings::GetInstance().GetTVChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP) ||
      Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddTVFavouritesChannelGroup();
  }

  if (Settings::GetInstance().GetTVChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
  {
    const std::string strTmp = StringUtils::Format("%sweb/getservices", Settings::GetInstance().GetConnectionURL().c_str());

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

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2service"))
    {
      ChannelGroup newChannelGroup;

      if (!newChannelGroup.UpdateFrom(pNode, false, m_extraDataChannelGroups))
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

  if ((!Settings::GetInstance().ExcludeLastScannedTVGroup() && Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::ALL_GROUPS) ||
      m_channelGroups.empty()) //If there are no channel groups default to including the last scanned group for TV Only
    AddTVLastScannedChannelGroup();

  Logger::Log(LEVEL_INFO, "%s Loaded %d TV Channelgroups", __FUNCTION__, m_channelGroups.size() - tempNumChannelGroups);
  return true;
}

bool ChannelGroups::LoadRadioChannelGroups()
{
  int tempNumChannelGroups = m_channelGroups.size();

  if ((Settings::GetInstance().GetRadioFavouritesMode() == FavouritesGroupMode::AS_FIRST_GROUP &&
      Settings::GetInstance().GetRadioChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP) ||
      Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::FAVOURITES_GROUP)
  {
    AddRadioFavouritesChannelGroup();
  }

  if (Settings::GetInstance().GetRadioChannelGroupMode() != ChannelGroupMode::FAVOURITES_GROUP)
  {
    const std::string strTmp = StringUtils::Format("%sweb/getallservices?type=radio&renameserviceforxbmc=yes", Settings::GetInstance().GetConnectionURL().c_str());

    const std::string strXML = WebUtils::GetHttpXML(strTmp);

    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);

    TiXmlElement* pElem = hDoc.FirstChildElement("e2servicelistrecursive").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_ERROR, "%s Could not find <e2servicelistrecursive> element for radio groups!", __FUNCTION__);
      return false;
    }

    TiXmlHandle hRoot = TiXmlHandle(pElem);

    TiXmlElement* pNode = hRoot.FirstChildElement("e2bouquet").Element();

    if (!pNode)
    {
      Logger::Log(LEVEL_ERROR, "%s Could not find <e2bouquet> element for radio groups", __FUNCTION__);
      return false;
    }

    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2bouquet"))
    {
      ChannelGroup newChannelGroup;

      if (!newChannelGroup.UpdateFrom(pNode, true, m_extraDataChannelGroups))
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

  if (!Settings::GetInstance().ExcludeLastScannedRadioGroup() && Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::ALL_GROUPS)
    AddRadioLastScannedChannelGroup();

  Logger::Log(LEVEL_INFO, "%s Loaded %d Radio Channelgroups", __FUNCTION__, m_channelGroups.size() - tempNumChannelGroups);
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

void ChannelGroups::AddTVLastScannedChannelGroup()
{
  ChannelGroup newChannelGroup;
  newChannelGroup.SetRadio(false);
  newChannelGroup.SetGroupName(LocalizedString(30112)); //Last Scanned (TV)
  newChannelGroup.SetServiceReference("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.LastScanned.tv\" ORDER BY bouquet");
  newChannelGroup.SetLastScannedGroup(true);
  AddChannelGroup(newChannelGroup);
  Settings::GetInstance().SetUsesLastScannedChannelGroup(true);
  Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
}

void ChannelGroups::AddRadioLastScannedChannelGroup()
{
  ChannelGroup newChannelGroup;
  newChannelGroup.SetRadio(true);
  newChannelGroup.SetGroupName(LocalizedString(30113)); //Last Scanned (Radio)
  //Hack used here, extra space in service reference so we can spearate TV and Radio, it must be unique
  newChannelGroup.SetServiceReference("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET  \"userbouquet.LastScanned.tv\" ORDER BY bouquet");
  newChannelGroup.SetLastScannedGroup(true);
  AddChannelGroup(newChannelGroup);
  Settings::GetInstance().SetUsesLastScannedChannelGroup(true);
  Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
}