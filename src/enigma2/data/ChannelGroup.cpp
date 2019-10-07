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

#include "ChannelGroup.h"

#include <cinttypes>

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool ChannelGroup::Like(const ChannelGroup& right) const
{
  bool isLike = (m_serviceReference == right.m_serviceReference);
  isLike &= (m_groupName == right.m_groupName);

  return isLike;
}

bool ChannelGroup::operator==(const ChannelGroup& right) const
{
  bool isEqual = (m_serviceReference == right.m_serviceReference);
  isEqual &= (m_groupName == right.m_groupName);
  isEqual &= (m_radio == right.m_radio);
  isEqual &= (m_lastScannedGroup == right.m_lastScannedGroup);

  for (int i = 0; i < m_channelGroupMembers.size(); i++)
  {
    isEqual &= (*(m_channelGroupMembers.at(i).GetChannel()) == *(right.m_channelGroupMembers.at(i).GetChannel()));

    if (!isEqual)
      break;
  }

  return isEqual;
}

bool ChannelGroup::operator!=(const ChannelGroup& right) const
{
  return !(*this == right);
}

bool ChannelGroup::UpdateFrom(TiXmlElement* groupNode, bool radio)
{
  std::string serviceReference;
  std::string groupName;

  if (!XMLUtils::GetString(groupNode, "e2servicereference", serviceReference))
    return false;

  // Check whether the current element is not just a label
  if (serviceReference.compare(0, 5, "1:64:") == 0)
    return false;

  if (!XMLUtils::GetString(groupNode, "e2servicename", groupName))
    return false;

  // Check if the current element is hidden or a separator
  if (groupName == "<n/a>" || StringUtils::EndsWith(groupName.c_str(), " - Separator"))
    return false;

  m_serviceReference = serviceReference;
  m_groupName = groupName;
  m_radio = radio;

  if (!radio && (Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::SOME_GROUPS ||
                 Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::CUSTOM_GROUPS))
  {
    auto& customGroupNamelist = Settings::GetInstance().GetCustomTVChannelGroupNameList();
    auto it = std::find_if(customGroupNamelist.begin(), customGroupNamelist.end(),
      [&groupName](std::string& customGroupName) { return customGroupName == groupName; });

    if (it == customGroupNamelist.end())
      return false;
    else
      Logger::Log(LEVEL_DEBUG, "%s Custom TV groups are set, current e2servicename '%s' matched", __FUNCTION__, groupName.c_str());
  }
  else if (radio && (Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::SOME_GROUPS ||
                     Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::CUSTOM_GROUPS))
  {
    auto& customGroupNamelist = Settings::GetInstance().GetCustomRadioChannelGroupNameList();
    auto it = std::find_if(customGroupNamelist.begin(), customGroupNamelist.end(),
      [&groupName](std::string& customGroupName) { return customGroupName == groupName; });

    if (it == customGroupNamelist.end())
      return false;
    else
      Logger::Log(LEVEL_DEBUG, "%s Custom Radio groups are set, current e2servicename '%s' matched", __FUNCTION__, groupName.c_str());
  }
  else if (groupName == "Last Scanned")
  {
    // Last scanned contains TV and Radio channels mixed, therefore we discard here and handle it separately,
    // similar to Favourites when loading channel groups.
    // Note that if Last Scanned is used in only one group for TV we do not discard and only allow it for TV
    // as it is never supplied as a radio group from Enigma2
    return false;
  }

  return true;
}

void ChannelGroup::UpdateTo(PVR_CHANNEL_GROUP& left) const
{
  left.bIsRadio = m_radio;
  left.iPosition = 0; // groups default order, unused
  strncpy(left.strGroupName, m_groupName.c_str(), sizeof(left.strGroupName));
}

void ChannelGroup::AddChannelGroupMember(std::shared_ptr<Channel>& channel)
{
  m_channelGroupMembers.emplace_back(ChannelGroupMember{channel});
}

void ChannelGroup::SetMemberChannelNumber(std::shared_ptr<enigma2::data::Channel>& channel, int channelNumber)
{
  for (auto& member : m_channelGroupMembers)
  {
    if (member.GetChannel() == channel)
    {
      member.SetChannelNumber(channelNumber);
      break;
    }
  }
}
