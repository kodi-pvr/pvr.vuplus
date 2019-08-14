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

#include "Enigma2.h"

#include "client.h"
#include "enigma2/utilities/CurlFile.h"
#include "enigma2/utilities/LocalizedString.h"
#include "enigma2/utilities/Logger.h"
#include "enigma2/utilities/WebUtils.h"
#include "util/XMLUtils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdlib.h>
#include <string>

#include <p8-platform/util/StringUtils.h>

using namespace ADDON;
using namespace P8PLATFORM;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

Enigma2::Enigma2(PVR_PROPERTIES* pvrProps) : m_epgMaxDays(pvrProps->iEpgMaxDays)
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

/* **************************************************************************
 * Connection
 * *************************************************************************/

void Enigma2::ConnectionLost()
{
  CLockObject lock(m_mutex);

  Logger::Log(LEVEL_NOTICE, "%s Lost connection with Enigma2 device...", __FUNCTION__);

  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();

  m_currentChannel = -1;
  m_isConnected = false;
}

void Enigma2::Reset()
{
  Logger::Log(LEVEL_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.ClearChannels();

  Logger::Log(LEVEL_DEBUG, "%s Removing internal timers list...", __FUNCTION__);
  m_timers.ClearTimers();

  Logger::Log(LEVEL_DEBUG, "%s Removing internal recordings list...", __FUNCTION__);
  m_recordings.ClearRecordings(false);
  m_recordings.ClearRecordings(true);

  Logger::Log(LEVEL_DEBUG, "%s Removing internal group list...", __FUNCTION__);
  m_channelGroups.ClearChannelGroups();
}

void Enigma2::ConnectionEstablished()
{
  CLockObject lock(m_mutex);

  Reset();

  Logger::Log(LEVEL_NOTICE, "%s Connection Established with Enigma2 device...", __FUNCTION__);

  Logger::Log(LEVEL_NOTICE, "%s - VU+ Addon Configuration options", __FUNCTION__);
  Logger::Log(LEVEL_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, m_settings.GetHostname().c_str());
  Logger::Log(LEVEL_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, m_settings.GetWebPortNum());
  Logger::Log(LEVEL_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, m_settings.GetStreamPortNum());
  if (!m_settings.GetUseSecureConnection())
    Logger::Log(LEVEL_NOTICE, "%s Use HTTPS: 'false'", __FUNCTION__);
  else
    Logger::Log(LEVEL_NOTICE, "%s Use HTTPS: 'true'", __FUNCTION__);

  if ((m_settings.GetUsername().length() > 0) && (m_settings.GetPassword().length() > 0))
  {
    if ((m_settings.GetUsername().find("@") != std::string::npos) || (m_settings.GetPassword().find("@") != std::string::npos))
    {
      Logger::Log(LEVEL_ERROR, "%s - You cannot use the '@' character in either the username or the password with this addon. Please change your configuraton!", __FUNCTION__);
      return;
    }
  }
  m_isConnected = m_admin.Initialise();

  if (!m_isConnected)
  {
    Logger::Log(LEVEL_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30515).c_str());
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
      Logger::Log(LEVEL_ERROR, "%s No channel groups (bouquets) found, please check the addon channel settings, exiting", __FUNCTION__);
      XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30516).c_str());

      return;
    }

    if (!m_channels.LoadChannels(m_channelGroups))
    {
      Logger::Log(LEVEL_ERROR, "%s No channels found, please check the addon channel settings, exiting", __FUNCTION__);
      XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30517).c_str());

      return;
    }
  }

  m_skipInitialEpgLoad = m_settings.SkipInitialEpgLoad();

  m_epg.Initialise(m_channels, m_channelGroups);

  m_timers.TimerUpdates();

  Logger::Log(LEVEL_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread();
}

/* **************************************************************************
 * Connection
 * *************************************************************************/

void Enigma2::OnSleep()
{
  connectionManager->OnSleep();
}

void Enigma2::OnWake()
{
  connectionManager->OnWake();
}

/***************************************************************************
 * Device and helpers
 **************************************************************************/

bool Enigma2::Start()
{
  CLockObject lock(m_mutex);

  connectionManager->Start();

  return true;
}

void* Enigma2::Process()
{
  Logger::Log(LEVEL_DEBUG, "%s - starting", __FUNCTION__);

  // Wait for the initial EPG update to complete
  int totalWaitSecs = 0;
  while (totalWaitSecs < INITIAL_EPG_WAIT_SECS)
  {
    totalWaitSecs += INITIAL_EPG_STEP_SECS;

    if (!m_epg.IsInitialEpgCompleted())
      Sleep(INITIAL_EPG_STEP_SECS * 1000);
  }

  m_skipInitialEpgLoad = false;

  // Whether or not initial EPG updates occurred now Trigger "Real" EPG updates
  // This will regard Initial EPG as completed anyway.
  m_epg.TriggerEpgUpdatesForChannels();

  unsigned int updateTimer = 0;
  time_t lastUpdateTimeSeconds = time(nullptr);
  int lastUpdateHour = m_settings.GetChannelAndGroupUpdateHour(); //ignore if we start during same hour

  while (!IsStopped() && m_isConnected)
  {
    Sleep(PROCESS_LOOP_WAIT_SECS * 1000);

    time_t currentUpdateTimeSeconds = time(nullptr);
    std::tm timeInfo = *std::localtime(&currentUpdateTimeSeconds);
    updateTimer += static_cast<unsigned int>(currentUpdateTimeSeconds - lastUpdateTimeSeconds);
    lastUpdateTimeSeconds = currentUpdateTimeSeconds;

    if (m_dueRecordingUpdate || updateTimer >= (m_settings.GetUpdateIntervalMins() * 60))
    {
      updateTimer = 0;

      // Trigger Timer and Recording updates according to the addon settings
      CLockObject lock(m_mutex);
      // We need to check this again in case StopThread is called (in destroying Enigma2) during the sleep, otherwise TimerUpdates could be called after the object is released
      if (!IsStopped() && m_isConnected)
      {
        Logger::Log(LEVEL_INFO, "%s Perform Updates!", __FUNCTION__);

        if (m_settings.GetAutoTimerListCleanupEnabled())
        {
          m_timers.RunAutoTimerListCleanup();
        }
        m_timers.TimerUpdates();

        if (m_dueRecordingUpdate || m_settings.GetUpdateMode() == UpdateMode::TIMERS_AND_RECORDINGS)
        {
          m_dueRecordingUpdate = false;
          PVR->TriggerRecordingUpdate();
        }
      }
    }

    if (lastUpdateHour != timeInfo.tm_hour && timeInfo.tm_hour == m_settings.GetChannelAndGroupUpdateHour())
    {
      // Trigger Channel and Group updates according to the addon settings
      CLockObject lock(m_mutex);
      // We need to check this again in case StopThread is called (in destroying Enigma2) during the sleep, otherwise TimerUpdates could be called after the object is released
      if (!IsStopped() && m_isConnected)
      {
        if (CheckForChannelAndGroupChanges() != ChannelsChangeState::NO_CHANGE &&
            m_settings.GetChannelAndGroupUpdateMode() != ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS)
        {
          connectionManager->Reconnect();
        }
      }
    }
    lastUpdateHour = timeInfo.tm_hour;
  }

  //CLockObject lock(m_mutex);
  m_started.Broadcast();

  return nullptr;
}

ChannelsChangeState Enigma2::CheckForChannelAndGroupChanges()
{
  ChannelsChangeState changeType = ChannelsChangeState::NO_CHANGE;

  if (m_settings.GetChannelAndGroupUpdateMode() != ChannelAndGroupUpdateMode::DISABLED)
  {
    Logger::Log(LEVEL_INFO, "%s Checking for Channel and Group Changes!", __FUNCTION__);

    //Now check for any channel or group changes
    ChannelGroups latestChannelGroups;
    Channels latestChannels;

    // Load the TV channels - close connection if no channels are found
    if (latestChannelGroups.LoadChannelGroups())
    {
      if (latestChannels.LoadChannels(latestChannelGroups))
      {
        changeType = m_channels.CheckForChannelAndGroupChanges(latestChannelGroups, latestChannels);

        if (m_settings.GetChannelAndGroupUpdateMode() != ChannelAndGroupUpdateMode::NOTIFY_AND_LOG)
        {
          if (changeType == ChannelsChangeState::CHANNEL_GROUPS_CHANGED)
          {
            Logger::Log(LEVEL_NOTICE, "%s Channel group (bouquet) changes detected, please restart to load changes", __FUNCTION__);
            XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30518).c_str());
          }
          else if (changeType == ChannelsChangeState::CHANNELS_CHANGED)
          {
            Logger::Log(LEVEL_NOTICE, "%s Channel changes detected, please restart to load changes", __FUNCTION__);
            XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30519).c_str());
          }
        }
        else // RELOAD_CHANNELS_AND_GROUPS
        {
          if (changeType == ChannelsChangeState::CHANNEL_GROUPS_CHANGED)
            Logger::Log(LEVEL_NOTICE, "%s Channel group (bouquet) changes detected, auto reconnecting to load changes", __FUNCTION__);
          else if (changeType == ChannelsChangeState::CHANNELS_CHANGED)
            Logger::Log(LEVEL_NOTICE, "%s Channel changes detected, , auto reconnecting to load changes", __FUNCTION__);
        }
      }
    }
  }

  return changeType;
}

void Enigma2::SendPowerstate()
{
  CLockObject lock(m_mutex);

  m_admin.SendPowerstate();
}

const char* Enigma2::GetServerName() const
{
  return m_admin.GetServerName();
}

const char* Enigma2::GetServerVersion() const
{
  return m_admin.GetServerVersion();
}

bool Enigma2::IsConnected() const
{
  return m_isConnected;
}

/***************************************************************************
 * Channel Groups
 **************************************************************************/

unsigned int Enigma2::GetNumChannelGroups() const
{
  return m_channelGroups.GetNumChannelGroups();
}

PVR_ERROR Enigma2::GetChannelGroups(ADDON_HANDLE handle, bool radio)
{
  std::vector<PVR_CHANNEL_GROUP> channelGroups;
  {
    CLockObject lock(m_mutex);
    m_channelGroups.GetChannelGroups(channelGroups, radio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channel groups available '%d'", __FUNCTION__, channelGroups.size());

  for (const auto& channelGroup : channelGroups)
    PVR->TransferChannelGroup(handle, &channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  std::vector<PVR_CHANNEL_GROUP_MEMBER> channelGroupMembers;
  {
    CLockObject lock(m_mutex);
    m_channelGroups.GetChannelGroupMembers(channelGroupMembers, group.strGroupName);
  }

  Logger::Log(LEVEL_DEBUG, "%s - group '%s' members available '%d'", __FUNCTION__, group.strGroupName, channelGroupMembers.size());

  for (const auto& channelGroupMember : channelGroupMembers)
      PVR->TransferChannelGroupMember(handle, &channelGroupMember);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Channels
 **************************************************************************/

int Enigma2::GetChannelsAmount() const
{
  return m_channels.GetNumChannels();
}

PVR_ERROR Enigma2::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  std::vector<PVR_CHANNEL> channels;
  {
    CLockObject lock(m_mutex);
    m_channels.GetChannels(channels, bRadio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channels available '%d', radio = %d", __FUNCTION__, channels.size(), bRadio);

  for (auto& channel : channels)
    PVR->TransferChannelEntry(handle, &channel);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * EPG
 **************************************************************************/

PVR_ERROR Enigma2::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  if (m_epg.IsInitialEpgCompleted() && m_settings.GetEPGDelayPerChannelDelay() != 0)
    Sleep(m_settings.GetEPGDelayPerChannelDelay());

  //Have a lock while getting the channel. Then we don't have to worry about a disconnection while retrieving the EPG data.
  std::shared_ptr<Channel> myChannel;
  {
    CLockObject lock(m_mutex);

    if (!m_channels.IsValid(channel.iUniqueId))
    {
      Logger::Log(LEVEL_ERROR, "%s Could not fetch channel object - not fetching EPG for channel with UniqueID '%d'", __FUNCTION__, channel.iUniqueId);
      return PVR_ERROR_SERVER_ERROR;
    }

    myChannel = m_channels.GetChannel(channel.iUniqueId);
  }

  if (m_skipInitialEpgLoad)
  {
    Logger::Log(LEVEL_DEBUG, "%s Skipping Initial EPG for channel: %s", __FUNCTION__, myChannel->GetChannelName().c_str());
    m_epg.MarkChannelAsInitialEpgRead(myChannel->GetServiceReference());
    return PVR_ERROR_NO_ERROR;
  }

  return m_epg.GetEPGForChannel(handle, myChannel->GetServiceReference(), iStart, iEnd);
}

/***************************************************************************
 * Livestream
 **************************************************************************/
bool Enigma2::OpenLiveStream(const PVR_CHANNEL& channelinfo)
{
  Logger::Log(LEVEL_DEBUG, "%s: channel=%u", __FUNCTION__, channelinfo.iUniqueId);
  CLockObject lock(m_mutex);

  if (channelinfo.iUniqueId != m_currentChannel)
  {
    m_currentChannel = channelinfo.iUniqueId;
    m_lastSignalStatusUpdateSeconds = 0;

    if (m_settings.GetZapBeforeChannelSwitch())
    {
      // Zapping is set to true, so send the zapping command to the PVR box
      const std::string strServiceReference = m_channels.GetChannel(channelinfo.iUniqueId)->GetServiceReference().c_str();

      const std::string strCmd = StringUtils::Format("web/zap?sRef=%s", WebUtils::URLEncodeInline(strServiceReference).c_str());

      std::string strResult;
      if (!WebUtils::SendSimpleCommand(strCmd, strResult, true))
        return false;
    }
  }
  return true;
}

void Enigma2::CloseLiveStream(void)
{
  CLockObject lock(m_mutex);
  m_currentChannel = -1;
}

const std::string Enigma2::GetLiveStreamURL(const PVR_CHANNEL& channelinfo)
{
  if (m_settings.GetAutoConfigLiveStreamsEnabled())
  {
    // we need to download the M3U file that contains the URL for the stream...
    // we do it here for 2 reasons:
    //  1. This is faster than doing it during initialization
    //  2. The URL can change, so this is more up-to-date.
    return GetStreamURL(m_channels.GetChannel(channelinfo.iUniqueId)->GetM3uURL());
  }

  return m_channels.GetChannel(channelinfo.iUniqueId)->GetStreamURL();
}

bool Enigma2::IsIptvStream(const PVR_CHANNEL& channelinfo) const
{
  return m_channels.GetChannel(channelinfo.iUniqueId)->IsIptvStream();
}

int Enigma2::GetChannelStreamProgramNumber(const PVR_CHANNEL& channelinfo)
{
  return m_channels.GetChannel(channelinfo.iUniqueId)->GetStreamProgramNumber();
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

unsigned int Enigma2::GetRecordingsAmount(bool deleted)
{
  return m_recordings.GetNumRecordings(deleted);
}

PVR_ERROR Enigma2::GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  m_recordings.LoadRecordings(deleted);

  std::vector<PVR_RECORDING> recordings;
  {
    CLockObject lock(m_mutex);
    m_recordings.GetRecordings(recordings, deleted);
  }

  Logger::Log(LEVEL_DEBUG, "%s - recordings available '%d'", __FUNCTION__, recordings.size());

  for (const auto& recording : recordings)
    PVR->TransferRecordingEntry(handle, &recording);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::DeleteRecording(const PVR_RECORDING& recinfo)
{
  return m_recordings.DeleteRecording(recinfo);
}

PVR_ERROR Enigma2::UndeleteRecording(const PVR_RECORDING& recording)
{
  return m_recordings.UndeleteRecording(recording);
}

PVR_ERROR Enigma2::DeleteAllRecordingsFromTrash()
{
  return m_recordings.DeleteAllRecordingsFromTrash();
}

PVR_ERROR Enigma2::GetRecordingEdl(const PVR_RECORDING& recinfo, PVR_EDL_ENTRY edl[], int* size)
{
  std::vector<PVR_EDL_ENTRY> edlEntries;
  {
    CLockObject lock(m_mutex);
    m_recordings.GetRecordingEdl(recinfo.strRecordingId, edlEntries);
  }

  Logger::Log(LEVEL_DEBUG, "%s - recording '%s' has '%d' EDL entries available", __FUNCTION__, recinfo.strTitle, edlEntries.size());

  int index = 0;
  int maxSize = *size;
  for (auto& edlEntry : edlEntries)
  {
    if (index >= maxSize)
      break;

    edl[index].start = edlEntry.start;
    edl[index].end = edlEntry.end;
    edl[index].type = edlEntry.type;

    index++;
  }
  *size = edlEntries.size();

  return PVR_ERROR_NO_ERROR;
}

RecordingReader* Enigma2::OpenRecordedStream(const PVR_RECORDING& recinfo)
{
  CLockObject lock(m_mutex);
  std::time_t now = std::time(nullptr), start = 0, end = 0;
  std::string channelName = recinfo.strChannelName;
  auto timer = m_timers.GetTimer([&](const Timer &timer)
      {
        return timer.IsRunning(&now, &channelName, recinfo.recordingTime);
      });
  if (timer)
  {
    start = timer->GetRealStartTime();
    end = timer->GetRealEndTime();
  }

  return new RecordingReader(m_recordings.GetRecordingURL(recinfo).c_str(), start, end, recinfo.iDuration);
}

bool Enigma2::HasRecordingStreamProgramNumber(const PVR_RECORDING& recording)
{
  return m_recordings.HasRecordingStreamProgramNumber(recording);
}

int Enigma2::GetRecordingStreamProgramNumber(const PVR_RECORDING& recording)
{
  return m_recordings.GetRecordingStreamProgramNumber(recording);
}

PVR_ERROR Enigma2::RenameRecording(const PVR_RECORDING& recording)
{
  CLockObject lock(m_mutex);
  return m_recordings.RenameRecording(recording);
}

PVR_ERROR Enigma2::SetRecordingPlayCount(const PVR_RECORDING& recording, int count)
{
  CLockObject lock(m_mutex);
  return m_recordings.SetRecordingPlayCount(recording, count);
}

PVR_ERROR Enigma2::SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastPlayedPosition)
{
  CLockObject lock(m_mutex);
  return m_recordings.SetRecordingLastPlayedPosition(recording, lastPlayedPosition);
}

int Enigma2::GetRecordingLastPlayedPosition(const PVR_RECORDING& recording)
{
  CLockObject lock(m_mutex);
  return m_recordings.GetRecordingLastPlayedPosition(recording);
}

/***************************************************************************
 * Timers
 **************************************************************************/

void Enigma2::GetTimerTypes(PVR_TIMER_TYPE types[], int* size)
{
  std::vector<PVR_TIMER_TYPE> timerTypes;
  {
    CLockObject lock(m_mutex);
    m_timers.GetTimerTypes(timerTypes);
  }

  int i = 0;
  for (auto& timerType : timerTypes)
    types[i++] = timerType;
  *size = timerTypes.size();
  Logger::Log(LEVEL_NOTICE, "%s Transfered %u timer types", __FUNCTION__, *size);
}

int Enigma2::GetTimersAmount()
{
  CLockObject lock(m_mutex);
  return m_timers.GetTimerCount();
}

PVR_ERROR Enigma2::GetTimers(ADDON_HANDLE handle)
{
  std::vector<PVR_TIMER> timers;
  {
    CLockObject lock(m_mutex);
    m_timers.GetTimers(timers);
    m_timers.GetAutoTimers(timers);
  }

  Logger::Log(LEVEL_DEBUG, "%s - timers available '%d'", __FUNCTION__, timers.size());

  for (auto& timer : timers)
    PVR->TransferTimerEntry(handle, &timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::AddTimer(const PVR_TIMER& timer)
{
  return m_timers.AddTimer(timer);
}

PVR_ERROR Enigma2::UpdateTimer(const PVR_TIMER& timer)
{
  return m_timers.UpdateTimer(timer);
}

PVR_ERROR Enigma2::DeleteTimer(const PVR_TIMER& timer)
{
  return m_timers.DeleteTimer(timer);
}

/***************************************************************************
 * Misc
 **************************************************************************/

PVR_ERROR Enigma2::GetDriveSpace(long long* iTotal, long long* iUsed)
{
  if (m_admin.GetDeviceHasHDD())
    return m_admin.GetDriveSpace(iTotal, iUsed, m_locations);

  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR Enigma2::GetTunerSignal(PVR_SIGNAL_STATUS& signalStatus)
{
  if (m_currentChannel >= 0)
  {
    const std::shared_ptr<Channel> channel = m_channels.GetChannel(m_currentChannel);

    strncpy(signalStatus.strServiceName, channel->GetChannelName().c_str(), sizeof(signalStatus.strServiceName) - 1);
    strncpy(signalStatus.strProviderName, channel->GetProviderName().c_str(), sizeof(signalStatus.strProviderName) - 1);

    time_t now = time(nullptr);
    if ((now - m_lastSignalStatusUpdateSeconds) >= POLL_INTERVAL_SECONDS)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Calling backend for Signal Status after interval of %d seconds", __FUNCTION__, POLL_INTERVAL_SECONDS);

      if (!m_admin.GetTunerSignal(m_signalStatus, channel))
      {
        return PVR_ERROR_SERVER_ERROR;
      }
      m_lastSignalStatusUpdateSeconds = now;
    }
  }

  signalStatus.iSNR = m_signalStatus.m_snrPercentage;
  signalStatus.iBER = m_signalStatus.m_ber;
  signalStatus.iSignal = m_signalStatus.m_signalStrength;
  strncpy(signalStatus.strAdapterName, m_signalStatus.m_adapterName.c_str(), sizeof(signalStatus.strAdapterName) - 1);
  strncpy(signalStatus.strAdapterStatus, m_signalStatus.m_adapterStatus.c_str(), sizeof(signalStatus.strAdapterStatus) - 1);

  return PVR_ERROR_NO_ERROR;
}