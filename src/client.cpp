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

#include <stdlib.h>

#include "Enigma2.h"
#include "enigma2/RecordingReader.h"
#include "enigma2/Settings.h"
#include "enigma2/StreamReader.h"
#include "enigma2/TimeshiftBuffer.h"
#include "enigma2/utilities/LocalizedString.h"
#include "enigma2/utilities/Logger.h"

#include "p8-platform/util/util.h"
#include <p8-platform/util/StringUtils.h>
#include "xbmc_pvr_dll.h"

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool m_created = false;
ADDON_STATUS m_currentStatus = ADDON_STATUS_UNKNOWN;
IStreamReader *streamReader = nullptr;
int m_streamReadChunkSize = 64;
RecordingReader *recordingReader = nullptr;
Settings &settings = Settings::GetInstance();

CHelper_libXBMC_addon *XBMC = nullptr;
CHelper_libXBMC_pvr *PVR = nullptr;
Enigma2 *enigma = nullptr;

extern "C" {

/***************************************************************************
 * Addon Calls
 **************************************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  Logger::Log(LEVEL_DEBUG, "%s - Creating VU+ PVR-Client", __FUNCTION__);

  m_currentStatus = ADDON_STATUS_UNKNOWN;

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([](LogLevel level, const char *message)
  {
    /* Convert the log level */
    addon_log_t addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_ERROR:
        addonLevel = addon_log_t::LOG_ERROR;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = addon_log_t::LOG_INFO;
        break;
      case LogLevel::LEVEL_NOTICE:
        addonLevel = addon_log_t::LOG_NOTICE;
        break;
      default:
        addonLevel = addon_log_t::LOG_DEBUG;
    }

    /* Don't log trace messages unless told so */
    //if (level == LogLevel::LEVEL_TRACE && !Settings::GetInstance().GetTraceDebug())
      //return;

    XBMC->Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.vuplus");

  Logger::Log(LogLevel::LEVEL_INFO, "%s starting PVR client...", __FUNCTION__);  

  settings.ReadFromAddon();

  enigma = new Enigma2();
  if (!enigma->Open()) 
  {
    SAFE_DELETE(enigma);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_currentStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_currentStatus;
  }

  m_currentStatus = ADDON_STATUS_OK;
  m_created = true;
  return m_currentStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_currentStatus == ADDON_STATUS_OK && !enigma->IsConnected())
    m_currentStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_currentStatus;
}

void ADDON_Destroy()
{
  if (m_created)
  {
    m_created = false;
  }

  if (enigma)
  {
    enigma->SendPowerstate();
  }
  
  SAFE_DELETE(enigma);
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);

  m_currentStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  if (!XBMC || !enigma)
    return ADDON_STATUS_OK;

  return settings.SetValue(settingName, settingValue);
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep()
{
}

void OnSystemWake()
{
}

void OnPowerSavingActivated()
{
}

void OnPowerSavingDeactivated()
{
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsRecordingsUndelete = false;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bSupportsChannelScan        = false;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = false;
  pCapabilities->bSupportsLastPlayedPosition = false;
  pCapabilities->bSupportsRecordingsRename   = false;
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *backendName = enigma ? enigma->GetServerName() : LocalizedString(60081).c_str(); //unknown
  return backendName;
}

const char *GetBackendVersion(void)
{
  static const char *backendVersion = enigma ? enigma->GetServerVersion() : LocalizedString(60081).c_str(); //unknown
  return backendVersion;
}

static std::string connectionString;

const char *GetConnectionString(void)
{
  if (enigma)
    connectionString = StringUtils::Format("%s%s", settings.GetHostname().c_str(), enigma->IsConnected() ? "" : LocalizedString(60082).c_str()); // (Not connected!)
  else
    connectionString = StringUtils::Format("%s (%s!)", settings.GetHostname().c_str(), LocalizedString(60083).c_str()); //addon error
  return connectionString.c_str();
}

const char *GetBackendHostname(void)
{
  return settings.GetHostname().c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return enigma->GetDriveSpace(iTotal, iUsed);
}


PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  // the RS api doesn't provide information about signal quality (yet)

  PVR_STRCPY(signalStatus.strAdapterName, LocalizedString(60084).c_str()); //Enigma2 Media Server
  PVR_STRCPY(signalStatus.strAdapterStatus, LocalizedString(60085).c_str()); //OK
  return PVR_ERROR_NO_ERROR;
}


/***************************************************************************
 * ChannelGroups
 **************************************************************************/

int GetChannelGroupsAmount(void)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetNumChannelGroups();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetChannelGroupMembers(handle, group);
}

/***************************************************************************
 * EPG and Channels
 **************************************************************************/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!enigma || !enigma->IsConnected())
    return 0;

  return enigma->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetChannels(handle, bRadio);
}

/***************************************************************************
 * Live Streams
 **************************************************************************/

PVR_ERROR GetStreamReadChunkSize(int* chunksize)
{
  if (!chunksize)
    return PVR_ERROR_INVALID_PARAMETERS;
  int size = settings.GetStreamReadChunkSizeKb();
  if (!size)
    return PVR_ERROR_NOT_IMPLEMENTED;
  *chunksize = settings.GetStreamReadChunkSizeKb() * 1024;
  return PVR_ERROR_NO_ERROR;
}

/* live stream functions */
bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (!enigma || !enigma->IsConnected())
    return false;

  if (!enigma->OpenLiveStream(channel))
    return false;

  /* queue a warning if the timeshift buffer path does not exist */
  if (settings.GetTimeshift() != Timeshift::OFF
      && !settings.IsTimeshiftBufferPathValid())
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30514).c_str());

  std::string streamURL = enigma->GetLiveStreamURL(channel);
  streamReader = new StreamReader(streamURL, settings.GetReadTimeoutSecs());
  if (settings.GetTimeshift() == Timeshift::ON_PLAYBACK)
    streamReader = new TimeshiftBuffer(streamReader, settings.GetTimeshiftBufferPath(), settings.GetReadTimeoutSecs());
  
  return streamReader->Start();
}

void CloseLiveStream(void)
{
  enigma->CloseLiveStream();
  SAFE_DELETE(streamReader);
}

bool IsRealTimeStream()
{
  return (streamReader) ? streamReader->IsRealTime() : false;
}

bool CanPauseStream(void)
{
  if (!enigma)
    return false;

  if (settings.GetTimeshift() != Timeshift::OFF && streamReader)
    return (streamReader->IsTimeshifting() || settings.IsTimeshiftBufferPathValid());

  return false;
}

bool CanSeekStream(void)
{
  if (!enigma)
    return false;

  return (settings.GetTimeshift() != Timeshift::OFF);
}

int ReadLiveStream(unsigned char *buffer, unsigned int size)
{
  return (streamReader) ? streamReader->ReadData(buffer, size) : 0;
}

long long SeekLiveStream(long long position, int whence)
{
  return (streamReader) ? streamReader->Seek(position, whence) : -1;
}

long long LengthLiveStream(void)
{
  return (streamReader) ? streamReader->Length() : -1;
}

bool IsTimeshifting(void)
{
  return (streamReader && streamReader->IsTimeshifting());
}

PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times)
{
  if (!times)
    return PVR_ERROR_INVALID_PARAMETERS;
  if (streamReader)
  {
    times->startTime = streamReader->TimeStart();
    times->ptsStart  = 0;
    times->ptsBegin  = 0;
    times->ptsEnd    = (!streamReader->IsTimeshifting()) ? 0
      : (streamReader->TimeEnd() - streamReader->TimeStart()) * DVD_TIME_BASE;
    
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_NOT_IMPLEMENTED;
}

void PauseStream(bool paused)
{
  if (!enigma)
    return;

  /* start timeshift on pause */
  if (paused && settings.GetTimeshift() == Timeshift::ON_PAUSE
      && streamReader && !streamReader->IsTimeshifting()
      && settings.IsTimeshiftBufferPathValid())
  {
    streamReader = new TimeshiftBuffer(streamReader, settings.GetTimeshiftBufferPath(), settings.GetReadTimeoutSecs());
    (void)streamReader->Start();
  }
}

/***************************************************************************
 * Recordings
 **************************************************************************/

int GetRecordingsAmount(bool deleted)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->DeleteRecording(recording);
}

/***************************************************************************
 * Recording Streams
 **************************************************************************/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (recordingReader)
    SAFE_DELETE(recordingReader);
  recordingReader = enigma->OpenRecordedStream(recording);
  return recordingReader->Start();
}

void CloseRecordedStream(void)
{
  if (recordingReader)
    SAFE_DELETE(recordingReader);
}

int ReadRecordedStream(unsigned char *buffer, unsigned int size)
{
  if (!recordingReader)
    return 0;

  return recordingReader->ReadData(buffer, size);
}

long long SeekRecordedStream(long long position, int whence)
{
  if (!recordingReader)
    return 0;

  return recordingReader->Seek(position, whence);
}

long long LengthRecordedStream(void)
{
  if (!recordingReader)
    return -1;

  return recordingReader->Length();
}

/***************************************************************************
 * Timers
 **************************************************************************/

PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  *size = 0;
  if (enigma && enigma->IsConnected())
    enigma->GetTimerTypes(types, size);
  return PVR_ERROR_NO_ERROR;
}

int GetTimersAmount(void)
{
  if (!enigma || !enigma->IsConnected())
    return 0;

  return enigma->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->DeleteTimer(timer);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->UpdateTimer(timer);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; } 
PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) { return; }
DemuxPacket* DemuxRead(void) { return nullptr; }
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool SeekTime(double,bool,double*) { return false; }
void SetSpeed(int) {};
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagPlayable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
}
