/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "RecordingEntry.h"

#include "../utilities/StringUtils.h"
#include "../utilities/WebUtils.h"
#include "../utilities/XMLUtils.h"

#include <cinttypes>
#include <cstdlib>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::localtime(&time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", tm);

  return buffer;
}

} // unnamed namespace

bool RecordingEntry::UpdateFrom(TiXmlElement* recordingNode, const std::string& directory, bool deleted, Channels& channels)
{
  std::string strTmp;
  int iTmp;

  m_directory = directory;
  m_deleted = deleted;

  if (xml::GetString(recordingNode, "e2servicereference", strTmp))
    m_recordingId = strTmp;

  if (xml::GetString(recordingNode, "e2title", strTmp))
    m_title = strTmp;

  if (xml::GetString(recordingNode, "e2description", strTmp))
    m_plotOutline = strTmp;

  if (xml::GetString(recordingNode, "e2descriptionextended", strTmp))
    m_plot = strTmp;

  if (xml::GetString(recordingNode, "e2servicename", strTmp))
    m_channelName = strTmp;

  if (xml::GetInt(recordingNode, "e2time", iTmp))
  {
    m_startTime = iTmp;
    m_startTimeW3CDateString = ParseAsW3CDateString(m_startTime);
  }

  if (xml::GetString(recordingNode, "e2length", strTmp))
  {
    iTmp = TimeStringToSeconds(strTmp.c_str());
    m_duration = iTmp;
  }
  else
    m_duration = 0;

  if (xml::GetString(recordingNode, "e2filename", strTmp))
  {
    m_edlURL = strTmp;

    strTmp = StringUtils::Format("%sfile?file=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(strTmp).c_str());
    m_streamURL = strTmp;

    m_edlURL = m_edlURL.substr(0, m_edlURL.find_last_of('.')) + ".edl";
    m_edlURL = StringUtils::Format("%sfile?file=%s", Settings::GetInstance().GetConnectionURL().c_str(), WebUtils::URLEncodeInline(m_edlURL).c_str());
  }

  double filesizeInBytes;
  if (xml::GetDouble(recordingNode, "e2filesize", filesizeInBytes))
    m_sizeInBytes = static_cast<int64_t>(filesizeInBytes);

  ProcessPrependMode(PrependOutline::IN_RECORDINGS);

  m_tags.clear();
  if (xml::GetString(recordingNode, "e2tags", strTmp))
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

void RecordingEntry::UpdateTo(kodi::addon::PVRRecording& left, Channels& channels, bool isInRecordingFolder)
{
  std::string strTmp;
  left.SetRecordingId(m_recordingId);
  left.SetTitle(m_title);
  left.SetPlotOutline(m_plotOutline);
  left.SetPlot(m_plot);
  left.SetChannelName(m_channelName);
  left.SetIconPath(m_iconPath);

  if (!Settings::GetInstance().GetKeepRecordingsFolders())
  {
    if (isInRecordingFolder)
      strTmp = StringUtils::Format("/%s/", m_title.c_str());
    else
      strTmp = StringUtils::Format("/");

    m_directory = strTmp;
  }

  left.SetDirectory(m_directory);
  left.SetIsDeleted(m_deleted);
  left.SetRecordingTime(m_startTime);
  left.SetDuration(m_duration);

  left.SetChannelUid(m_channelUniqueId);
  left.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_UNKNOWN);

  if (m_haveChannelType)
  {
    if (m_radio)
      left.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_RADIO);
    else
      left.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_TV);
  }

  left.SetPlayCount(m_playCount);
  left.SetLastPlayedPosition(m_lastPlayedPosition);
  left.SetSeriesNumber(m_seasonNumber);
  left.SetEpisodeNumber(m_episodeNumber);
  left.SetYear(m_year);
  left.SetGenreType(m_genreType);
  left.SetGenreSubType(m_genreSubType);
  left.SetGenreDescription(m_genreDescription);

  // If this recording was new when it aired, then we can infer the first aired date
  if (m_new || m_live || m_premiere)
    left.SetFirstAired(m_startTimeW3CDateString);

  unsigned int flags = PVR_RECORDING_FLAG_UNDEFINED;
  if (m_new)
    flags |= PVR_RECORDING_FLAG_IS_NEW;
  if (m_premiere)
    flags |= PVR_RECORDING_FLAG_IS_PREMIERE;
  if (m_live)
    flags |= PVR_RECORDING_FLAG_IS_LIVE;
  left.SetFlags(flags);

  left.SetSizeInBytes(m_sizeInBytes);
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
    // We need to correctly cast to unsigned char as for some platforms such as windows it will
    // fail on a negative value as it will be treated as an int instead of character
    auto func = [](char c) { return isspace(static_cast<unsigned char>(c)); };
    // alternatively this can be done as follows:
    //auto func = [](unsigned char const c) { return isspace(std::char_traits<char>::to_int_type(c)); };
    fuzzyRecordingChannelName.erase(remove_if(fuzzyRecordingChannelName.begin(), fuzzyRecordingChannelName.end(), func), fuzzyRecordingChannelName.end());

    if (fuzzyRecordingChannelName == channel->GetFuzzyChannelName() && (!m_haveChannelType || (channel->IsRadio() == m_radio)))
    {
      return channel;
    }
  }

  return nullptr;
}
