/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Channel.h"

#include <memory>

namespace enigma2
{
  namespace data
  {
    class ATTRIBUTE_HIDDEN ChannelGroupMember
    {
    public:
      ChannelGroupMember() = default;
      ChannelGroupMember(std::shared_ptr<enigma2::data::Channel>& channel) : m_channel(channel) {};
      ChannelGroupMember(std::shared_ptr<enigma2::data::Channel>& channel, int channelNumber) : m_channelNumber(channelNumber), m_channel(channel) {};
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
