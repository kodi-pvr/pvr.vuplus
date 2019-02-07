#include "ChannelGroup.h"

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool ChannelGroup::UpdateFrom(TiXmlElement* groupNode, bool radio)
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

  if (!radio && Settings::GetInstance().GetTVChannelGroupMode() == ChannelGroupMode::ONLY_ONE_GROUP)
  {
    if (Settings::GetInstance().GetOneTVGroupName() != groupName)
    {
      Logger::Log(LEVEL_DEBUG, "%s Only one TV group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, groupName.c_str(), Settings::GetInstance().GetOneTVGroupName().c_str());
      return false;
    }
  }
  else if (radio && Settings::GetInstance().GetRadioChannelGroupMode() == ChannelGroupMode::ONLY_ONE_GROUP)
  {
    if (Settings::GetInstance().GetOneRadioGroupName() != groupName)
    {
      Logger::Log(LEVEL_DEBUG, "%s Only one Radio group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, groupName.c_str(), Settings::GetInstance().GetOneRadioGroupName().c_str());
      return false;
    }
  }
  else if (groupName == "Last Scanned")
  {
    // Last scanned contains TV and Radio channels mixed, therefore we discard here and handle it separately,
    // similar to Favourites when loading channel groups.
    // Note that if Last Scanned is used in only one group for TV we do not discard and only allow it for TV
    // as it is never supplied as a radio group from Enigma2
    return false;
  }

  m_serviceReference = serviceReference;
  m_groupName = groupName;
  m_radio = radio;

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