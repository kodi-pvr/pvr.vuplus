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
#include "RecordingReader.h"
#include "Settings.h"
#include "Timers.h"
#include "VuBase.h"
#include "extract/EpgEntryExtractor.h"

#include "tinyxml.h"
#include "p8-platform/threads/threads.h"

class CCurlFile
{
public:
  CCurlFile(void) {};
  ~CCurlFile(void) {};

  bool Get(const std::string &strURL, std::string &strResult);
};

typedef enum VU_UPDATE_STATE
{
    VU_UPDATE_STATE_NONE,
    VU_UPDATE_STATE_FOUND,
    VU_UPDATE_STATE_UPDATED,
    VU_UPDATE_STATE_NEW
} VU_UPDATE_STATE;

struct VuEPGEntry : public VuBase
{
  int iEventId;
  std::string strServiceReference;
  int iChannelId;
  time_t startTime;
  time_t endTime;
};

struct VuChannelGroup 
{
  std::string strServiceReference;
  std::string strGroupName;
  int iGroupState;
  std::vector<VuEPGEntry> initialEPG;
};

struct VuChannel
{
  bool bRadio;
  bool bRequiresInitialEPG = true;
  int iUniqueId;
  int iChannelNumber;
  std::string strGroupName;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strM3uURL;
  std::string strIconPath;
};

struct VuRecording : public VuBase
{
  std::string strRecordingId;
  time_t startTime;
  int iDuration;
  int iLastPlayedPosition;
  std::string strStreamURL;
  std::string strChannelName;
  std::string strDirectory;
  std::string strIconPath;
};

class Vu  : public P8PLATFORM::CThread
{
public:
  Vu(const VUPLUS::Settings &settings);
  ~Vu();

  const std::string SERVICE_REF_ICON_PREFIX = "1:0:1:";
  const std::string SERVICE_REF_ICON_POSTFIX = ":0:0:0";

  inline unsigned int GenerateWebIfVersionNum(unsigned int major, unsigned int minor, unsigned int patch)
  {
    return (major << 16 | minor << 8 | patch);
  };

  VUPLUS::Settings &GetSettings()
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
  std::vector<VuChannel> GetChannels() const;
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  bool GetInitialEPGForGroup(VuChannelGroup &group);
  PVR_ERROR GetInitialEPGForChannel(ADDON_HANDLE handle, const VuChannel &channel, time_t iStart, time_t iEnd);
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
  RecordingReader *OpenRecordedStream(const PVR_RECORDING &recinfo);
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
  bool LoadChannels(std::string strServerReference, std::string strGroupName);
  bool LoadChannels();
  std::string GetChannelIconPath(std::string strChannelName);
  bool LoadLocations();
  bool CheckIfAllChannelsHaveInitialEPG() const;

  // helper functions
  std::string GetStreamURL(std::string& strM3uURL);
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
  std::string m_strServerName = "Vu";
  std::string m_strURL;
  int m_iCurrentChannel = -1;
  unsigned int m_iUpdateTimer = 0;
  std::vector<VuChannel> m_channels;
  std::vector<VuRecording> m_recordings;
  std::vector<VuChannelGroup> m_groups;
  std::vector<std::string> m_locations;

  VUPLUS::Timers my_timers = VUPLUS::Timers(*this);
  VUPLUS::Settings m_settings;
  std::unique_ptr<VUPLUS::EpgEntryExtractor> m_entryExtractor;

  P8PLATFORM::CMutex m_mutex;
  P8PLATFORM::CCondition<bool> m_started;

  bool m_bUpdating = false;
  bool m_bAllChannelsHaveInitialEPG = false;
};