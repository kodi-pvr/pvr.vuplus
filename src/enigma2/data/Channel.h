/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "BaseChannel.h"

#include <memory>
#include <string>
#include <vector>
#include <array>

#include <kodi/addon-instance/pvr/Channels.h>
#include <kodi/addon-instance/pvr/Providers.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class ChannelGroup;

    class ATTRIBUTE_HIDDEN Channel : public BaseChannel
    {
    public:
      const std::string SERVICE_REF_GENERIC_PREFIX = "1:0:1:";
      const std::string SERVICE_REF_GENERIC_POSTFIX = ":0:0:0";
      // There are at least two different service types for radio, see EN300468 Table 87
      const std::array<std::string, 3> RADIO_SERVICE_TYPES = {"2", "A", "a"};

      Channel() = default;
      Channel(const Channel &c) : BaseChannel(c), m_channelNumber(c.GetChannelNumber()), m_standardServiceReference(c.GetStandardServiceReference()),
        m_extendedServiceReference(c.GetExtendedServiceReference()), m_genericServiceReference(c.GetGenericServiceReference()),
        m_streamURL(c.GetStreamURL()), m_m3uURL(c.GetM3uURL()), m_iconPath(c.GetIconPath()),
        m_providerName(c.GetProviderName()), m_providerUniqueId(c.GetProviderUniqueId()),
        m_fuzzyChannelName(c.GetFuzzyChannelName()), m_streamProgramNumber(c.GetStreamProgramNumber()),
        m_usingDefaultChannelNumber(c.UsingDefaultChannelNumber()), m_isIptvStream(c.IsIptvStream()) {};
      ~Channel() = default;

      int GetChannelNumber() const { return m_channelNumber; }
      void SetChannelNumber(int value) { m_channelNumber = value; }

      const std::string& GetStandardServiceReference() const { return m_standardServiceReference; }
      void SetStandardServiceReference(const std::string& value) { m_standardServiceReference = value; }

      const std::string& GetExtendedServiceReference() const { return m_extendedServiceReference; }
      void SetExtendedServiceReference(const std::string& value) { m_extendedServiceReference = value; }

      const std::string& GetGenericServiceReference() const { return m_genericServiceReference; }
      void SetGenericServiceReference(const std::string& value) { m_genericServiceReference = value; }

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value) { m_streamURL = value; }

      const std::string& GetM3uURL() const { return m_m3uURL; }
      void SetM3uURL(const std::string& value) { m_m3uURL = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      const std::string& GetProviderName() const { return m_providerName; }
      void SetProviderlName(const std::string& value) { m_providerName = value; }

      int GetProviderUniqueId() const { return m_providerUniqueId; }
      void SetProviderUniqueId(unsigned int value) { m_providerUniqueId = value; }

      const std::string& GetFuzzyChannelName() const { return m_fuzzyChannelName; }
      void SetFuzzyChannelName(const std::string& value) { m_fuzzyChannelName = value; }

      int GetStreamProgramNumber() const { return m_streamProgramNumber; }
      void SetStreamProgramNumber(int value) { m_streamProgramNumber = value; }

      bool UsingDefaultChannelNumber() const { return m_usingDefaultChannelNumber; }
      void SetUsingDefaultChannelNumber(bool value) { m_usingDefaultChannelNumber = value; }

      bool IsIptvStream() const { return m_isIptvStream; }

      bool UpdateFrom(TiXmlElement* channelNode);
      void UpdateTo(kodi::addon::PVRChannel& left) const;

      void AddChannelGroup(std::shared_ptr<enigma2::data::ChannelGroup>& channelGroup);
      std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> GetChannelGroupList() { return m_channelGroupList; };

      bool Like(const Channel& right) const;
      bool operator==(const Channel& right) const;
      bool operator!=(const Channel& right) const;

      static std::string NormaliseServiceReference(const std::string& serviceReference);
      static std::string CreateStandardServiceReference(const std::string& serviceReference);

    private:
      static std::string CreateCommonServiceReference(const std::string& serviceReference);
      std::string CreateGenericServiceReference(const std::string& commonServiceReference);
      std::string CreateIconPath(const std::string& commonServiceReference);
      std::string ExtractIptvStreamURL();
      bool HasRadioServiceType();

      int m_channelNumber;
      bool m_usingDefaultChannelNumber = true;
      bool m_isIptvStream = false;
      std::string m_standardServiceReference;
      std::string m_extendedServiceReference;
      std::string m_genericServiceReference;
      std::string m_streamURL;
      std::string m_m3uURL;
      std::string m_iconPath;
      std::string m_providerName;
      int m_providerUniqueId = PVR_PROVIDER_INVALID_UID;
      std::string m_fuzzyChannelName;
      int m_streamProgramNumber;

      std::vector<std::shared_ptr<enigma2::data::ChannelGroup>> m_channelGroupList;
    };
  } //namespace data
} //namespace enigma2
