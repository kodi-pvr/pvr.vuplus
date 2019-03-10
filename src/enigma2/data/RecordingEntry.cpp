#include "RecordingEntry.h"

#include "../utilities/WebUtils.h"

#include "inttypes.h"
#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool RecordingEntry::UpdateFrom(TiXmlElement* recordingNode, const std::string &directory, Channels &channels)
{
  std::string strTmp;
  int iTmp;

  m_directory = directory;

  m_lastPlayedPosition = 0;
  if (XMLUtils::GetString(recordingNode, "e2servicereference", strTmp))
    m_recordingId = strTmp;

  if (XMLUtils::GetString(recordingNode, "e2title", strTmp))
    m_title = strTmp;

  if (XMLUtils::GetString(recordingNode, "e2description", strTmp))
    m_plotOutline = strTmp;

  if (XMLUtils::GetString(recordingNode, "e2descriptionextended", strTmp))
    m_plot = strTmp;

  if (XMLUtils::GetString(recordingNode, "e2servicename", strTmp))
    m_channelName = strTmp;

  m_iconPath = channels.GetChannelIconPath(strTmp);

  if (XMLUtils::GetInt(recordingNode, "e2time", iTmp))
    m_startTime = iTmp;

  if (XMLUtils::GetString(recordingNode, "e2length", strTmp))
  {
    iTmp = TimeStringToSeconds(strTmp.c_str());
    m_duration = iTmp;
  }
  else
    m_duration = 0;

  if (XMLUtils::GetString(recordingNode, "e2filename", strTmp))
  {
    m_edlURL = strTmp;

    strTmp = StringUtils::Format("%sfile?file=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(strTmp).c_str());
    m_streamURL = strTmp;

    m_edlURL = m_edlURL.substr(0, m_edlURL.find_last_of('.')) + ".edl";
    m_edlURL = StringUtils::Format("%sfile?file=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(m_edlURL).c_str());
  }

  ProcessPrependMode(PrependOutline::IN_RECORDINGS);

  m_tags.clear();
  if (XMLUtils::GetString(recordingNode, "e2tags", strTmp))
    m_tags = strTmp;

  if (ContainsTag(TAG_FOR_GENRE_ID))
  {
    int genreId = 0;
    if (std::sscanf(ReadTagValue(TAG_FOR_GENRE_ID).c_str(), "0x%02X", &genreId) == 1)
    {
      m_genreType = genreId & 0xF0;
      m_genreSubType = genreId & 0x0F;
    }
    else
    {
      m_genreType = 0;
      m_genreSubType = 0;
    }
  }

  return true;
}

long RecordingEntry::TimeStringToSeconds(const std::string &timeString)
{
  std::vector<std::string> tokens;

  std::string s = timeString;
  const std::string delimiter = ":";

  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    tokens.emplace_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.emplace_back(s);

  int timeInSecs = 0;

  if (tokens.size() == 2)
  {
    timeInSecs += atoi(tokens[0].c_str()) * 60;
    timeInSecs += atoi(tokens[1].c_str());
  }

  return timeInSecs;
}

void RecordingEntry::UpdateTo(PVR_RECORDING &left, Channels &channels, bool isInRecordingFolder)
{
  std::string strTmp;
  strncpy(left.strRecordingId, m_recordingId.c_str(), sizeof(left.strRecordingId));
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle));
  strncpy(left.strPlotOutline, m_plotOutline.c_str(), sizeof(left.strPlotOutline));
  strncpy(left.strPlot, m_plot.c_str(), sizeof(left.strPlot));
  strncpy(left.strChannelName, m_channelName.c_str(), sizeof(left.strChannelName));
  strncpy(left.strIconPath, m_iconPath.c_str(), sizeof(left.strIconPath));

  if (!Settings::GetInstance().GetKeepRecordingsFolders())
  {
    if(isInRecordingFolder)
      strTmp = StringUtils::Format("/%s/", m_title.c_str());
    else
      strTmp = StringUtils::Format("/");

    m_directory = strTmp;
  }

  strncpy(left.strDirectory, m_directory.c_str(), sizeof(left.strDirectory));
  left.recordingTime     = m_startTime;
  left.iDuration         = m_duration;

  left.iChannelUid = PVR_CHANNEL_INVALID_UID;
  left.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

  for (const auto& channel : channels.GetChannelsList())
  {
    if (m_channelName == channel->GetChannelName())
    {
      /* PVR API 5.0.0: iChannelUid in recordings */
      left.iChannelUid = channel->GetUniqueId();

      /* PVR API 5.1.0: Support channel type in recordings */
      if (channel->IsRadio())
        left.channelType = PVR_RECORDING_CHANNEL_TYPE_RADIO;
      else
        left.channelType = PVR_RECORDING_CHANNEL_TYPE_TV;
    }
  }

  left.iSeriesNumber = m_seasonNumber;
  left.iEpisodeNumber = m_episodeNumber;
  left.iYear = m_year;
  left.iGenreType = m_genreType;
  left.iGenreSubType = m_genreSubType;
  strncpy(left.strGenreDescription, m_genreDescription.c_str(), sizeof(left.strGenreDescription));
}