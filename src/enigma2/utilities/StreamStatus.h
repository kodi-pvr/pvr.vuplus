/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace enigma2
{
  namespace utilities
  {
    enum class StreamType
      : int // same type as addon settings
    {
      DIRECTLY_STREAMED = 0,
      TRANSCODED
    };

    struct StreamStatus
    {
      std::string m_ipAddress;
      std::string m_serviceReference;
      std::string m_channelName;
      StreamType m_streamType;
    };
  } //namespace utilities
} //namespace enigma2