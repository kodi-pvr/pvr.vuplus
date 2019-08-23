#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
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

#include "ChannelGroups.h"
#include "data/Channel.h"
#include "kodi/libXBMC_pvr.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace enigma2
{
  class ChannelGroups;

  namespace data
  {
    class ChannelGroup;
  }

  enum class ChannelsChangeState : int // same type as addon settings
  {
    NO_CHANGE = 0,
    CHANNEL_GROUPS_CHANGED,
    CHANNELS_CHANGED
  };

  class Channels
  {
  public:
    void GetChannels(std::vector<PVR_CHANNEL>& timers, bool bRadio) const;

    int GetChannelUniqueId(const std::string& channelServiceReference);
    std::shared_ptr<enigma2::data::Channel> GetChannel(int uniqueId);
    std::shared_ptr<enigma2::data::Channel> GetChannel(const std::string& channelServiceReference);
    std::shared_ptr<enigma2::data::Channel> GetChannel(const std::string& channelName, bool isRadio);
    bool IsValid(int uniqueId);
    bool IsValid(const std::string& channelServiceReference);
    int GetNumChannels() const;
    void ClearChannels();
    std::vector<std::shared_ptr<enigma2::data::Channel>>& GetChannelsList();
    std::string GetChannelIconPath(std::string& channelName);
    bool LoadChannels(enigma2::ChannelGroups& channelGroups);

    ChannelsChangeState CheckForChannelAndGroupChanges(enigma2::ChannelGroups& latestChannelGroups, enigma2::Channels& latestChannels);

  private:
    void AddChannel(enigma2::data::Channel& channel, std::shared_ptr<enigma2::data::ChannelGroup>& channelGroup);
    bool LoadChannels(const std::string groupServiceReference,
                      const std::string groupName,
                      std::shared_ptr<enigma2::data::ChannelGroup>& channelGroup);
    int LoadChannelsExtraData(const std::shared_ptr<enigma2::data::ChannelGroup> channelGroup, int lastGroupLatestChannelPosition);

    std::vector<std::shared_ptr<enigma2::data::Channel>> m_channels;
    std::unordered_map<int, std::shared_ptr<enigma2::data::Channel>> m_channelsUniqueIdMap;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::Channel>> m_channelsServiceReferenceMap;

    ChannelGroups m_channelGroups;
  };
} //namespace enigma2