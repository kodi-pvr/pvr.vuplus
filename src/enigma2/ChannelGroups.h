#pragma once 
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "data/Channel.h"
#include "data/ChannelGroup.h"

#include "libXBMC_pvr.h"

namespace enigma2
{
  class ChannelGroups
  {
  public:
    void GetChannelGroups(std::vector<PVR_CHANNEL_GROUP> &channelGroups) const;
    PVR_ERROR GetChannelGroupMembers(std::vector<PVR_CHANNEL_GROUP_MEMBER> &channelGroupMembers, const std::string &groupName);

    int GetChannelGroupUniqueId(const std::string &groupName) const;
    std::string GetChannelGroupServiceReference(const std::string &groupName);
    std::shared_ptr<enigma2::data::ChannelGroup> GetChannelGroup(int uniqueId);
    std::shared_ptr<enigma2::data::ChannelGroup> GetChannelGroup(std::string groupName);
    bool IsValid(int uniqueId) const;
    bool IsValid(std::string groupName);
    int GetNumChannelGroups() const;
    void ClearChannelGroups();
    std::vector<std::shared_ptr<enigma2::data::ChannelGroup>>& GetChannelGroupsList();
    bool LoadChannelGroups();

  private:   
    void AddChannelGroup(enigma2::data::ChannelGroup& channelGroup);

    std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroups;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroupsNameMap;
  };
} //namespace enigma2