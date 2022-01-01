/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <kodi/AddonBase.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class ATTR_DLL_LOCAL BaseChannel
    {
    public:
      BaseChannel() = default;
      BaseChannel(const BaseChannel& b) : m_radio(b.IsRadio()), m_uniqueId(b.GetUniqueId()),
        m_channelName(b.GetChannelName()), m_serviceReference(b.GetServiceReference()) {};
      ~BaseChannel() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value) { m_channelName = value; }

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value) { m_serviceReference = value; }

    protected:
      bool m_radio;
      int m_uniqueId = -1;
      std::string m_channelName;
      std::string m_serviceReference;
    };
  } //namespace data
} //namespace enigma2
