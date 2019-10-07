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

#include "Channel.h"

#include <memory>

namespace enigma2
{
  namespace data
  {
    class ChannelGroupMember
    {
    public:
      ChannelGroupMember() = default;
      ChannelGroupMember(std::shared_ptr<enigma2::data::Channel>& channel) : m_channel(channel) {};
      ChannelGroupMember(std::shared_ptr<enigma2::data::Channel>& channel, int channelNumber) : m_channel(channel), m_channelNumber(channelNumber) {};
      ~ChannelGroupMember() = default;

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }

      const std::shared_ptr<enigma2::data::Channel>& GetChannel() const { return m_channel; };
      void SetChannel(std::shared_ptr<enigma2::data::Channel>& channel);

    private:
      int m_channelNumber = 0;
      std::shared_ptr<enigma2::data::Channel> m_channel;
    };
  } //namespace data
} //namespace enigma2