#include "EpgEntry.h"

#include "inttypes.h"
#include "util/XMLUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

void EpgEntry::UpdateTo(EPG_TAG &left) const
{
  left.iUniqueBroadcastId  = m_eventId;
  left.strTitle            = m_title.c_str();
  left.iUniqueChannelId    = m_channelId;
  left.startTime           = m_startTime;
  left.endTime             = m_endTime;
  left.strPlotOutline      = m_plotOutline.c_str();
  left.strPlot             = m_plot.c_str();
  left.strOriginalTitle    = nullptr; // unused
  left.strCast             = nullptr; // unused
  left.strDirector         = nullptr; // unused
  left.strWriter           = nullptr; // unused
  left.iYear               = m_year;
  left.strIMDBNumber       = nullptr; // unused
  left.strIconPath         = ""; // unused
  left.iGenreType          = m_genreType;
  left.iGenreSubType       = m_genreSubType;
  left.strGenreDescription = m_genreDescription.c_str();
  left.firstAired          = 0;  // unused
  left.iParentalRating     = 0;  // unused
  left.iStarRating         = 0;  // unused
  left.bNotify             = false;
  left.iSeriesNumber       = m_seasonNumber;
  left.iEpisodeNumber      = m_episodeNumber;
  left.iEpisodePartNumber  = m_episodePartNumber;
  left.strEpisodeName      = ""; // unused
  left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
}

bool EpgEntry::UpdateFrom(TiXmlElement* eventNode, std::map<std::string, std::shared_ptr<EpgChannel>> &epgChannelsMap)
{
  if(!XMLUtils::GetString(eventNode, "e2eventservicereference", m_serviceReference))
    return false;

  // Check whether the current element is not just a label or that it's not an empty record
  if (m_serviceReference.compare(0,5,"1:64:") == 0)
    return false;

  std::shared_ptr<data::EpgChannel> epgChannel = std::make_shared<data::EpgChannel>();

  auto epgChannelSearch = epgChannelsMap.find(m_serviceReference);
  if (epgChannelSearch != epgChannelsMap.end())
    epgChannel = epgChannelSearch->second;

  if (!epgChannel)
  {
    Logger::Log(LEVEL_DEBUG, "%s could not find channel so skipping entry", __FUNCTION__);
    return false;
  }

  m_channelId = epgChannel->GetUniqueId();

  return UpdateFrom(eventNode, epgChannel, 0, 0);
}

bool EpgEntry::UpdateFrom(TiXmlElement* eventNode, const std::shared_ptr<EpgChannel> &epgChannel, time_t iStart, time_t iEnd)
{
  std::string strTmp;

  int iTmpStart;
  int iTmp;

  // check and set event starttime and endtimes
  if (!XMLUtils::GetInt(eventNode, "e2eventstart", iTmpStart))
    return false;

  // Skip unneccessary events
  if (iStart > iTmpStart)
    return false;

  if (!XMLUtils::GetInt(eventNode, "e2eventduration", iTmp))
    return false;

  if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
    return false;

  m_startTime = iTmpStart;
  m_endTime = iTmpStart + iTmp;

  if (!XMLUtils::GetInt(eventNode, "e2eventid", iTmp))
    return false;

  m_eventId = iTmp;
  m_channelId = epgChannel->GetUniqueId();

  if(!XMLUtils::GetString(eventNode, "e2eventtitle", strTmp))
    return false;

  m_title = strTmp;

  m_serviceReference = epgChannel->GetServiceReference().c_str();

  // Check that it's not an empty record
  if (m_eventId == 0 && m_title == "None")
    return false;

  if (XMLUtils::GetString(eventNode, "e2eventdescriptionextended", strTmp))
    m_plot = strTmp;

  if (XMLUtils::GetString(eventNode, "e2eventdescription", strTmp))
      m_plotOutline = strTmp;

  ProcessPrependMode(PrependOutline::IN_EPG);

  if (XMLUtils::GetString(eventNode, "e2eventgenre", strTmp))
  {
    m_genreDescription = strTmp;

    TiXmlElement* genreNode = eventNode->FirstChildElement("e2eventgenre");
    if (genreNode)
    {
      int genreId = 0;
      if (genreNode->QueryIntAttribute("id", &genreId) == TIXML_SUCCESS)
      {
        m_genreType = genreId & 0xF0;
        m_genreSubType = genreId & 0x0F;
      }
    }
  }

  return true;
}