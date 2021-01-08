/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "enigma2/Admin.h"
#include "enigma2/ChannelGroups.h"
#include "enigma2/Channels.h"
#include "enigma2/ConnectionManager.h"
#include "enigma2/Epg.h"
#include "enigma2/IConnectionListener.h"
#include "enigma2/RecordingReader.h"
#include "enigma2/Recordings.h"
#include "enigma2/Settings.h"
#include "enigma2/StreamReader.h"
#include "enigma2/Timers.h"
#include "enigma2/data/BaseEntry.h"
#include "enigma2/data/Channel.h"
#include "enigma2/data/ChannelGroup.h"
#include "enigma2/data/EpgEntry.h"
#include "enigma2/data/RecordingEntry.h"
#include "enigma2/extract/EpgEntryExtractor.h"
#include "enigma2/utilities/SignalStatus.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <tinyxml.h>

// The windows build defines this but it breaks nlohmann/json.hpp's reference to std::snprintf
#if defined(snprintf)
#undef snprintf
#endif

class ATTRIBUTE_HIDDEN Enigma2 : public enigma2::IConnectionListener
{
public:
  Enigma2(KODI_HANDLE instance, const std::string& version);
  ~Enigma2();

  // IConnectionListener implementation
  void ConnectionLost() override;
  void ConnectionEstablished() override;

  //device and helper functions
  bool Start();
  void SendPowerstate();
  bool IsConnected() const;

  PVR_ERROR OnSystemSleep() override;
  PVR_ERROR OnSystemWake() override;

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR GetBackendHostname(std::string& hostname) override;
  PVR_ERROR GetConnectionString(std::string& connection) override;

  //groups, channels and EPG
  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results) override;
  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  PVR_ERROR GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties) override;
  PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) override;
  PVR_ERROR SetEPGMaxPastDays(int epgMaxPastDays) override;
  PVR_ERROR SetEPGMaxFutureDays(int epgMaxFutureDays) override;

  //recordings and Timers
  PVR_ERROR GetRecordingsAmount(bool deleted, int& amount) override;
  PVR_ERROR GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results) override;
  PVR_ERROR DeleteRecording(const kodi::addon::PVRRecording& recinfo) override;
  PVR_ERROR UndeleteRecording(const kodi::addon::PVRRecording& recording) override;
  PVR_ERROR DeleteAllRecordingsFromTrash() override;
  bool GetRecordingsFromLocation(std::string strRecordingFolder);
  PVR_ERROR GetRecordingEdl(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVREDLEntry>& edl) override;
  PVR_ERROR RenameRecording(const kodi::addon::PVRRecording& recording) override;
  PVR_ERROR SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count) override;
  PVR_ERROR SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int lastplayedposition) override;
  PVR_ERROR GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int& position) override;
  PVR_ERROR GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& sizeInBytes) override;
  PVR_ERROR GetRecordingStreamProperties(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVRStreamProperty>& properties) override;
  bool HasRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording);
  int GetRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording);

  PVR_ERROR GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types) override;
  PVR_ERROR GetTimersAmount(int& amount) override;
  PVR_ERROR GetTimers(kodi::addon::PVRTimersResultSet& results) override;
  PVR_ERROR AddTimer(const kodi::addon::PVRTimer& timer) override;
  PVR_ERROR UpdateTimer(const kodi::addon::PVRTimer& timer) override;
  PVR_ERROR DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete) override;
  PVR_ERROR GetDriveSpace(uint64_t& total, uint64_t& used) override;
  PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override;

  //live streams
  bool OpenLiveStream(const kodi::addon::PVRChannel& channelinfo) override;
  void CloseLiveStream() override;
  const std::string GetLiveStreamURL(const kodi::addon::PVRChannel& channelinfo);
  bool IsIptvStream(const kodi::addon::PVRChannel& channelinfo) const;
  int GetChannelStreamProgramNumber(const kodi::addon::PVRChannel& channelinfo);

  PVR_ERROR GetStreamReadChunkSize(int& chunksize) override;
  bool IsRealTimeStream() override;
  bool CanPauseStream() override;
  bool CanSeekStream() override;
  int ReadLiveStream(unsigned char* buffer, unsigned int size) override;
  int64_t SeekLiveStream(int64_t position, int whence) override;
  int64_t LengthLiveStream() override;
  PVR_ERROR GetStreamTimes(kodi::addon::PVRStreamTimes& times) override;
  void PauseStream(bool paused) override;

  bool OpenRecordedStream(const kodi::addon::PVRRecording& recinfo) override;
  void CloseRecordedStream() override;
  int ReadRecordedStream(unsigned char* buffer, unsigned int size) override;
  int64_t SeekRecordedStream(int64_t position, int whence) override;
  int64_t LengthRecordedStream() override;

protected:
  void Process();

private:
  static const int INITIAL_EPG_WAIT_SECS = 60;
  static const int INITIAL_EPG_STEP_SECS = 5;
  static const int PROCESS_LOOP_WAIT_SECS = 5;

  // helper functions
  std::string GetStreamURL(const std::string& strM3uURL);
  enigma2::ChannelsChangeState CheckForChannelAndGroupChanges();
  void ReloadChannelsGroupsAndEPG();

  // members
  bool m_isConnected = false;
  int m_currentChannel = -1;
  std::atomic_bool m_dueRecordingUpdate{true};
  time_t m_lastSignalStatusUpdateSeconds;
  bool m_skipInitialEpgLoad;
  int m_epgMaxPastDays;
  int m_epgMaxFutureDays;

  mutable enigma2::Channels m_channels;
  enigma2::ChannelGroups m_channelGroups;
  enigma2::Recordings m_recordings{*this, m_channels, m_entryExtractor};
  std::vector<std::string>& m_locations = m_recordings.GetLocations();
  enigma2::Epg m_epg{*this, m_entryExtractor, m_epgMaxPastDays, m_epgMaxFutureDays};
  enigma2::Timers m_timers{*this, m_channels, m_channelGroups, m_locations, m_epg, m_entryExtractor};
  enigma2::Settings& m_settings = enigma2::Settings::GetInstance();
  enigma2::Admin m_admin;
  enigma2::extract::EpgEntryExtractor m_entryExtractor;
  enigma2::utilities::SignalStatus m_signalStatus;
  enigma2::ConnectionManager* connectionManager;

  enigma2::IStreamReader* m_activeStreamReader = nullptr;
  enigma2::IStreamReader* m_timeshiftInternalStreamReader = nullptr;
  enigma2::RecordingReader* m_recordingReader = nullptr;

  std::atomic<bool> m_running = {false};
  std::thread m_thread;
  mutable std::mutex m_mutex;

  std::atomic<bool> m_paused = {false};
};
