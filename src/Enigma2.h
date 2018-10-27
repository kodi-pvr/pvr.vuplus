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

#include "client.h"
#include "enigma2/Admin.h"
#include "enigma2/Channels.h"
#include "enigma2/ChannelGroups.h"
#include "enigma2/Epg.h"
#include "enigma2/RecordingReader.h"
#include "enigma2/Recordings.h"
#include "enigma2/Settings.h"
#include "enigma2/Timers.h"
#include "enigma2/data/BaseEntry.h"
#include "enigma2/data/Channel.h"
#include "enigma2/data/ChannelGroup.h"
#include "enigma2/data/EpgEntry.h"
#include "enigma2/data/RecordingEntry.h"
#include "enigma2/extract/EpgEntryExtractor.h"

#include "tinyxml.h"
#include "p8-platform/threads/threads.h"

class Enigma2  : public P8PLATFORM::CThread
{
public:
  Enigma2();
  ~Enigma2();

  //device and helper functions
  bool Open();
  void SendPowerstate();
  const char * GetServerName() const;
  const char * GetServerVersion() const;
  bool IsConnected() const; 
  
  //groups, channels and EPG
  unsigned int GetNumChannelGroups(void) const;
  PVR_ERROR    GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  int GetChannelsAmount(void) const;
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  //live streams, recordings and Timers
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  const std::string GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  unsigned int GetRecordingsAmount();
  PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  bool GetRecordingsFromLocation(std::string strRecordingFolder);
  enigma2::RecordingReader *OpenRecordedStream(const PVR_RECORDING &recinfo);
  void GetTimerTypes(PVR_TIMER_TYPE types[], int *size);
  int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);

protected:
  virtual void *Process(void);

private:
  // helper functions
  std::string GetStreamURL(const std::string& strM3uURL);

  // members
  bool m_isConnected = false;
  unsigned int m_updateTimer = 0;
  int m_currentChannel = -1;

  enigma2::Channels m_channels;
  enigma2::ChannelGroups m_channelGroups;
  enigma2::Recordings m_recordings = enigma2::Recordings(m_channels, m_entryExtractor);
  std::vector<std::string>& m_locations = m_recordings.GetLocations();
  enigma2::Timers m_timers = enigma2::Timers(m_channels, m_locations);
  enigma2::Settings &m_settings = enigma2::Settings::GetInstance();
  enigma2::Admin m_admin;
  enigma2::Epg m_epg = enigma2::Epg(m_channels, m_channelGroups, m_entryExtractor);
  enigma2::extract::EpgEntryExtractor m_entryExtractor;

  P8PLATFORM::CMutex m_mutex;
  P8PLATFORM::CCondition<bool> m_started;
};