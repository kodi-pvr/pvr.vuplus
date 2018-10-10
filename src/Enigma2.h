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
#include "enigma2/RecordingReader.h"
#include "enigma2/Settings.h"
#include "enigma2/Timers.h"
#include "enigma2/data/BaseEntry.h"
#include "enigma2/data/Channel.h"
#include "enigma2/data/ChannelGroup.h"
#include "enigma2/data/EPGEntry.h"
#include "enigma2/data/RecordingEntry.h"
#include "enigma2/extract/EpgEntryExtractor.h"

#include "tinyxml.h"
#include "p8-platform/threads/threads.h"

class Enigma2  : public P8PLATFORM::CThread
{
public:
  Enigma2(const enigma2::Settings &settings);
  ~Enigma2();

  const std::string SERVICE_REF_ICON_PREFIX = "1:0:1:";
  const std::string SERVICE_REF_ICON_POSTFIX = ":0:0:0";

  inline unsigned int GenerateWebIfVersionNum(unsigned int major, unsigned int minor, unsigned int patch)
  {
    return (major << 16 | minor << 8 | patch);
  };

  enigma2::Settings &GetSettings()
  { 
    return m_settings; 
  };

  //device and helper functions
  bool Open();
  void SendPowerstate();
  const char * GetServerName() const;
  unsigned int GetWebIfVersion() const;
  bool IsConnected() const; 
  std::string GetConnectionURL() const;
  std::vector<std::string> GetLocations() const;
  std::string GetHttpXML(const std::string& url) const;
  bool SendSimpleCommand(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false) const;
  std::string URLEncodeInline(const std::string& sSrc) const;
  bool GetGenRepeatTimersEnabled() const;
  int GetNumGenRepeatTimers() const;
  bool GetAutoTimersEnabled() const;

  
  //groups, channels and EPG
  unsigned int GetNumChannelGroups(void) const;
  PVR_ERROR    GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  int GetChannelsAmount(void) const;
  int GetChannelNumber(std::string strServiceReference) const;
  std::vector<enigma2::data::Channel> GetChannels() const;
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  bool GetInitialEPGForGroup(enigma2::data::ChannelGroup &group);
  PVR_ERROR GetInitialEPGForChannel(ADDON_HANDLE handle, const enigma2::data::Channel &channel, time_t iStart, time_t iEnd);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  //live streams, recordings and Timers
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  const std::string GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  unsigned int GetRecordingsAmount();
  PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
  std::string  GetRecordingURL(const PVR_RECORDING &recinfo);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  bool GetRecordingFromLocation(std::string strRecordingFolder);
  std::string GetRecordingPath() const;
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
  // functions
  bool GetDeviceInfo();
  static unsigned int GetWebIfVersion(std::string versionString);
  bool LoadChannelGroups();
  std::string GetGroupServiceReference(std::string strGroupName);
  bool LoadChannels(std::string groupServiceReference, std::string groupName);
  bool LoadChannels();
  std::string GetChannelIconPath(std::string strChannelName);
  bool LoadLocations();
  bool CheckIfAllChannelsHaveInitialEPG() const;

  // helper functions
  std::string GetStreamURL(const std::string& strM3uURL);
  static long TimeStringToSeconds(const std::string &timeString);
  std::string& Escape(std::string &s, std::string from, std::string to);
  bool IsInRecordingFolder(std::string);
  void TransferRecordings(ADDON_HANDLE handle);

  // members
  void *m_writeHandle;
  void *m_readHandle;
  std::string m_strEnigmaVersion;
  std::string m_strImageVersion;
  std::string m_strWebIfVersion;
  unsigned int m_iWebIfVersion;
  bool m_bIsConnected = false;
  std::string m_strServerName = "Enigma2";
  std::string m_strURL;
  int m_iCurrentChannel = -1;
  unsigned int m_iUpdateTimer = 0;
  std::vector<enigma2::data::Channel> m_channels;
  std::vector<enigma2::data::RecordingEntry> m_recordings;
  std::vector<enigma2::data::ChannelGroup> m_groups;
  std::vector<std::string> m_locations;

  enigma2::Timers my_timers = enigma2::Timers(*this);
  enigma2::Settings &m_settings = enigma2::Settings::GetInstance();
  std::unique_ptr<enigma2::extract::EpgEntryExtractor> m_entryExtractor;

  P8PLATFORM::CMutex m_mutex;
  P8PLATFORM::CCondition<bool> m_started;

  bool m_bUpdating = false;
  bool m_bAllChannelsHaveInitialEPG = false;
};