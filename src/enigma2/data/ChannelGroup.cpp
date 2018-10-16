#include "ChannelGroup.h"

#include "inttypes.h"
#include "util/XMLUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool ChannelGroup::UpdateFrom(TiXmlElement* groupNode)
{
  std::string serviceReference;
  std::string groupName;

  if (!XMLUtils::GetString(groupNode, "e2servicereference", serviceReference))
    return false;
  
  // Check whether the current element is not just a label
  if (serviceReference.compare(0,5,"1:64:") == 0)
    return false;

  if (!XMLUtils::GetString(groupNode, "e2servicename", groupName)) 
    return false;

  if (Settings::GetInstance().GetOneGroupOnly() && Settings::GetInstance().GetOneGroupName() != groupName) 
  {
    Logger::Log(LEVEL_INFO, "%s Only one group is set, but current e2servicename '%s' does not match requested name '%s'", __FUNCTION__, serviceReference.c_str(), Settings::GetInstance().GetOneGroupName().c_str());
    return false;
  }

  m_serviceReference = serviceReference;
  m_groupName = groupName;

  return true;
}

void ChannelGroup::UpdateTo(PVR_CHANNEL_GROUP &left) const
{
  left.bIsRadio = false;
  left.iPosition = 0; // groups default order, unused
  strncpy(left.strGroupName, m_groupName.c_str(), sizeof(left.strGroupName));
}