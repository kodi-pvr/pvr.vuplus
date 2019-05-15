#include "ChannelGroup.h"

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool ChannelGroup::Like(const ChannelGroup &right) const
{
  bool isLike = (m_serviceReference == right.m_serviceReference);
  isLike &= (m_groupName == right.m_groupName);

  return isLike;
}

bool ChannelGroup::operator==(const ChannelGroup &right) const
{
  bool isEqual = (m_serviceReference == right.m_serviceReference);
  isEqual &= (m_groupName == right.m_groupName);
  isEqual &= (m_radio == right.m_radio);
  isEqual &= (m_lastScannedGroup == right.m_lastScannedGroup);

  for (int i = 0; i < m_channelList.size(); i++)
  {
    isEqual &= (*(m_channelList.at(i)) == *(right.m_channelList.at(i)));

    if (!isEqual)
      break;
  }

  return isEqual;
}

bool ChannelGroup::operator!=(const ChannelGroup &right) const
{
  return !(*this == right);
}

bool ChannelGroup::UpdateFrom(TiXmlElement* groupNode, bool radio, std::vector<std::shared_ptr<ChannelGroup>> &extraDataChannelGroups)
{
  std::string serviceReference;
  std::string groupName;

  if (!XMLUtils::GetString(groupNode, "e2servicereference", serviceReference))
    return false;

  // Check whether the current element is not just a label
  if (serviceReference.compare(0, 5, "1:64:") == 0)
    return false;

  if (!XMLUtils::GetString(groupNode, "e2servicename", groupName))
    return false;

  // Check if the current element is hidden or a separator
  if (groupName == "<n/a>" || StringUtils::EndsWith(groupName.c_str(), " - Separator"))
    return false;

  m_serviceReference = serviceReference;
  m_groupName = groupName;
  m_radio = radio;

  // we keep this group even if it get's discarded so we can calculate backend channel numbers later on
  extraDataChannelGroups.emplace_back(new ChannelGroup(*this));

  if (!radio && (Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::SOME_GROUPS ||
                 Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::CUSTOM_GROUPS))
  {
    auto &customGroupNamelist = Settings::GetInstance().GetCustomTVChannelGroupNameList();
    auto it = std::find_if(customGroupNamelist.begin(), customGroupNamelist.end(),
      [&groupName](std::string& customGroupName) { return customGroupName == groupName; });

    if (it == customGroupNamelist.end())
      return false;
    else
      Logger::Log(LEVEL_DEBUG, "%s Custom TV groups are set, current e2servicename '%s' matched", __FUNCTION__, groupName.c_str());
  }
  else if (radio && (Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::SOME_GROUPS ||
                     Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::CUSTOM_GROUPS))
  {
    auto &customGroupNamelist = Settings::GetInstance().GetCustomRadioChannelGroupNameList();
    auto it = std::find_if(customGroupNamelist.begin(), customGroupNamelist.end(),
      [&groupName](std::string& customGroupName) { return customGroupName == groupName; });

    if (it == customGroupNamelist.end())
      return false;
    else
      Logger::Log(LEVEL_DEBUG, "%s Custom Radio groups are set, current e2servicename '%s' matched", __FUNCTION__, groupName.c_str());
  }
  else if (groupName == "Last Scanned")
  {
    // Last scanned contains TV and Radio channels mixed, therefore we discard here and handle it separately,
    // similar to Favourites when loading channel groups.
    // Note that if Last Scanned is used in only one group for TV we do not discard and only allow it for TV
    // as it is never supplied as a radio group from Enigma2
    return false;
  }

  return true;
}

void ChannelGroup::UpdateTo(PVR_CHANNEL_GROUP &left) const
{
  left.bIsRadio = m_radio;
  left.iPosition = 0; // groups default order, unused
  strncpy(left.strGroupName, m_groupName.c_str(), sizeof(left.strGroupName));
}

void ChannelGroup::AddChannel(std::shared_ptr<Channel> channel)
{
  m_channelList.emplace_back(channel);
}