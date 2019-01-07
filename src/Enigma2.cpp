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

#include <algorithm>
#include <iostream> 
#include <fstream> 
#include <string>
#include <regex>
#include <stdlib.h>

#include "client.h" 
#include "enigma2/utilities/CurlFile.h"
#include "enigma2/utilities/LocalizedString.h"
#include "enigma2/utilities/Logger.h"
#include "enigma2/utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include <p8-platform/util/StringUtils.h>

using namespace ADDON;
using namespace P8PLATFORM;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

Enigma2::Enigma2() 
{
  m_lastUpdateTimeSeconds = time(nullptr);
}

Enigma2::~Enigma2() 
{
  CLockObject lock(m_mutex);
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.ClearChannels();  
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal timers list...", __FUNCTION__);
  m_timers.ClearTimers();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal recordings list...", __FUNCTION__);
  m_recordings.ClearRecordings();
  
  Logger::Log(LEVEL_DEBUG, "%s Removing internal group list...", __FUNCTION__);
  m_channelGroups.ClearChannelGroups();
  m_isConnected = false;
}

/***************************************************************************
 * Device and helpers
 **************************************************************************/

bool Enigma2::Open()
{
  CLockObject lock(m_mutex);

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
      return false;
    }
  } 
  m_isConnected = m_admin.Initialise();

  if (!m_isConnected)
  {
    Logger::Log(LEVEL_ERROR, "%s It seem's that the webinterface cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30515).c_str());
    return false;
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

      return false;
    }

    if (!m_channels.LoadChannels(m_channelGroups))
    {
      Logger::Log(LEVEL_ERROR, "%s No channels found, please check the addon channel settings, exiting", __FUNCTION__);
      XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30517).c_str());

      return false;
    }
  }
  m_timers.AddTimerChangeWatcher(&m_dueRecordingUpdate);
  m_timers.TimerUpdates();

  Logger::Log(LEVEL_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 

  return IsRunning(); 
}

void *Enigma2::Process()
{
  Logger::Log(LEVEL_DEBUG, "%s - starting", __FUNCTION__);

  // Wait for the initial EPG update to complete 
  bool bwait = true;
  int cycles = 0;

  while (bwait)
  {
    if (cycles == 30)
      bwait = false;

    cycles++;

    if (!m_epg.IsInitialEpgCompleted())
      Sleep(5 * 1000);
  }

  // Trigger "Real" EPG updates 
  m_epg.TriggerEpgUpdatesForChannels();

  while(!IsStopped())
  {
    Sleep(5 * 1000);
     
    time_t currentUpdateTimeSeconds = time(nullptr);
    m_updateTimer += static_cast<unsigned int>(currentUpdateTimeSeconds - m_lastUpdateTimeSeconds);
    m_lastUpdateTimeSeconds = currentUpdateTimeSeconds;

    if (m_dueRecordingUpdate || m_updateTimer >= (m_settings.GetUpdateIntervalMins() * 60)) 
    {
      m_updateTimer = 0;
 
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      Logger::Log(LEVEL_INFO, "%s Perform Updates!", __FUNCTION__);

      if (m_settings.GetAutoTimerListCleanupEnabled()) 
      {
        m_timers.RunAutoTimerListCleanup();
      }
      m_timers.TimerUpdates();

      m_dueRecordingUpdate = false;

      PVR->TriggerRecordingUpdate();
    }
  }

  //CLockObject lock(m_mutex);
  m_started.Broadcast();

  return nullptr;
}

void Enigma2::SendPowerstate()
{
  CLockObject lock(m_mutex);

  m_admin.SendPowerstate();
}

const char * Enigma2::GetServerName() const
{
  return m_admin.GetServerName().c_str();  
}

const char * Enigma2::GetServerVersion() const
{
  return m_admin.GetWebIfVersion().c_str();  
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

PVR_ERROR Enigma2::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
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

  for (auto &channel : channels)
    PVR->TransferChannelEntry(handle, &channel);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * EPG
 **************************************************************************/

PVR_ERROR Enigma2::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  return m_epg.GetEPGForChannel(handle, channel, iStart, iEnd);
}

/***************************************************************************
 * Livestream
 **************************************************************************/
bool Enigma2::OpenLiveStream(const PVR_CHANNEL &channelinfo)
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
      if(!WebUtils::SendSimpleCommand(strCmd, strResult))
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

const std::string Enigma2::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
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

unsigned int Enigma2::GetRecordingsAmount() 
{
  return m_recordings.GetNumRecordings();
}

PVR_ERROR Enigma2::GetRecordings(ADDON_HANDLE handle)
{
  m_recordings.LoadRecordings();

  std::vector<PVR_RECORDING> recordings;
  {
    CLockObject lock(m_mutex);
    m_recordings.GetRecordings(recordings);
  }

  Logger::Log(LEVEL_DEBUG, "%s - recordings available '%d'", __FUNCTION__, recordings.size());

  for (const auto& recording : recordings)
    PVR->TransferRecordingEntry(handle, &recording);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  return m_recordings.DeleteRecording(recinfo);
}

RecordingReader *Enigma2::OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  CLockObject lock(m_mutex);
  std::time_t now = std::time(nullptr), start = 0, end = 0;
  std::string channelName = recinfo.strChannelName;
  auto timer = m_timers.GetTimer([&](const Timer &timer)
      {
        return timer.isRunning(&now, &channelName);
      });
  if (timer)
  {
    start = timer->GetStartTime();
    end = timer->GetEndTime();
  }

  return new RecordingReader(m_recordings.GetRecordingURL(recinfo).c_str(), start, end, recinfo.iDuration);
}

/***************************************************************************
 * Timers
 **************************************************************************/

void Enigma2::GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  std::vector<PVR_TIMER_TYPE> timerTypes;
  {
    CLockObject lock(m_mutex);
    m_timers.GetTimerTypes(timerTypes);
  }

  int i = 0;
  for (auto &timerType : timerTypes)
    types[i++] = timerType;
  *size = timerTypes.size();
  Logger::Log(LEVEL_NOTICE, "Transfered %u timer types", *size);
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

  for (auto &timer : timers)
    PVR->TransferTimerEntry(handle, &timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Enigma2::AddTimer(const PVR_TIMER &timer)
{
  return m_timers.AddTimer(timer);
}

PVR_ERROR Enigma2::UpdateTimer(const PVR_TIMER &timer)
{
  return m_timers.UpdateTimer(timer);
}

PVR_ERROR Enigma2::DeleteTimer(const PVR_TIMER &timer)
{
  return m_timers.DeleteTimer(timer);
}

/***************************************************************************
 * Misc
 **************************************************************************/

PVR_ERROR Enigma2::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return m_admin.GetDriveSpace(iTotal, iUsed, m_locations);
}

PVR_ERROR Enigma2::GetTunerSignal(PVR_SIGNAL_STATUS &signalStatus)
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

      if (!m_admin.GetTunerSignal(m_signalStatus, channel->GetServiceReference()))
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