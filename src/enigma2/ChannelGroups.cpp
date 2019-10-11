/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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

#include "ChannelGroups.h"

#include "../Enigma2.h"
#include "../client.h"
#include "p8-platform/util/StringUtils.h"
#include "util/XMLUtils.h"
#include "utilities/FileUtils.h"
#include "utilities/LocalizedString.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <regex>

#include <nlohmann/json.hpp>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using json = nlohmann::json;

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP>& kodiChannelGroups, bool radio) const
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

PVR_ERROR ChannelGroups::GetChannelGroupMembers(std::vector<PVR_CHANNEL_GROUP_MEMBER>& channelGroupMembers, const std::string& groupName)
{
  std::shared_ptr<ChannelGroup> channelGroup = GetChannelGroupUsingName(groupName);

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

    strncpy(tag.strGroupName, groupName.c_str(), sizeof(tag.strGroupName) - 1);
    tag.iChannelUniqueId = channel->GetUniqueId();
    tag.iChannelNumber = channelNumberInGroup; //Keep the channels in list order as per the groups on the STB

    Logger::Log(LEVEL_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d", __FUNCTION__, channel->GetChannelName().c_str(), tag.iChannelUniqueId, groupName.c_str(), channel->GetChannelNumber());

    channelGroupMembers.emplace_back(tag);

    channelNumberInGroup++;
  }

  Logger::Log(LEVEL_DEBUG, "%s - Finished getting ChannelGroupsMembers for PVR for group: %s", __FUNCTION__, groupName.c_str());

  return PVR_ERROR_NO_ERROR;
}

std::string ChannelGroups::GetChannelGroupServiceReference(const std::string& groupName)
{
  for (const auto& channelGroup : m_channelGroups)
  {
    if (groupName == channelGroup->GetGroupName())
      return channelGroup->GetServiceReference();
  }
  return "error";
}

std::shared_ptr<ChannelGroup> ChannelGroups::GetChannelGroup(const std::string& groupServiceReference)
{
  const auto channelGroupPair = m_channelGroupsServiceReferenceMap.find(groupServiceReference);
  if (channelGroupPair != m_channelGroupsServiceReferenceMap.end())
    return channelGroupPair->second;

  return {};
}

std::shared_ptr<ChannelGroup> ChannelGroups::GetChannelGroupUsingName(const std::string& groupName)
{
  std::shared_ptr<ChannelGroup> channelGroup;

  auto channelGroupPair = m_channelGroupsNameMap.find(groupName);
  if (channelGroupPair != m_channelGroupsNameMap.end())
  {
    channelGroup = channelGroupPair->second;
  }

  return channelGroup;
}

bool ChannelGroups::IsValid(std::string groupName)
{
  return GetChannelGroupUsingName(groupName) != nullptr;
}

int ChannelGroups::GetNumChannelGroups() const
{
  return m_channelGroups.size();
}

void ChannelGroups::ClearChannelGroups()
{
  m_channelGroups.clear();
  m_channelGroupsNameMap.clear();
  m_channelGroupsServiceReferenceMap.clear();

  Settings::GetInstance().SetUsesLastScannedChannelGroup(false);
}

void ChannelGroups::AddChannelGroup(ChannelGroup& newChannelGroup)
{
  std::shared_ptr<ChannelGroup> foundChannelGroup = GetChannelGroupUsingName(newChannelGroup.GetGroupName());

  if (!foundChannelGroup)
  {
    newChannelGroup.SetUniqueId(m_channelGroups.size() + 1);

    m_channelGroups.emplace_back(new ChannelGroup(newChannelGroup));

    std::shared_ptr<ChannelGroup> channelGroup = m_channelGroups.back();
    m_channelGroupsNameMap.insert({channelGroup->GetGroupName(), channelGroup});
    m_channelGroupsServiceReferenceMap.insert({channelGroup->GetServiceReference(), channelGroup});
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

      if (!newChannelGroup.UpdateFrom(pNode, false))
        continue;

      AddChannelGroup(newChannelGroup);

      Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
    }
  }

  LoadChannelGroupsStartPosition(false);

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
    const std::string strTmp = StringUtils::Format("%sweb/getservices?sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"bouquets.radio\" ORDER BY bouquet").c_str());

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

      if (!newChannelGroup.UpdateFrom(pNode, true))
        continue;

      AddChannelGroup(newChannelGroup);

      Logger::Log(LEVEL_INFO, "%s Loaded channelgroup: %s", __FUNCTION__, newChannelGroup.GetGroupName().c_str());
    }
  }

  LoadChannelGroupsStartPosition(true);

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

void ChannelGroups::LoadChannelGroupsStartPosition(bool radio)
{
  if (Settings::GetInstance().SupportsChannelNumberGroupStartPos())
  {
    //We can use the JSON API so let's supplement the existing groups with start channel numbers
    std::string jsonURL;

    if (!radio)
    {
      Logger::Log(LEVEL_INFO, "%s loading channel group start channel number for all TV groups", __FUNCTION__);
      jsonURL = StringUtils::Format("%sapi/getservices", Settings::GetInstance().GetConnectionURL().c_str());
    }
    else
    {
      Logger::Log(LEVEL_INFO, "%s loading channel group start channel number for all Radio groups", __FUNCTION__);
      jsonURL = StringUtils::Format("%sapi/getservices?sRef=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline("1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"bouquets.radio\" ORDER BY bouquet").c_str());
    }

    const std::string strJson = WebUtils::GetHttpXML(jsonURL);

    try
    {
      auto jsonDoc = json::parse(strJson);

      if (!jsonDoc["services"].empty())
      {
        for (const auto& it : jsonDoc["services"].items())
        {
          auto jsonGroup = it.value();

          std::string serviceReference = jsonGroup["servicereference"].get<std::string>();

          // Check whether the current element is not just a label or that it's not a hidden entry
          if (serviceReference.compare(0, 5, "1:64:") == 0 || serviceReference.compare(0, 6, "1:320:") == 0)
            continue;

          auto group = GetChannelGroup(serviceReference);

          if (group)
          {
            if (!jsonGroup["startpos"].empty())
            {
              Logger::Log(LEVEL_DEBUG, "%s For Group %s, set start pos for channel number is %d", __FUNCTION__, jsonGroup["servicename"].get<std::string>().c_str(), jsonGroup["startpos"].get<int>());
              group->SetStartChannelNumber(jsonGroup["startpos"].get<int>());
            }
          }
        }
      }
    }
    catch (nlohmann::detail::parse_error& e)
    {
      Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load start channel number for group from OpenWebIf - JSON parse error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
    }
    catch (nlohmann::detail::type_error& e)
    {
      Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __FUNCTION__, e.what(), e.id);
    }
  }
}
