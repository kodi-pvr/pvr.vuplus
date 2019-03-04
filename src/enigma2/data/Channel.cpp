#include "Channel.h"

#include <regex>

#include "ChannelGroup.h"
#include "../Settings.h"
#include "../utilities/WebUtils.h"

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool Channel::Like(const Channel &right) const
{
  bool isLike = (m_serviceReference == right.m_serviceReference);
  isLike &= (m_channelName == right.m_channelName);

  return isLike;
}

bool Channel::operator==(const Channel &right) const
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

bool Channel::operator!=(const Channel &right) const
{
  return !(*this == right);
}

bool Channel::UpdateFrom(TiXmlElement* channelNode)
{
  if (!XMLUtils::GetString(channelNode, "e2servicereference", m_serviceReference))
    return false;

  // Check whether the current element is not just a label
  if (m_serviceReference.compare(0,5,"1:64:") == 0)
    return false;

  if (!XMLUtils::GetString(channelNode, "e2servicename", m_channelName))
    return false;

  if (m_radio != HasRadioServiceType())
    return false;

  const std::string commonServiceReference = GetCommonServiceReference(m_serviceReference);
  m_genericServiceReference = GetGenericServiceReference(commonServiceReference);
  m_iconPath = CreateIconPath(commonServiceReference);

  Logger::Log(LEVEL_DEBUG, "%s: Loaded Channel: %s, sRef=%s, picon: %s", __FUNCTION__, m_channelName.c_str(), m_serviceReference.c_str(), m_iconPath.c_str());

  m_m3uURL = StringUtils::Format("%sweb/stream.m3u?ref=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(m_serviceReference).c_str());

  m_streamURL = StringUtils::Format(
    "http%s://%s%s:%d/%s",
    Settings::GetInstance().UseSecureConnectionStream() ? "s" : "",
    Settings::GetInstance().UseLoginStream() ? StringUtils::Format("%s:%s@", Settings::GetInstance().GetUsername().c_str(), Settings::GetInstance().GetPassword().c_str()).c_str() : "",
    Settings::GetInstance().GetHostname().c_str(),
    Settings::GetInstance().GetStreamPortNum(),
    commonServiceReference.c_str()
  );

  return true;
}

std::string Channel::GetCommonServiceReference(const std::string &serviceReference)
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
  std::string::size_type index = it-commonServiceReference.begin();

  commonServiceReference = commonServiceReference.substr(0, index);

  it = commonServiceReference.end() - 1;
  if (*it == ':')
  {
    commonServiceReference.erase(it);
  }

  return commonServiceReference;
}

std::string Channel::GetGenericServiceReference(const std::string &commonServiceReference)
{
  //Same as common service reference but starts with SERVICE_REF_GENERIC_PREFIX and ends with SERVICE_REF_GENERIC_POSTFIX
  std::string genericServiceReference = commonServiceReference;

  std::regex startPrefixRegex ("^\\d+:\\d+:\\d+:");
  std::string replaceWith = "";
  genericServiceReference = regex_replace(commonServiceReference, startPrefixRegex, replaceWith);
  std::regex endPostfixRegex (":\\d+:\\d+:\\d+$");
  genericServiceReference = regex_replace(genericServiceReference, endPostfixRegex, replaceWith);
  genericServiceReference = SERVICE_REF_GENERIC_PREFIX + genericServiceReference + SERVICE_REF_GENERIC_POSTFIX;

  return genericServiceReference;
}

std::string Channel::CreateIconPath(const std::string &commonServiceReference)
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

bool Channel::HasRadioServiceType()
{
  std::string radioServiceType = m_serviceReference.substr(4, m_serviceReference.size());
  size_t found = radioServiceType.find(':');
  if (found != std::string::npos)
    radioServiceType = radioServiceType.substr(0, found);

  return radioServiceType == RADIO_SERVICE_TYPE;
}

void Channel::UpdateTo(PVR_CHANNEL &left) const
{
  left.iUniqueId = m_uniqueId;
  left.bIsRadio = m_radio;
  left.iChannelNumber = m_channelNumber;
  strncpy(left.strChannelName, m_channelName.c_str(), sizeof(left.strChannelName));
  strncpy(left.strInputFormat, "", 0); // unused
  left.iEncryptionSystem = 0;
  left.bIsHidden = false;
  strncpy(left.strIconPath, m_iconPath.c_str(), sizeof(left.strIconPath));
}

void Channel::AddChannelGroup(std::shared_ptr<ChannelGroup> &channelGroup)
{
  m_channelGroupList.emplace_back(channelGroup);
}