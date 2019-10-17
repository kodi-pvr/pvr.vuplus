/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "RecordingEntry.h"

#include "../utilities/WebUtils.h"

#include <cinttypes>
#include <cstdlib>

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool RecordingEntry::UpdateFrom(TiXmlElement* recordingNode, const std::string& directory, bool deleted, Channels& channels)
{
  std::string strTmp;
  int iTmp;

  m_directory = directory;
  m_deleted = deleted;

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

  if (ContainsTag(TAG_FOR_PLAY_COUNT))
  {
    if (std::sscanf(ReadTagValue(TAG_FOR_PLAY_COUNT).c_str(), "%d", &m_playCount) != 1)
      m_playCount = 0;
  }

  if (ContainsTag(TAG_FOR_LAST_PLAYED))
  {
    if (std::sscanf(ReadTagValue(TAG_FOR_LAST_PLAYED).c_str(), "%d", &m_lastPlayedPosition) != 1)
      m_lastPlayedPosition = 0;
  }

  if (ContainsTag(TAG_FOR_NEXT_SYNC_TIME))
  {
    long long scannedTime = 0;
    if (std::sscanf(ReadTagValue(TAG_FOR_NEXT_SYNC_TIME).c_str(), "%lld", &scannedTime) != 1)
      m_nextSyncTime = 0;
    else
      m_nextSyncTime = static_cast<time_t>(scannedTime);

  }

  auto channel = FindChannel(channels);

  if (channel)
  {
    m_radio = channel->IsRadio();
    m_channelUniqueId = channel->GetUniqueId();
    m_iconPath = channel->GetIconPath();
    m_haveChannelType = true;
  }

  return true;
}

long RecordingEntry::TimeStringToSeconds(const std::string& timeString)
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
    timeInSecs += std::atoi(tokens[0].c_str()) * 60;
    timeInSecs += std::atoi(tokens[1].c_str());
  }

  return timeInSecs;
}

void RecordingEntry::UpdateTo(PVR_RECORDING& left, Channels& channels, bool isInRecordingFolder)
{
  std::string strTmp;
  strncpy(left.strRecordingId, m_recordingId.c_str(), sizeof(left.strRecordingId) - 1);
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle) - 1);
  strncpy(left.strPlotOutline, m_plotOutline.c_str(), sizeof(left.strPlotOutline) - 1);
  strncpy(left.strPlot, m_plot.c_str(), sizeof(left.strPlot) - 1);
  strncpy(left.strChannelName, m_channelName.c_str(), sizeof(left.strChannelName) - 1);
  strncpy(left.strIconPath, m_iconPath.c_str(), sizeof(left.strIconPath) - 1);

  if (!Settings::GetInstance().GetKeepRecordingsFolders())
  {
    if (isInRecordingFolder)
      strTmp = StringUtils::Format("/%s/", m_title.c_str());
    else
      strTmp = StringUtils::Format("/");

    m_directory = strTmp;
  }

  strncpy(left.strDirectory, m_directory.c_str(), sizeof(left.strDirectory) - 1);
  left.bIsDeleted = m_deleted;
  left.recordingTime = m_startTime;
  left.iDuration = m_duration;

  left.iChannelUid = m_channelUniqueId;
  left.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

  if (m_haveChannelType)
  {
    if (m_radio)
      left.channelType = PVR_RECORDING_CHANNEL_TYPE_RADIO;
    else
      left.channelType = PVR_RECORDING_CHANNEL_TYPE_TV;
  }

  left.iPlayCount = m_playCount;
  left.iLastPlayedPosition = m_lastPlayedPosition;
  left.iSeriesNumber = m_seasonNumber;
  left.iEpisodeNumber = m_episodeNumber;
  left.iYear = m_year;
  left.iGenreType = m_genreType;
  left.iGenreSubType = m_genreSubType;
  strncpy(left.strGenreDescription, m_genreDescription.c_str(), sizeof(left.strGenreDescription) - 1);
}

std::shared_ptr<Channel> RecordingEntry::FindChannel(Channels& channels) const
{
  std::shared_ptr<Channel> channel = GetChannelFromChannelReferenceTag(channels);

  if (channel)
    return channel;

  if (ContainsTag(TAG_FOR_CHANNEL_TYPE))
  {
    m_radio = ReadTagValue(TAG_FOR_CHANNEL_TYPE) == VALUE_FOR_CHANNEL_TYPE_RADIO;
    m_haveChannelType = true;
  }

  m_anyChannelTimerSource = ContainsTag(TAG_FOR_ANY_CHANNEL);

  channel = GetChannelFromChannelNameSearch(channels);

  if (channel)
  {
    if (!m_hasStreamProgramNumber)
    {
      m_streamProgramNumber = channel->GetStreamProgramNumber();
      m_hasStreamProgramNumber = true;
    }

    return channel;
  }

  channel = GetChannelFromChannelNameFuzzySearch(channels);

  if (channel && !m_hasStreamProgramNumber)
  {
    m_streamProgramNumber = channel->GetStreamProgramNumber();
    m_hasStreamProgramNumber = true;
  }

  return channel;
}

std::shared_ptr<Channel> RecordingEntry::GetChannelFromChannelReferenceTag(Channels& channels) const
{
  std::string channelServiceReference;

  if (ContainsTag(TAG_FOR_CHANNEL_REFERENCE))
  {
    channelServiceReference = Channel::NormaliseServiceReference(ReadTagValue(TAG_FOR_CHANNEL_REFERENCE, true));

    std::sscanf(channelServiceReference.c_str(), "%*X:%*X:%*X:%X:%*s", &m_streamProgramNumber);
    m_hasStreamProgramNumber = true;
  }

  return channels.GetChannel(channelServiceReference);
}

std::shared_ptr<Channel> RecordingEntry::GetChannelFromChannelNameSearch(Channels& channels) const
{
  //search for channel name using exact match
  for (const auto& channel : channels.GetChannelsList())
  {
    if (m_channelName == channel->GetChannelName() && (!m_haveChannelType || (channel->IsRadio() == m_radio)))
    {
      return channel;
    }
  }

  return nullptr;
}

std::shared_ptr<Channel> RecordingEntry::GetChannelFromChannelNameFuzzySearch(Channels& channels) const
{
  std::string fuzzyRecordingChannelName;

  //search for channel name using fuzzy match
  for (const auto& channel : channels.GetChannelsList())
  {
    fuzzyRecordingChannelName = m_channelName;
    fuzzyRecordingChannelName.erase(remove_if(fuzzyRecordingChannelName.begin(), fuzzyRecordingChannelName.end(), isspace), fuzzyRecordingChannelName.end());

    if (fuzzyRecordingChannelName == channel->GetFuzzyChannelName() && (!m_haveChannelType || (channel->IsRadio() == m_radio)))
    {
      return channel;
    }
  }

  return nullptr;
}