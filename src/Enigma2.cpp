/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Enigma2.h"

#include "enigma2/TimeshiftBuffer.h"
#include "enigma2/utilities/CurlFile.h"
#include "enigma2/utilities/Logger.h"
#include "enigma2/utilities/StreamUtils.h"
#include "enigma2/utilities/WebUtils.h"
#include "enigma2/utilities/XMLUtils.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using namespace kodi::tools;

template<typename T> void SafeDelete(T*& p)
{
  if (p)
  {
    delete p;
    p = nullptr;
  }
}

Enigma2::Enigma2(const kodi::addon::IInstanceInfo& instance)
  : enigma2::IConnectionListener(instance),
    m_epgMaxPastDays(EpgMaxPastDays()),
    m_epgMaxFutureDays(EpgMaxFutureDays())
{
  m_timers.AddTimerChangeWatcher(&m_dueRecordingUpdate);

  connectionManager = new ConnectionManager(*this);
}

Enigma2::~Enigma2()
{
  if (connectionManager)
    connectionManager->Stop();
  delete connectionManager;
}

PVR_ERROR Enigma2::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsEPGEdl(false);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsRadio(true);
  capabilities.SetSupportsRecordings(true);
  capabilities.SetSupportsRecordingsDelete(true);
  capabilities.SetSupportsRecordingsUndelete(true);
  capabilities.SetSupportsTimers(true);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsChannelScan(false);
  capabilities.SetSupportsChannelSettings(false);
  capabilities.SetHandlesInputStream(true);
  capabilities.SetHandlesDemuxing(false);
  capabilities.SetSupportsRecordingPlayCount(m_settings.SupportsEditingRecordings() && m_settings.GetStoreRecordingLastPlayedAndCount());
  capabilities.SetSupportsLastPlayedPosition(m_settings.SupportsEditingRecordings() && m_settings.GetStoreRecordingLastPlayedAndCount());
  capabilities.SetSupportsRecordingEdl(true);
  capabilities.SetSupportsRecordingsRename(m_settings.SupportsEditingRecordings());
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);
  capabilities.SetSupportsAsyncEPGTransfer(false);
  capabilities.SetSupportsRecordingSize(m_settings.SupportsRecordingSizes());
  capabilities.SetSupportsProviders(true);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetBackendName(std::string& name)
{
  name = m_admin.GetServerName();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetBackendVersion(std::string& version)
{
  version = m_admin.GetServerVersion();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetBackendHostname(std::string& hostname)
{
  hostname = m_settings.GetHostname();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetConnectionString(std::string& connection)
{
  connection = StringUtils::Format("%s%s", m_settings.GetHostname().c_str(), IsConnected() ? "" : kodi::addon::GetLocalizedString(30082).c_str()); // (Not connected!)
  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Connection
 * *************************************************************************/

void Enigma2::ConnectionLost()
{
  Logger::Log(LEVEL_INFO, "%s Lost connection with Enigma2 device...", __func__);

  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __func__);
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();

  std::lock_guard<std::mutex> lock(m_mutex);
  m_currentChannel = -1;
  m_isConnected = false;
}

void Enigma2::ConnectionEstablished()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  Logger::Log(LEVEL_DEBUG, "%s Removing internal channels and groups lists...", __func__);
  m_channels.ClearChannels();
  m_channelGroups.ClearChannelGroups();
  m_providers.ClearProviders();

  Logger::Log(LEVEL_INFO, "%s Connection Established with Enigma2 device...", __func__);

  Logger::Log(LEVEL_INFO, "%s - VU+ Addon Configuration options", __func__);
  Logger::Log(LEVEL_INFO, "%s - Hostname: '%s'", __func__, m_settings.GetHostname().c_str());
  Logger::Log(LEVEL_INFO, "%s - WebPort: '%d'", __func__, m_settings.GetWebPortNum());
  Logger::Log(LEVEL_INFO, "%s - StreamPort: '%d'", __func__, m_settings.GetStreamPortNum());
  if (!m_settings.GetUseSecureConnection())
    Logger::Log(LEVEL_INFO, "%s Use HTTPS: 'false'", __func__);
  else
    Logger::Log(LEVEL_INFO, "%s Use HTTPS: 'true'", __func__);

  m_isConnected = m_admin.Initialise();

  if (!m_isConnected)
  {
    Logger::Log(LEVEL_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __func__);
    kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(30515));
    return;
  }

  m_settings.ReadFromAddon();

  m_recordings.ClearLocations();
  m_recordings.LoadLocations();

  if (m_channels.GetNumChannels() == 0)
  {
    // Load the TV channels - close connection if no channels are found
    if (!m_channelGroups.LoadChannelGroups())
    {
      Logger::Log(LEVEL_ERROR, "%s No channel groups (bouquets) found, please check the addon channel settings, exiting", __func__);
      kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(30516));

      return;
    }

    if (!m_channels.LoadChannels(m_channelGroups))
    {
      Logger::Log(LEVEL_ERROR, "%s No channels found, please check the addon channel settings, exiting", __func__);
      kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(30517));

      return;
    }
  }

  m_epg.Initialise(m_channels, m_channelGroups);

  m_timers.TimerUpdates();

  Logger::Log(LEVEL_INFO, "%s Starting separate client update thread...", __func__);
  m_running = true;
  m_thread = std::thread([&] { Process(); });
}

/* **************************************************************************
 * Connection
 * *************************************************************************/

PVR_ERROR Enigma2::OnSystemSleep()
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  connectionManager->OnSleep();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::OnSystemWake()
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  connectionManager->OnWake();
  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Device and helpers
 **************************************************************************/

bool Enigma2::Start()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  connectionManager->Start();

  return true;
}

void Enigma2::Process()
{
  Logger::Log(LEVEL_DEBUG, "%s - starting", __func__);

  // Whether or not initial EPG updates occurred now Trigger "Real" EPG updates
  // This will regard Initial EPG as completed anyway.
  m_epg.TriggerEpgUpdatesForChannels();

  unsigned int updateTimer = 0;
  time_t lastUpdateTimeSeconds = std::time(nullptr);
  int lastUpdateHour = m_settings.GetChannelAndGroupUpdateHour(); //ignore if we start during same hour

  while (m_running && m_isConnected)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_LOOP_WAIT_SECS * 1000));

    time_t currentUpdateTimeSeconds = std::time(nullptr);
    std::tm timeInfo = *std::localtime(&currentUpdateTimeSeconds);
    updateTimer += static_cast<unsigned int>(currentUpdateTimeSeconds - lastUpdateTimeSeconds);
    lastUpdateTimeSeconds = currentUpdateTimeSeconds;

    if (m_dueRecordingUpdate || updateTimer >= (m_settings.GetUpdateIntervalMins() * 60))
    {
      updateTimer = 0;

      // Trigger Timer and Recording updates according to the addon settings
      std::lock_guard<std::mutex> lock(m_mutex);
      // We need to check this again in case the thread is stopped (when destroying Enigma2) during the sleep, otherwise TimerUpdates could be called after the object is released
      if (m_running && m_isConnected)
      {
        Logger::Log(LEVEL_INFO, "%s Perform Updates!", __func__);

        if (m_settings.GetAutomaticTimerListCleanupEnabled())
        {
          m_timers.RunAutoTimerListCleanup();
        }
        m_timers.TimerUpdates();

        if (m_dueRecordingUpdate || m_settings.GetUpdateMode() == UpdateMode::TIMERS_AND_RECORDINGS)
        {
          m_dueRecordingUpdate = false;
          kodi::addon::CInstancePVRClient::TriggerRecordingUpdate();
        }
      }
    }

    if (lastUpdateHour != timeInfo.tm_hour && timeInfo.tm_hour == m_settings.GetChannelAndGroupUpdateHour())
    {
      // Trigger Channel and Group updates according to the addon settings
      std::lock_guard<std::mutex> lock(m_mutex);
      // We need to check this again in case the thread is stopped (when destroying Enigma2) during the sleep, otherwise TimerUpdates could be called after the object is released
      if (m_running && m_isConnected)
      {
        if (CheckForChannelAndGroupChanges() != ChannelsChangeState::NO_CHANGE &&
            m_settings.GetChannelAndGroupUpdateMode() == ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS)
        {
          ReloadChannelsGroupsAndEPG();
        }
      }
    }
    lastUpdateHour = timeInfo.tm_hour;
  }
}

ChannelsChangeState Enigma2::CheckForChannelAndGroupChanges()
{
  ChannelsChangeState changeType = ChannelsChangeState::NO_CHANGE;

  if (m_settings.GetChannelAndGroupUpdateMode() != ChannelAndGroupUpdateMode::DISABLED)
  {
    Logger::Log(LEVEL_INFO, "%s Checking for Channel and Group Changes!", __func__);

    //Now check for any channel or group changes
    Providers latestProviders;
    ChannelGroups latestChannelGroups;
    Channels latestChannels{latestProviders};

    // Load the TV channels - close connection if no channels are found
    if (latestChannelGroups.LoadChannelGroups())
    {
      if (latestChannels.LoadChannels(latestChannelGroups))
      {
        changeType = m_channels.CheckForChannelAndGroupChanges(latestChannelGroups, latestChannels);

        if (m_settings.GetChannelAndGroupUpdateMode() == ChannelAndGroupUpdateMode::NOTIFY_AND_LOG)
        {
          if (changeType == ChannelsChangeState::CHANNEL_GROUPS_CHANGED)
          {
            Logger::Log(LEVEL_INFO, "%s Channel group (bouquet) changes detected, please restart to load changes", __func__);
            kodi::QueueNotification(QUEUE_INFO, "", kodi::addon::GetLocalizedString(30518));
          }
          else if (changeType == ChannelsChangeState::CHANNELS_CHANGED)
          {
            Logger::Log(LEVEL_INFO, "%s Channel changes detected, please restart to load changes", __func__);
            kodi::QueueNotification(QUEUE_INFO, "", kodi::addon::GetLocalizedString(30519));
          }
        }
        else // RELOAD_CHANNELS_AND_GROUPS
        {
          if (changeType == ChannelsChangeState::CHANNEL_GROUPS_CHANGED)
          {
            Logger::Log(LEVEL_INFO, "%s Channel group (bouquet) changes detected, reloading channels, groups and EPG now", __func__);
            kodi::QueueNotification(QUEUE_INFO, "", kodi::addon::GetLocalizedString(30521));
          }
          else if (changeType == ChannelsChangeState::CHANNELS_CHANGED)
          {
            Logger::Log(LEVEL_INFO, "%s Channel changes detected, reloading channels, groups and EPG now", __func__);
            kodi::QueueNotification(QUEUE_INFO, "", kodi::addon::GetLocalizedString(30522));
          }
        }
      }
    }
  }

  return changeType;
}

void Enigma2::ReloadChannelsGroupsAndEPG()
{
  Logger::Log(LEVEL_DEBUG, "%s Removing internal channels list...", __func__);
  m_channels.ClearChannels();
  m_channelGroups.ClearChannelGroups();
  m_providers.ClearProviders();

  m_recordings.ClearLocations();
  m_recordings.LoadLocations();

  m_channelGroups.LoadChannelGroups();
  m_channels.LoadChannels(m_channelGroups);

  kodi::addon::CInstancePVRClient::TriggerProvidersUpdate();
  kodi::addon::CInstancePVRClient::TriggerChannelGroupsUpdate();
  kodi::addon::CInstancePVRClient::TriggerChannelUpdate();

  m_epg.Initialise(m_channels, m_channelGroups);

  m_timers.TimerUpdates();

  for (const auto& myChannel : m_channels.GetChannelsList())
    kodi::addon::CInstancePVRClient::TriggerEpgUpdate(myChannel->GetUniqueId());

  kodi::addon::CInstancePVRClient::TriggerRecordingUpdate();
}

void Enigma2::SendPowerstate()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_admin.SendPowerstate();
}

bool Enigma2::IsConnected() const
{
  return m_isConnected;
}

/***************************************************************************
 * Providers
 **************************************************************************/

PVR_ERROR Enigma2::GetProvidersAmount(int& amount)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  amount = m_providers.GetNumProviders();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetProviders(kodi::addon::PVRProvidersResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::vector<kodi::addon::PVRProvider> providers;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_providers.GetProviders(providers);
  }

  Logger::Log(LEVEL_DEBUG, "%s - providers available '%d'", __func__, providers.size());

  for (const auto& provider : providers)
    results.Add(provider);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Channel Groups
 **************************************************************************/

PVR_ERROR Enigma2::GetChannelGroupsAmount(int& amount)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  amount = m_channelGroups.GetNumChannelGroups();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::vector<kodi::addon::PVRChannelGroup> channelGroups;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channelGroups.GetChannelGroups(channelGroups, radio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channel groups available '%d'", __func__, channelGroups.size());

  for (const auto& channelGroup : channelGroups)
    results.Add(channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::vector<kodi::addon::PVRChannelGroupMember> channelGroupMembers;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channelGroups.GetChannelGroupMembers(channelGroupMembers, group.GetGroupName());
  }

  Logger::Log(LEVEL_DEBUG, "%s - group '%s' members available '%d'", __func__, group.GetGroupName().c_str(), channelGroupMembers.size());

  for (const auto& channelGroupMember : channelGroupMembers)
    results.Add(channelGroupMember);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Channels
 **************************************************************************/

PVR_ERROR Enigma2::GetChannelsAmount(int& amount)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  amount = m_channels.GetNumChannels();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::vector<kodi::addon::PVRChannel> channels;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channels.GetChannels(channels, radio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channels available '%d', radio = %d", __func__, channels.size(), radio);

  for (auto& channel : channels)
    results.Add(channel);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  //
  // For Enimga2 native streams we only set properties that do not change the stream URL as they use
  // their own inputstream within the add-on. First we set the MIME type as it will always be "video/mp2t" and
  // will speed up channel open times. Secondly we set the program number which comes with every Enigma2 channel.
  // For providers that use MPTS it allows the FFMPEG Demux to identify the correct Program/PID. This is controlled by
  // a setting as there is a slight delay introdcued when opening channels and supplying this value.
  //
  // For IPTV streams this function is used to set the stream URL and ffmpegdirect inputstream properties as they
  // do not use the add-ons own inputsream.
  //

  if (!IsIptvStream(channel))
  {
    properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, "video/mp2t");

    if (m_settings.SetStreamProgramID())
    {
      const std::string strStreamProgramNumber = std::to_string(GetChannelStreamProgramNumber(channel));

      Logger::Log(LEVEL_INFO, "%s - for channel: %s, set Stream Program Number to %s - %s",
                      __func__, channel.GetChannelName().c_str(), strStreamProgramNumber.c_str(), GetLiveStreamURL(channel).c_str());

      properties.emplace_back("program", strStreamProgramNumber);
    }
  }
  else
  {
    std::string streamURL = GetLiveStreamURL(channel);

    if (StreamUtils::CheckInputstreamInstalledAndEnabled(INPUTSTREAM_FFMPEGDIRECT) &&
        Settings::GetInstance().IsTimeshiftEnabledIptv())
    {
      StreamType streamType = StreamUtils::GetStreamType(streamURL);
      if (streamType == StreamType::OTHER_TYPE)
        streamType = StreamUtils::InspectStreamType(streamURL);

      properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_FFMPEGDIRECT);
      StreamUtils::SetFFmpegDirectManifestTypeStreamProperty(properties, streamURL, streamType);
      properties.emplace_back("inputstream.ffmpegdirect.stream_mode", "timeshift");
      properties.emplace_back("inputstream.ffmpegdirect.is_realtime_stream", "true");


      streamURL = StreamUtils::GetURLWithFFmpegReconnectOptions(streamURL, streamType);
    }

    properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);
  }

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * EPG
 **************************************************************************/

PVR_ERROR Enigma2::GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results)
{
  if (m_settings.GetEPGDelayPerChannelDelay() != 0)
    std::this_thread::sleep_for(std::chrono::seconds(m_settings.GetEPGDelayPerChannelDelay()));

  //Have a lock while getting the channel. Then we don't have to worry about a disconnection while retrieving the EPG data.
  std::shared_ptr<Channel> myChannel;
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_channels.IsValid(channelUid))
    {
      Logger::Log(LEVEL_ERROR, "%s Could not fetch channel object - not fetching EPG for channel with UniqueID '%d'", __func__, channelUid);
      return PVR_ERROR_SERVER_ERROR;
    }

    myChannel = m_channels.GetChannel(channelUid);
  }

  return m_epg.GetEPGForChannel(myChannel->GetServiceReference(), start, end, results);
}

PVR_ERROR Enigma2::SetEPGMaxPastDays(int epgMaxPastDays)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  m_epg.SetEPGMaxPastDays(epgMaxPastDays);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::SetEPGMaxFutureDays(int epgMaxFutureDays)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  m_epg.SetEPGMaxFutureDays(epgMaxFutureDays);
  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Livestream
 **************************************************************************/

bool Enigma2::OpenLiveStream(const kodi::addon::PVRChannel& channelinfo)
{
  if (!IsConnected())
    return false;

  Logger::Log(LEVEL_DEBUG, "%s: channel=%u", __func__, channelinfo.GetUniqueId());
  std::lock_guard<std::mutex> lock(m_mutex);

  if (channelinfo.GetUniqueId() != m_currentChannel)
  {
    m_currentChannel = channelinfo.GetUniqueId();
    m_lastSignalStatusUpdateSeconds = 0;

    if (m_settings.GetZapBeforeChannelSwitch())
    {
      // Zapping is set to true, so send the zapping command to the PVR box
      const std::string strServiceReference = m_channels.GetChannel(channelinfo.GetUniqueId())->GetServiceReference().c_str();

      const std::string strCmd = StringUtils::Format("web/zap?sRef=%s", WebUtils::URLEncodeInline(strServiceReference).c_str());

      std::string strResult;
      if (!WebUtils::SendSimpleCommand(strCmd, strResult, true))
        return false;
    }
  }

  /* queue a warning if the timeshift buffer path does not exist */
  if (m_settings.GetTimeshift() != Timeshift::OFF && !m_settings.IsTimeshiftBufferPathValid())
    kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(30514));

  const std::string streamURL = GetLiveStreamURL(channelinfo);
  m_activeStreamReader = new StreamReader(streamURL, m_settings.GetReadTimeoutSecs());
  if (m_settings.GetTimeshift() == Timeshift::ON_PLAYBACK && m_settings.IsTimeshiftBufferPathValid())
  {
    m_timeshiftInternalStreamReader = m_activeStreamReader;
    m_activeStreamReader = new TimeshiftBuffer(m_activeStreamReader);
  }

  return m_activeStreamReader->Start();
}

void Enigma2::CloseLiveStream()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_currentChannel = -1;
  SafeDelete(m_activeStreamReader);
  if (m_timeshiftInternalStreamReader)
    SafeDelete(m_timeshiftInternalStreamReader);
}

const std::string Enigma2::GetLiveStreamURL(const kodi::addon::PVRChannel& channelinfo)
{
  if (m_settings.GetAutoConfigLiveStreamsEnabled())
  {
    // we need to download the M3U file that contains the URL for the stream...
    // we do it here for 2 reasons:
    //  1. This is faster than doing it during initialization
    //  2. The URL can change, so this is more up-to-date.
    return GetStreamURL(m_channels.GetChannel(channelinfo.GetUniqueId())->GetM3uURL());
  }

  return m_channels.GetChannel(channelinfo.GetUniqueId())->GetStreamURL();
}

bool Enigma2::IsIptvStream(const kodi::addon::PVRChannel& channelinfo) const
{
  return m_channels.GetChannel(channelinfo.GetUniqueId())->IsIptvStream();
}

int Enigma2::GetChannelStreamProgramNumber(const kodi::addon::PVRChannel& channelinfo)
{
  return m_channels.GetChannel(channelinfo.GetUniqueId())->GetStreamProgramNumber();
}

/**
  * GetStreamURL() reads out a stream-URL from a M3U-file.
  *
  * This method downloads a M3U-file from the address that is given by strM3uURL.
  * It returns the first line that starts with "http".
  * If no line starts with "http" the last line is returned.
  */
std::string Enigma2::GetStreamURL(const std::string& strM3uURL)
{
  const std::string strM3U = WebUtils::GetHttpXML(strM3uURL);
  std::istringstream streamM3U(strM3U);
  std::string strURL = "";
  while (std::getline(streamM3U, strURL))
  {
    if (strURL.compare(0, 4, "http", 4) == 0)
      break;
  };
  return strURL;
}

/***************************************************************************
 * Recordings
 **************************************************************************/

PVR_ERROR Enigma2::GetRecordingsAmount(bool deleted, int& amount)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  amount = m_recordings.GetNumRecordings(deleted);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  m_recordings.LoadRecordings(deleted);

  std::vector<kodi::addon::PVRRecording> recordings;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recordings.GetRecordings(recordings, deleted);
  }

  Logger::Log(LEVEL_DEBUG, "%s - recordings available '%d'", __func__, recordings.size());

  for (const auto& recording : recordings)
    results.Add(recording);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::DeleteRecording(const kodi::addon::PVRRecording& recinfo)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_recordings.DeleteRecording(recinfo);
}

PVR_ERROR Enigma2::UndeleteRecording(const kodi::addon::PVRRecording& recording)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_recordings.UndeleteRecording(recording);
}

PVR_ERROR Enigma2::DeleteAllRecordingsFromTrash()
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_recordings.DeleteAllRecordingsFromTrash();
}

PVR_ERROR Enigma2::GetRecordingEdl(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVREDLEntry>& edl)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (!m_settings.GetRecordingEDLsEnabled())
    return PVR_ERROR_NO_ERROR;


  std::lock_guard<std::mutex> lock(m_mutex);

  m_recordings.GetRecordingEdl(recording.GetRecordingId(), edl);

  Logger::Log(LEVEL_DEBUG, "%s - recording '%s' has '%d' EDL entries available", __func__, recording.GetTitle().c_str(), edl.size());

  return PVR_ERROR_NO_ERROR;
}

bool Enigma2::OpenRecordedStream(const kodi::addon::PVRRecording& recinfo)
{
  if (m_recordingReader)
    SafeDelete(m_recordingReader);

  if (!IsConnected())
    return false;

  std::lock_guard<std::mutex> lock(m_mutex);
  std::time_t now = std::time(nullptr), start = 0, end = 0;
  std::string channelName = recinfo.GetChannelName();
  auto timer = m_timers.GetTimer([&](const Timer &timer)
      {
        return timer.IsRunning(&now, &channelName, recinfo.GetRecordingTime());
      });
  if (timer)
  {
    start = timer->GetRealStartTime();
    end = timer->GetRealEndTime();
  }

  m_recordingReader = new RecordingReader(m_recordings.GetRecordingURL(recinfo), start, end, recinfo.GetDuration());
  return m_recordingReader->Start();
}

bool Enigma2::HasRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording)
{
  return m_recordings.HasRecordingStreamProgramNumber(recording);
}

int Enigma2::GetRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording)
{
  return m_recordings.GetRecordingStreamProgramNumber(recording);
}

PVR_ERROR Enigma2::RenameRecording(const kodi::addon::PVRRecording& recording)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  return m_recordings.RenameRecording(recording);
}

PVR_ERROR Enigma2::SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  return m_recordings.SetRecordingPlayCount(recording, count);
}

PVR_ERROR Enigma2::SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int lastPlayedPosition)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  return m_recordings.SetRecordingLastPlayedPosition(recording, lastPlayedPosition);
}

PVR_ERROR Enigma2::GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int& position)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  position = m_recordings.GetRecordingLastPlayedPosition(recording);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& sizeInBytes)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  return m_recordings.GetRecordingSize(recording, sizeInBytes);
}

PVR_ERROR Enigma2::GetRecordingStreamProperties(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  if (!m_settings.SetStreamProgramID())
    return PVR_ERROR_NOT_IMPLEMENTED;

  //
  // We only use this function to set the program number which may comes with every Enigma2 recording. For providers that
  // use MPTS it allows the FFMPEG Demux to identify the correct Program/PID.
  //

  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (HasRecordingStreamProgramNumber(recording))
  {
    const std::string strStreamProgramNumber = std::to_string(GetRecordingStreamProgramNumber(recording));

    Logger::Log(LEVEL_INFO, "%s - for recording for channel: %s, set Stream Program Number to %s - %s",
                    __func__, recording.GetChannelName().c_str(), strStreamProgramNumber.c_str(), recording.GetRecordingId().c_str());

    properties.emplace_back("program", strStreamProgramNumber);
  }

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Timers
 **************************************************************************/

PVR_ERROR Enigma2::GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (IsConnected())
  {
    m_timers.GetTimerTypes(types);
    Logger::Log(LEVEL_INFO, "%s Transferred %u timer types", __func__, types.size());
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetTimersAmount(int& amount)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::lock_guard<std::mutex> lock(m_mutex);
  amount = m_timers.GetTimerCount();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetTimers(kodi::addon::PVRTimersResultSet& results)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  std::vector<kodi::addon::PVRTimer> timers;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timers.GetTimers(timers);
    m_timers.GetAutoTimers(timers);
  }

  Logger::Log(LEVEL_DEBUG, "%s - timers available '%d'", __func__, timers.size());

  for (auto& timer : timers)
    results.Add(timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::AddTimer(const kodi::addon::PVRTimer& timer)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_timers.AddTimer(timer);
}

PVR_ERROR Enigma2::UpdateTimer(const kodi::addon::PVRTimer& timer)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_timers.UpdateTimer(timer);
}

PVR_ERROR Enigma2::DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return m_timers.DeleteTimer(timer);
}

/***************************************************************************
 * Misc
 **************************************************************************/

PVR_ERROR Enigma2::GetDriveSpace(uint64_t& total, uint64_t& used)
{
  if (!IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (m_admin.GetDeviceHasHDD())
    return m_admin.GetDriveSpace(total, used, m_locations);

  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR Enigma2::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
{
  // SNR = Signal to Noise Ratio - which means signal quality
  // AGC = Automatic Gain Control - which means signal strength
  // BER = Bit Error Rate - which shows the error rate of the signal.
  // UNC = There is not notion of UNC on enigma devices

  // So, SNR and AGC should be as high as possible.
  // BER should be as low as possible, like 0. It can be higher, if your other values are higher.

  if (channelUid >= 0)
  {
    const std::shared_ptr<Channel> channel = m_channels.GetChannel(channelUid);

    signalStatus.SetServiceName(channel->GetChannelName());
    signalStatus.SetProviderName(channel->GetProviderName());

    time_t now = std::time(nullptr);
    if ((now - m_lastSignalStatusUpdateSeconds) >= POLL_INTERVAL_SECONDS)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Calling backend for Signal Status after interval of %d seconds", __func__, POLL_INTERVAL_SECONDS);

      if (!m_admin.GetTunerSignal(m_signalStatus, channel))
      {
        return PVR_ERROR_SERVER_ERROR;
      }
      m_lastSignalStatusUpdateSeconds = now;
    }
  }

  signalStatus.SetSNR(m_signalStatus.m_snrPercentage);
  signalStatus.SetBER(m_signalStatus.m_ber);
  signalStatus.SetSignal(m_signalStatus.m_signalStrength);
  signalStatus.SetAdapterName(m_signalStatus.m_adapterName);
  signalStatus.SetAdapterStatus(m_signalStatus.m_adapterStatus);

  Logger::Log(LEVEL_DEBUG, "%s Tuner Details - name: %s, status: %s",
                  __func__, signalStatus.GetAdapterName().c_str(), signalStatus.GetAdapterStatus().c_str());
  Logger::Log(LEVEL_DEBUG, "%s Service Details - service: %s, provider: %s",
                  __func__, signalStatus.GetServiceName().c_str(), signalStatus.GetProviderName().c_str());
  // For some reason the iSNR and iSignal values need to multiplied by 655!
  Logger::Log(LEVEL_DEBUG, "%s Signal - snrPercent: %d, ber: %u, signal strength: %d",
                  __func__, signalStatus.GetSNR() / 655, signalStatus.GetBER(), signalStatus.GetSignal() / 655);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetStreamReadChunkSize(int& chunksize)
{
  if (!chunksize)
    return PVR_ERROR_INVALID_PARAMETERS;
  int size = m_settings.GetStreamReadChunkSizeKb();
  if (!size)
    return PVR_ERROR_NOT_IMPLEMENTED;
  chunksize = m_settings.GetStreamReadChunkSizeKb() * 1024;
  return PVR_ERROR_NO_ERROR;
}

/* live stream functions */

bool Enigma2::IsRealTimeStream()
{
  return (m_activeStreamReader) ? m_activeStreamReader->IsRealTime() : false;
}

bool Enigma2::CanPauseStream()
{
  if (!IsConnected())
    return false;

  if (m_settings.GetTimeshift() != Timeshift::OFF && m_activeStreamReader && m_settings.IsTimeshiftBufferPathValid())
    return (m_settings.GetTimeshift() == Timeshift::ON_PAUSE || m_paused || m_activeStreamReader->HasTimeshiftCapacity());

  return false;
}

bool Enigma2::CanSeekStream()
{
  if (!IsConnected())
    return false;

  return (m_settings.GetTimeshift() != Timeshift::OFF);
}

int Enigma2::ReadLiveStream(unsigned char* buffer, unsigned int size)
{
  return (m_activeStreamReader) ? m_activeStreamReader->ReadData(buffer, size) : 0;
}

int64_t Enigma2::SeekLiveStream(int64_t position, int whence)
{
  return (m_activeStreamReader) ? m_activeStreamReader->Seek(position, whence) : -1;
}

int64_t Enigma2::LengthLiveStream()
{
  return (m_activeStreamReader) ? m_activeStreamReader->Length() : -1;
}

PVR_ERROR Enigma2::GetStreamTimes(kodi::addon::PVRStreamTimes& times)
{
  if (m_activeStreamReader)
  {
    times.SetStartTime(m_activeStreamReader->TimeStart());
    times.SetPTSStart(0);
    times.SetPTSBegin(0);
    times.SetPTSEnd((!m_activeStreamReader->IsTimeshifting()) ? 0
      : (m_activeStreamReader->TimeEnd() - m_activeStreamReader->TimeStart()) * STREAM_TIME_BASE);

    if (m_activeStreamReader->IsTimeshifting())
    {
      if (!m_activeStreamReader->HasTimeshiftCapacity())
      {
        Logger::Log(LEVEL_INFO, "%s Timeshift disk limit of %.1f GiB exceeded, switching to live stream without timehift", __func__, m_settings.GetTimeshiftDiskLimitGB());
        IStreamReader* timeshiftedReader = m_activeStreamReader;
        m_activeStreamReader = m_timeshiftInternalStreamReader;
        m_timeshiftInternalStreamReader = nullptr;
        SafeDelete(timeshiftedReader);
      }
    }

    return PVR_ERROR_NO_ERROR;
  }
  else if (m_recordingReader)
  {
    times.SetStartTime(0);
    times.SetPTSStart(0);
    times.SetPTSBegin(0);
    times.SetPTSEnd(static_cast<int64_t>(m_recordingReader->CurrentDuration()) * STREAM_TIME_BASE);

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

void Enigma2::PauseStream(bool paused)
{
  if (!IsConnected())
    return;

  /* start timeshift on pause */
  if (paused && m_settings.GetTimeshift() == Timeshift::ON_PAUSE &&
      m_activeStreamReader && !m_activeStreamReader->IsTimeshifting() &&
      m_settings.IsTimeshiftBufferPathValid())
  {
    m_timeshiftInternalStreamReader = m_activeStreamReader;
    m_activeStreamReader = new TimeshiftBuffer(m_activeStreamReader);
    m_activeStreamReader->Start();
  }

  m_paused = paused;
}

void Enigma2::CloseRecordedStream()
{
  if (m_recordingReader)
    SafeDelete(m_recordingReader);
}

int Enigma2::ReadRecordedStream(unsigned char* buffer, unsigned int size)
{
  if (!m_recordingReader)
    return 0;

  return m_recordingReader->ReadData(buffer, size);
}

int64_t Enigma2::SeekRecordedStream(int64_t position, int whence)
{
  if (!m_recordingReader)
    return 0;

  return m_recordingReader->Seek(position, whence);
}

int64_t Enigma2::LengthRecordedStream()
{
  if (!m_recordingReader)
    return -1;

  return m_recordingReader->Length();
}
