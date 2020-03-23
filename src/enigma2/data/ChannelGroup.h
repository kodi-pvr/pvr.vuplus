/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Channel.h"
#include "ChannelGroupMember.h"
#include "EpgEntry.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class ChannelGroup
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
      void UpdateTo(PVR_CHANNEL_GROUP& left) const;

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