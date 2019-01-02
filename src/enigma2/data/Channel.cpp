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

bool Channel::UpdateFrom(TiXmlElement* channelNode, const std::string &enigmaURL)
{
  if (!XMLUtils::GetString(channelNode, "e2servicereference", m_serviceReference))
    return false;
  
  // Check whether the current element is not just a label
  if (m_serviceReference.compare(0,5,"1:64:") == 0)
    return false;

  if (!XMLUtils::GetString(channelNode, "e2servicename", m_channelName)) 
    return false;

  Logger::Log(LEVEL_DEBUG, "%s: Loaded Channel: %s, sRef=%s", __FUNCTION__, m_channelName.c_str(), m_serviceReference.c_str());

  std::string iconPath = m_serviceReference;

  int j = 0;
  std::string::iterator it = iconPath.begin();

  while (j < 10 && it != iconPath.end())
  {
    if (*it == ':')
      j++;

    it++;
  }
  std::string::size_type index = it-iconPath.begin();

  iconPath = iconPath.substr(0, index);

  it = iconPath.end() - 1;
  if (*it == ':')
  {
    iconPath.erase(it);
  }

  if (Settings::GetInstance().UsePiconsEuFormat())
  {
    //Extract the unique part of the icon name and apply the standard pre and post-fix
    std::regex startPrefixRegex ("^\\d+:\\d+:\\d+:");
    std::string replaceWith = "";
    iconPath = regex_replace(iconPath, startPrefixRegex, replaceWith);
    std::regex endPostfixRegex (":\\d+:\\d+:\\d+$");
    iconPath = regex_replace(iconPath, endPostfixRegex, replaceWith);
    iconPath = SERVICE_REF_ICON_PREFIX + iconPath + SERVICE_REF_ICON_POSTFIX;
  }
  
  std::string tempString = StringUtils::Format("%s", iconPath.c_str());

  std::replace(iconPath.begin(), iconPath.end(), ':', '_');
  iconPath = Settings::GetInstance().GetIconPath().c_str() + iconPath + ".png";

  m_m3uURL = StringUtils::Format("%sweb/stream.m3u?ref=%s", enigmaURL.c_str(), WebUtils::URLEncodeInline(m_serviceReference).c_str());

  m_streamURL = StringUtils::Format(
    "http%s://%s%s:%d/%s",
    Settings::GetInstance().UseSecureConnectionStream() ? "s" : "",
    Settings::GetInstance().UseLoginStream() ? StringUtils::Format("%s:%s@", Settings::GetInstance().GetUsername().c_str(), Settings::GetInstance().GetPassword().c_str()).c_str() : "",
    Settings::GetInstance().GetHostname().c_str(),
    Settings::GetInstance().GetStreamPortNum(),
    tempString.c_str()
  );

  if (Settings::GetInstance().UseOnlinePicons())
  {
    std::replace(tempString.begin(), tempString.end(), ':', '_');
    iconPath = StringUtils::Format("%spicon/%s.png", enigmaURL.c_str(), tempString.c_str());
  }
  m_iconPath = iconPath;

  return true;
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