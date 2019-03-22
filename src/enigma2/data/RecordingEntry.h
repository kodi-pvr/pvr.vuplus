#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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

#include <string>

#include "BaseEntry.h"
#include "Channel.h"
#include "Tags.h"
#include "../Channels.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"
namespace enigma2
{
  namespace data
  {
    static const std::string TAG_FOR_PLAY_COUNT = "PlayCount";
    static const std::string TAG_FOR_LAST_PLAYED = "LastPlayed";
    static const std::string TAG_FOR_NEXT_SYNC_TIME = "NextSyncTime";

    class RecordingEntry : public BaseEntry, public Tags
    {
    public:
      const std::string& GetRecordingId() const { return m_recordingId; }
      void SetRecordingId(const std::string& value ) { m_recordingId = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      int GetDuration() const { return m_duration; }
      void SetDuration(int value) { m_duration = value; }

      int GetPlayCount() const { return m_playCount; }
      void SetPlayCount(int value) { m_playCount = value; }

      int GetLastPlayedPosition() const { return m_lastPlayedPosition; }
      void SetLastPlayedPosition(int value) { m_lastPlayedPosition = value; }

      time_t GetNextSyncTime() const { return m_nextSyncTime; }
      void SetNextSyncTime(time_t value) { m_nextSyncTime = value; }

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value ) { m_streamURL = value; }

      const std::string& GetEdlURL() const { return m_edlURL; }
      void SetEdlURL(const std::string& value ) { m_edlURL = value; }

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value ) { m_channelName = value; }

      int GetChannelUniqueId() const { return m_channelUniqueId; }
      void SetChannelUniqueId(int value) { m_channelUniqueId = value; }

      const std::string& GetDirectory() const { return m_directory; }
      void SetDirectory(const std::string& value ) { m_directory = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value ) { m_iconPath = value; }

      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      bool UpdateFrom(TiXmlElement* recordingNode, const std::string &directory, enigma2::Channels &channels);
      void UpdateTo(PVR_RECORDING &left, Channels &channels, bool isInRecordingFolder);

    private:
      long TimeStringToSeconds(const std::string &timeString);

      std::shared_ptr<Channel> FindChannel(enigma2::Channels &channels) const;
      std::shared_ptr<Channel> GetChannelFromChannelReferenceTag(enigma2::Channels &channels) const;
      std::shared_ptr<Channel> GetChannelFromChannelNameSearch(enigma2::Channels &channels) const;
      std::shared_ptr<Channel> GetChannelFromChannelNameFuzzySearch(enigma2::Channels &channels) const;

      std::string m_recordingId;
      time_t m_startTime;
      int m_duration;
      int m_playCount = 0;
      int m_lastPlayedPosition = 0;
      time_t m_nextSyncTime = 0;
      std::string m_streamURL;
      std::string m_edlURL;
      std::string m_channelName;
      int m_channelUniqueId = PVR_CHANNEL_INVALID_UID;
      std::string m_directory;
      std::string m_iconPath;
      mutable bool m_radio = false;
      mutable bool m_haveChannelType = false;
      mutable bool m_anyChannelTimerSource = false;
    };
  } //namespace data
} //namespace enigma2