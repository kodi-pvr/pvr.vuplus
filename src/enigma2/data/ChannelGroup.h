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
#include <vector>

#include "Channel.h"
#include "EpgEntry.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    class ChannelGroup
    {
    public:
      ChannelGroup() = default;
      ChannelGroup(ChannelGroup &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()),
        m_groupName(c.GetGroupName()), m_serviceReference(c.GetServiceReference()), m_lastScannedGroup(c.IsLastScannedGroup()) {};
      ~ChannelGroup() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }

      const std::string& GetGroupName() const { return m_groupName; }
      void SetGroupName(const std::string& value ) { m_groupName = value; }

      bool IsLastScannedGroup() const { return m_lastScannedGroup; }
      void SetLastScannedGroup(bool value) { m_lastScannedGroup = value; }

      void AddChannel(std::shared_ptr<enigma2::data::Channel> channel);

      bool UpdateFrom(TiXmlElement* groupNode, bool radio, std::vector<std::shared_ptr<ChannelGroup>> &extraDataChannelGroups);
      void UpdateTo(PVR_CHANNEL_GROUP &left) const;

      std::vector<std::shared_ptr<enigma2::data::Channel>> GetChannelList() { return m_channelList; };

      bool Like(const ChannelGroup &right) const;
      bool operator==(const ChannelGroup &right) const;
      bool operator!=(const ChannelGroup &right) const;

    private:
      bool m_radio;
      int m_uniqueId;
      std::string m_serviceReference;
      std::string m_groupName;
      bool m_lastScannedGroup;

      std::vector<std::shared_ptr<enigma2::data::Channel>> m_channelList;
    };
  } //namespace data
} //namespace enigma2