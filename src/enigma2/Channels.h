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

#include "libXBMC_pvr.h"

namespace enigma2
{
  class ChannelGroups;

  namespace data
  {
    class ChannelGroup;
  }

  class Channels
  {
  public:
    void GetChannels(std::vector<PVR_CHANNEL> &timers, bool bRadio) const;

    int GetChannelUniqueId(const std::string &channelServiceReference);
    data::ChannelPtr GetChannel(int uniqueId);
    data::ChannelPtr GetChannel(const std::string &channelServiceReference);
    bool IsValid(int uniqueId) const;
    bool IsValid(const std::string &channelServiceReference);
    int GetNumChannels() const;
    void ClearChannels();
    std::vector<data::ChannelPtr>& GetChannelsList();
    bool CheckIfAllChannelsHaveInitialEPG() const;
    std::string GetChannelIconPath(std::string strChannelName);
    bool LoadChannels(enigma2::ChannelGroups &channelGroups);

  private:   
    void AddChannel(enigma2::data::Channel &channel, enigma2::data::ChannelGroupPtr channelGroup);
    bool LoadChannels(const std::string groupServiceReference, const std::string groupName, enigma2::data::ChannelGroupPtr channelGroup);

    std::vector<data::ChannelPtr> m_channels;
    std::unordered_map<std::string, data::ChannelPtr> m_channelsServiceReferenceMap;
  };
} //namespace enigma2