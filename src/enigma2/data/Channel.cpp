/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Channel.h"

#include "../Settings.h"
#include "../utilities/WebUtils.h"
#include "../utilities/XMLUtils.h"
#include "ChannelGroup.h"

#include <cinttypes>
#include <regex>

#include <kodi/tools/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using namespace kodi::tools;

bool Channel::Like(const Channel& right) const
{
  bool isLike = (m_serviceReference == right.m_serviceReference);
  isLike &= (m_channelName == right.m_channelName);

  return isLike;
}

bool Channel::operator==(const Channel& right) const
{
  bool isEqual = (m_serviceReference == right.m_serviceReference);
  isEqual &= (m_channelName == right.m_channelName);
  isEqual &= (m_radio == right.m_radio);
  isEqual &= (m_genericServiceReference == right.m_genericServiceReference);
  isEqual &= (m_streamURL == right.m_streamURL);
  isEqual &= (m_m3uURL == right.m_m3uURL);
  isEqual &= (m_iconPath == right.m_iconPath);
  isEqual &= (m_providerName == right.m_providerName);

  return isEqual;
}

bool Channel::operator!=(const Channel& right) const
{
  return !(*this == right);
}

bool Channel::UpdateFrom(TiXmlElement* channelNode)
{
  if (!xml::GetString(channelNode, "e2servicereference", m_serviceReference))
    return false;

  // Check whether the current element is not just a label or that it's not a hidden entry
  if (m_serviceReference.compare(0, 5, "1:64:") == 0 || m_serviceReference.compare(0, 6, "1:320:") == 0)
    return false;

  if (!xml::GetString(channelNode, "e2servicename", m_channelName))
    return false;

  m_fuzzyChannelName = m_channelName;

  // We need to correctly cast to unsigned char as for some platforms such as windows it will
  // fail on a negative value as it will be treated as an int instead of character
  auto func = [](char c) { return isspace(static_cast<unsigned char>(c)); };
  // alternatively this can be done as follows:
  //auto func = [](unsigned char const c) { return isspace(std::char_traits<char>::to_int_type(c)); };
  m_fuzzyChannelName.erase(std::remove_if(m_fuzzyChannelName.begin(), m_fuzzyChannelName.end(), func), m_fuzzyChannelName.end());

  if (m_radio != HasRadioServiceType())
    return false;

  // Automatically convert stream relay service references to satellite service references
  // There should be no downstream impact to other providers (hopefully!)
  if (StringUtils::EndsWith(m_serviceReference, STREAM_REPLAY_SERVICE_REFERENCE_POSTFIX))
  {
    static const std::regex endPostfixRegex(STREAM_REPLAY_SERVICE_REFERENCE_POSTFIX + "$");
    std::string replaceWith = "";
    std::string m_satelliteServiceReference = std::regex_replace(m_serviceReference, endPostfixRegex, replaceWith) + SAT_SERVICE_REFERENCE_POSTFIX;

    Logger::Log(LEVEL_DEBUG, "%s: Converted Stream Replay to Satellie Reference: %s, sRef=%s", __func__, m_channelName.c_str(), m_satelliteServiceReference.c_str());
    m_serviceReference = m_satelliteServiceReference;
  }

  m_extendedServiceReference = m_serviceReference;
  const std::string commonServiceReference = CreateCommonServiceReference(m_serviceReference);
  m_standardServiceReference = commonServiceReference + ":";
  m_genericServiceReference = CreateGenericServiceReference(commonServiceReference);
  m_iconPath = CreateIconPath(commonServiceReference);

  const std::string iptvStreamURL = ExtractIptvStreamURL();

  Settings& settings = Settings::GetInstance();
  if (settings.UseStandardServiceReference())
    m_serviceReference = m_standardServiceReference;

  std::sscanf(m_serviceReference.c_str(), "%*X:%*X:%*X:%X:%*s", &m_streamProgramNumber);

  Logger::Log(LEVEL_DEBUG, "%s: Loaded Channel: %s, sRef=%s, picon: %s, program number: %d", __func__, m_channelName.c_str(), m_serviceReference.c_str(), m_iconPath.c_str(), m_streamProgramNumber);

  if (m_isIptvStream)
  {
    Logger::Log(LEVEL_DEBUG, "%s: Loaded Channel: %s, sRef=%s, IPTV Stream URL: %s", __func__, m_channelName.c_str(), m_serviceReference.c_str(), iptvStreamURL.c_str());
  }

  m_m3uURL = StringUtils::Format("%sweb/stream.m3u?ref=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(m_serviceReference).c_str());

  if (!m_isIptvStream)
  {
    m_streamURL = StringUtils::Format(
      "http%s://%s%s:%d/%s",
      settings.UseSecureConnectionStream() ? "s" : "",
      settings.UseLoginStream() ? StringUtils::Format("%s:%s@", settings.GetUsername().c_str(), settings.GetPassword().c_str()).c_str() : "",
      settings.GetHostname().c_str(),
      settings.GetStreamPortNum(),
      commonServiceReference.c_str()
    );
  }
  else
  {
    m_streamURL = iptvStreamURL;
  }

  return true;
}

std::string Channel::NormaliseServiceReference(const std::string& serviceReference)
{
  if (Settings::GetInstance().UseStandardServiceReference())
    return CreateStandardServiceReference(serviceReference);
  else
    return serviceReference;
}

std::string Channel::CreateStandardServiceReference(const std::string& serviceReference)
{
  return CreateCommonServiceReference(serviceReference) + ":";
}

std::string Channel::CreateCommonServiceReference(const std::string& serviceReference)
{
  //The common service reference contains only the first 10 groups of digits with colon's in between
  std::string commonServiceReference = serviceReference;

  int j = 0;
  std::string::iterator it = commonServiceReference.begin();

  while (j < 10 && it != commonServiceReference.end())
  {
    if (*it == ':')
      j++;

    it++;
  }
  std::string::size_type index = it - commonServiceReference.begin();

  commonServiceReference = commonServiceReference.substr(0, index);

  it = commonServiceReference.end() - 1;
  if (*it == ':')
  {
    commonServiceReference.erase(it);
  }

  return commonServiceReference;
}

std::string Channel::CreateGenericServiceReference(const std::string& commonServiceReference)
{
  //Same as common service reference but starts with SERVICE_REF_GENERIC_PREFIX and ends with SERVICE_REF_GENERIC_POSTFIX
  static const std::regex startPrefixRegex("^\\d+:\\d+:\\d+:");
  std::string replaceWith = "";
  std::string genericServiceReference = std::regex_replace(commonServiceReference, startPrefixRegex, replaceWith);
  static const std::regex endPostfixRegex(":\\d+:\\d+:\\d+$");
  genericServiceReference = std::regex_replace(genericServiceReference, endPostfixRegex, replaceWith);
  genericServiceReference = SERVICE_REF_GENERIC_PREFIX + genericServiceReference + SERVICE_REF_GENERIC_POSTFIX;

  return genericServiceReference;
}

std::string Channel::CreateIconPath(const std::string& commonServiceReference)
{
  std::string iconPath = commonServiceReference;

  if (Settings::GetInstance().UsePiconsEuFormat())
  {
    iconPath = m_genericServiceReference;
  }

  std::replace(iconPath.begin(), iconPath.end(), ':', '_');

  if (Settings::GetInstance().UseOnlinePicons())
    iconPath = StringUtils::Format("%spicon/%s.png", Settings::GetInstance().GetConnectionURL().c_str(), iconPath.c_str());
  else
    iconPath = Settings::GetInstance().GetIconPath().c_str() + iconPath + ".png";

  return iconPath;
}

std::string Channel::ExtractIptvStreamURL()
{
  std::string iptvStreamURL;

  std::size_t found = m_extendedServiceReference.find(m_standardServiceReference);
  if (found != std::string::npos)
  {
    const std::string possibleIptvStreamURL = m_extendedServiceReference.substr(m_standardServiceReference.length());
    found = possibleIptvStreamURL.find("%3a"); //look for an URL encoded colon which means we have an embedded URL
    if (found != std::string::npos)
    {
      m_isIptvStream = true;
      iptvStreamURL = possibleIptvStreamURL;
      std::size_t foundColon = iptvStreamURL.find_last_of(":");
      if (foundColon != std::string::npos)
      {
        iptvStreamURL = iptvStreamURL.substr(0, foundColon);
      }
      static const std::regex escapedColonRegex("%3a");
      iptvStreamURL = std::regex_replace(iptvStreamURL, escapedColonRegex, ":");
    }
  }

  return iptvStreamURL;
}

bool Channel::HasRadioServiceType()
{
  std::string radioServiceType = m_serviceReference.substr(4, m_serviceReference.size());
  size_t found = radioServiceType.find(':');
  if (found != std::string::npos)
    radioServiceType = radioServiceType.substr(0, found);

  return std::find(RADIO_SERVICE_TYPES.begin(), RADIO_SERVICE_TYPES.end(), radioServiceType) != RADIO_SERVICE_TYPES.end();
}

void Channel::UpdateTo(kodi::addon::PVRChannel& left) const
{
  left.SetUniqueId(m_uniqueId);
  left.SetIsRadio(m_radio);
  left.SetChannelNumber(m_channelNumber);
  left.SetChannelName(m_channelName);
  left.SetEncryptionSystem(0);
  left.SetIsHidden(false);
  left.SetIconPath(m_iconPath);
}

void Channel::AddChannelGroup(std::shared_ptr<ChannelGroup>& channelGroup)
{
  m_channelGroupList.emplace_back(channelGroup);
}
