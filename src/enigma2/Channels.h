/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "ChannelGroups.h"
#include "Providers.h"
#include "data/Channel.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <kodi/addon-instance/pvr/Channels.h>

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

  class ATTRIBUTE_HIDDEN Channels
  {
  public:
    Channels(Providers& providers) : m_providers(providers) {}

    void GetChannels(std::vector<kodi::addon::PVRChannel>& kodiChannels, bool bRadio) const;

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
    void LoadProviders();

    std::vector<std::shared_ptr<enigma2::data::Channel>> m_channels;
    std::unordered_map<int, std::shared_ptr<enigma2::data::Channel>> m_channelsUniqueIdMap;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::Channel>> m_channelsServiceReferenceMap;

    ChannelGroups m_channelGroups;
    Providers& m_providers;
  };
} //namespace enigma2
