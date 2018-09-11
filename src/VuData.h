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
#include "Timers.h"
#include "p8-platform/threads/threads.h"
#include "tinyxml.h"

#define CHANNELDATAVERSION  2

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

struct VuEPGEntry 
{
  int iEventId;
  std::string strServiceReference;
  std::string strTitle;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  std::string strPlotOutline;
  std::string strPlot;
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
  bool bInitialEPG;
  int iUniqueId;
  int iChannelNumber;
  std::string strGroupName;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strM3uURL;
  std::string strIconPath;
};

struct VuRecording
{
  std::string strRecordingId;
  time_t startTime;
  int iDuration;
  int iLastPlayedPosition;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
  std::string strDirectory;
  std::string strIconPath;
};
 
class Vu  : public P8PLATFORM::CThread
{
private:

  // members
  void *m_writeHandle;
  void *m_readHandle;
  std::string m_strEnigmaVersion;
  std::string m_strImageVersion;
  std::string m_strWebIfVersion;
  bool  m_bIsConnected;
  std::string m_strServerName;
  std::string m_strURL;
  int m_iNumRecordings;
  int m_iNumChannelGroups;
  int m_iCurrentChannel;
  unsigned int m_iUpdateTimer;
  std::vector<VuChannel> m_channels;
  std::vector<VuRecording> m_recordings;
  std::vector<VuChannelGroup> m_groups;
  std::vector<std::string> m_locations;

  vuplus::Timers my_timers = vuplus::Timers(*this);

  P8PLATFORM::CMutex m_mutex;
  P8PLATFORM::CCondition<bool> m_started;

  bool m_bUpdating;

  // functions

  std::string GetChannelIconPath(std::string strChannelName);

  std::string GetGroupServiceReference(std::string strGroupName);
  bool LoadChannels(std::string strServerReference, std::string strGroupName);
  bool LoadChannels();
  bool LoadChannelGroups();
  bool LoadLocations();
  bool GetDeviceInfo();
  std::string GetStreamURL(std::string& strM3uURL);

  // helper functions
  static long TimeStringToSeconds(const std::string &timeString);
  bool CheckForGroupUpdate();
  bool CheckForChannelUpdate();
  std::string& Escape(std::string &s, std::string from, std::string to);

  bool IsInRecordingFolder(std::string);
  void TransferRecordings(ADDON_HANDLE handle);

protected:
  virtual void *Process(void);

public:
  Vu(void);
  ~Vu();

  //helper functions
  int GetChannelNumber(std::string strServiceReference);
  std::vector<VuChannel> getChannels();
  std::string getConnectionURL();
  std::vector<std::string> getLocations();
  std::string GetHttpXML(std::string& url);
  bool SendSimpleCommand(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false);
  std::string URLEncodeInline(const std::string& sSrc);

  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  const std::string GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  const char * GetServerName();
  bool IsConnected(); 
  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  std::string GetChannelURL(const PVR_CHANNEL &channelinfo);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  PVR_ERROR GetInitialEPGForChannel(ADDON_HANDLE handle, const VuChannel &channel, time_t iStart, time_t iEnd);
  bool GetInitialEPGForGroup(VuChannelGroup &group);
  void GetTimerTypes(PVR_TIMER_TYPE types[], int *size);
  int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  bool GetRecordingFromLocation(std::string strRecordingFolder);
  unsigned int GetRecordingsAmount();
  PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
  std::string  GetRecordingURL(const PVR_RECORDING &recinfo);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  unsigned int GetNumChannelGroups(void);
  PVR_ERROR    GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  void SendPowerstate();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  bool Open();
  void Action();
  bool m_bInitialEPG;
};

