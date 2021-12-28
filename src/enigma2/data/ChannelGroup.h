/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Channel.h"
#include "ChannelGroupMember.h"
#include "EpgEntry.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/ChannelGroups.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class ATTR_DLL_LOCAL ChannelGroup
    {
    public:
      ChannelGroup() = default;
      ChannelGroup(ChannelGroup &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()),
        m_groupName(c.GetGroupName()), m_serviceReference(c.GetServiceReference()), m_lastScannedGroup(c.IsLastScannedGroup()),
        m_startChannelNumber(c.GetStartChannelNumber()), m_emptyGroup(c.IsEmptyGroup()) {};
      ~ChannelGroup() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value) { m_serviceReference = value; }

      const std::string& GetGroupName() const { return m_groupName; }
      void SetGroupName(const std::string& value) { m_groupName = value; }

      bool IsLastScannedGroup() const { return m_lastScannedGroup; }
      void SetLastScannedGroup(bool value) { m_lastScannedGroup = value; }

      bool IsEmptyGroup() const { return m_emptyGroup; }
      void SetEmptyGroup(bool value) { m_emptyGroup = value; }

      int GetStartChannelNumber() const { return m_startChannelNumber; }
      void SetStartChannelNumber(int value) { m_startChannelNumber = value; }

      bool HasStartChannelNumber() const { return m_startChannelNumber >= 0; }

      void AddChannelGroupMember(std::shared_ptr<enigma2::data::Channel>& channel);

      void SetMemberChannelNumber(std::shared_ptr<enigma2::data::Channel>& channel, int channelNumber);

      bool UpdateFrom(TiXmlElement* groupNode, bool radio);
      void UpdateTo(kodi::addon::PVRChannelGroup& left) const;

      std::vector<enigma2::data::ChannelGroupMember>& GetChannelGroupMembers() { return m_channelGroupMembers; };

      bool Like(const ChannelGroup& right) const;
      bool operator==(const ChannelGroup& right) const;
      bool operator!=(const ChannelGroup& right) const;

    private:
      bool m_radio;
      int m_uniqueId;
      std::string m_serviceReference;
      std::string m_groupName;
      bool m_lastScannedGroup;
      bool m_emptyGroup;
      int m_startChannelNumber = -1;

      std::vector<enigma2::data::ChannelGroupMember> m_channelGroupMembers;
    };
  } //namespace data
} //namespace enigma2
