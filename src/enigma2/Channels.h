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

#include <string>
#include <vector>

#include "data/Channel.h"

#include "libXBMC_pvr.h"

namespace enigma2
{
  class ChannelGroups;

  class Channels
  {
  public:
    void GetChannels(std::vector<PVR_CHANNEL> &timers, bool bRadio) const;

    int GetChannelUniqueId(const std::string strServiceReference) const;
    enigma2::data::Channel& GetChannel(int uniqueId);
    bool IsValid(int uniqueId) const;
    int GetNumChannels() const;
    void ClearChannels();
    std::vector<enigma2::data::Channel>& GetChannelsList();
    bool CheckIfAllChannelsHaveInitialEPG() const;
    std::string GetChannelIconPath(std::string strChannelName);
    bool LoadChannels(enigma2::ChannelGroups &channelGroups);

  private:   
    void AddChannel(enigma2::data::Channel& channel);
    bool LoadChannels(std::string groupServiceReference, std::string groupName);

    std::vector<enigma2::data::Channel> m_channels;
  };
} //namespace enigma2