#include "ChannelGroup.h"

#include "inttypes.h"
#include "util/XMLUtils.h"

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

  // Check if the current element is hidden, a separator or last scanned
  if (groupName == "<n/a>" || groupName.compare(groupName.length() - 12, 12, " - Separator") == 0 || groupName == "Last Scanned")
    return false;

  if (!radio && Settings::GetInstance().GetOneTVGroupOnly() && Settings::GetInstance().GetOneTVGroupName() != groupName)
  {
    Logger::Log(LEVEL_DEBUG, "%s Only one TV group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, serviceReference.c_str(), Settings::GetInstance().GetOneTVGroupName().c_str());
    return false;
  } 
  else if (radio && Settings::GetInstance().GetOneRadioGroupOnly() && Settings::GetInstance().GetOneRadioGroupName() != groupName)
  {
    Logger::Log(LEVEL_DEBUG, "%s Only one Radio group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, serviceReference.c_str(), Settings::GetInstance().GetOneRadioGroupName().c_str());
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

void ChannelGroup::AddChannel(ChannelPtr channel)
{
  m_channelList.emplace_back(channel);
}