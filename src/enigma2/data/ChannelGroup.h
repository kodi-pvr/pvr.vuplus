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
    class ChannelGroup;
    typedef std::shared_ptr<enigma2::data::ChannelGroup> ChannelGroupPtr;

    class ChannelGroup 
    {
    public:
      ChannelGroup() = default;
      ChannelGroup(ChannelGroup &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()),
        m_groupName(c.GetGroupName()), m_serviceReference(c.GetServiceReference()) {};
      ~ChannelGroup() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }      

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }  

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }          

      const std::string& GetGroupName() const { return m_groupName; }
      void SetGroupName(const std::string& value ) { m_groupName = value; }        

      int GetGroupState() const { return m_groupState; }
      void SetGroupState(int value) { m_groupState = value; }      

      void AddChannel(ChannelPtr channel);

      std::vector<EpgEntry>& GetInitialEPG() { return m_initialEPG; }

      bool UpdateFrom(TiXmlElement* groupNode, bool radio); 
      void UpdateTo(PVR_CHANNEL_GROUP &left) const;

      std::vector<ChannelPtr> GetChannelList() { return m_channelList; };

    private:
      bool m_radio;
      int m_uniqueId;
      std::string m_serviceReference;
      std::string m_groupName;
      int m_groupState;
      std::vector<EpgEntry> m_initialEPG; 

      std::vector<ChannelPtr> m_channelList;
    };
  } //namespace data
} //namespace enigma2