/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"
#include "data/ChannelGroup.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <kodi/libXBMC_pvr.h>

namespace enigma2
{
  class ChannelGroups
  {
  public:
    void GetChannelGroups(std::vector<PVR_CHANNEL_GROUP>& channelGroups, bool radio) const;
    PVR_ERROR GetChannelGroupMembers(std::vector<PVR_CHANNEL_GROUP_MEMBER>& channelGroupMembers, const std::string& groupName);

    std::string GetChannelGroupServiceReference(const std::string& groupName);
    std::shared_ptr<enigma2::data::ChannelGroup> GetChannelGroup(const std::string& groupServiceReference);
    std::shared_ptr<enigma2::data::ChannelGroup> GetChannelGroupUsingName(const std::string& groupName);
    bool IsValid(std::string groupName);
    int GetNumChannelGroups() const;
    void ClearChannelGroups();
    std::vector<std::shared_ptr<enigma2::data::ChannelGroup>>& GetChannelGroupsList();
    bool LoadChannelGroups();

  private:
    bool LoadTVChannelGroups();
    bool LoadRadioChannelGroups();
    void AddTVFavouritesChannelGroup();
    void AddRadioFavouritesChannelGroup();
    void AddTVLastScannedChannelGroup();
    void AddRadioLastScannedChannelGroup();
    void AddChannelGroup(enigma2::data::ChannelGroup& channelGroup);
    void LoadChannelGroupsStartPosition(bool radio);

    std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroups;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroupsNameMap;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroupsServiceReferenceMap;
  };
} //namespace enigma2