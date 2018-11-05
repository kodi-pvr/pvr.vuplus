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

#include "libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    class ChannelGroup;
    typedef std::shared_ptr<enigma2::data::ChannelGroup> ChannelGroupPtr;

    class Channel;
    typedef std::shared_ptr<enigma2::data::Channel> ChannelPtr;

    class Channel
    {
    public:
      const std::string SERVICE_REF_ICON_PREFIX = "1:0:1:";
      const std::string SERVICE_REF_ICON_POSTFIX = ":0:0:0";

      Channel() = default;
      Channel(Channel &c) : m_radio(c.IsRadio()), m_uniqueId(c.GetUniqueId()), m_channelNumber(c.GetChannelNumber()),
        m_channelName(c.GetChannelName()), m_serviceReference(c.GetServiceReference()),
        m_streamURL(c.GetStreamURL()), m_m3uURL(c.GetM3uURL()), m_iconPath(c.GetIconPath()) {};
      ~Channel() = default;

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }      

      bool RequiresInitialEPG() const { return m_requiresInitialEPG; }
      void SetRequiresInitialEPG(bool value) { m_requiresInitialEPG = value; }      

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }      

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }      

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value ) { m_channelName = value; }      

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }      

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value ) { m_streamURL = value; }      

      const std::string& GetM3uURL() const { return m_m3uURL; }
      void SetM3uURL(const std::string& value ) { m_m3uURL = value; }      

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value ) { m_iconPath = value; }

      bool UpdateFrom(TiXmlElement* channelNode, const std::string &enigmaURL); 
      void UpdateTo(PVR_CHANNEL &left) const;

      void AddChannelGroup(data::ChannelGroupPtr channelGroup);
      std::vector<ChannelGroupPtr> GetChannelGroupList() { return m_channelGroupList; };

    private:   
      bool m_radio;
      bool m_requiresInitialEPG = true;
      int m_uniqueId;
      int m_channelNumber;
      std::string m_channelName;
      std::string m_serviceReference;
      std::string m_streamURL;
      std::string m_m3uURL;
      std::string m_iconPath;

      std::vector<ChannelGroupPtr> m_channelGroupList;
    };
  } //namespace data
} //namespace enigma2