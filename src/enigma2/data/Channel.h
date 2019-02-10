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

#include "BaseChannel.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    class ChannelGroup;

    class Channel : public BaseChannel
    {
    public:
      const std::string SERVICE_REF_GENERIC_PREFIX = "1:0:1:";
      const std::string SERVICE_REF_GENERIC_POSTFIX = ":0:0:0";
      const std::string RADIO_SERVICE_TYPE = "2";

      Channel() = default;
      Channel(const Channel &c) : BaseChannel(c), m_channelNumber(c.GetChannelNumber()),
        m_streamURL(c.GetStreamURL()), m_m3uURL(c.GetM3uURL()), m_iconPath(c.GetIconPath()) {};
      ~Channel() = default;

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }

      const std::string& GetGenericServiceReference() const { return m_genericServiceReference; }
      void SetGenericServiceReference(const std::string& value ) { m_genericServiceReference = value; }

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value ) { m_streamURL = value; }

      const std::string& GetM3uURL() const { return m_m3uURL; }
      void SetM3uURL(const std::string& value ) { m_m3uURL = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value ) { m_iconPath = value; }

      const std::string& GetProviderName() const { return m_providerName; }
      void SetProviderlName(const std::string& value ) { m_providerName = value; }

      bool UpdateFrom(TiXmlElement* channelNode);
      void UpdateTo(PVR_CHANNEL &left) const;

      void AddChannelGroup(std::shared_ptr<enigma2::data::ChannelGroup> &channelGroup);
      std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> GetChannelGroupList() { return m_channelGroupList; };

    private:
      std::string GetCommonServiceReference(const std::string &serviceReference);
      std::string GetGenericServiceReference(const std::string &commonServiceReference);
      std::string CreateIconPath(const std::string &commonServiceReference);
      bool HasRadioServiceType();

      int m_channelNumber;
      std::string m_genericServiceReference;
      std::string m_streamURL;
      std::string m_m3uURL;
      std::string m_iconPath;
      std::string m_providerName;

      std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroupList;
    };
  } //namespace data
} //namespace enigma2