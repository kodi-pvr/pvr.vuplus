/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"

#include "Enigma2.h"
#include "enigma2/RecordingReader.h"
#include "enigma2/Settings.h"
#include "enigma2/StreamReader.h"
#include "enigma2/TimeshiftBuffer.h"
#include "enigma2/utilities/LocalizedString.h"
#include "enigma2/utilities/Logger.h"

#include <cstdlib>

#include <kodi/xbmc_pvr_dll.h>
#include <p8-platform/util/StringUtils.h>

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool m_created = false;
ADDON_STATUS m_currentStatus = ADDON_STATUS_UNKNOWN;
IStreamReader* streamReader = nullptr;
int m_streamReadChunkSize = 64;
RecordingReader* recordingReader = nullptr;
Settings& settings = Settings::GetInstance();

CHelper_libXBMC_addon* XBMC = nullptr;
CHelper_libXBMC_pvr* PVR = nullptr;
Enigma2* enigma = nullptr;

template<typename T> void SafeDelete(T*& p)
{
  if (p)
  {
    delete p;
    p = nullptr;
  }
}

extern "C"
{

/***************************************************************************
 * Addon Calls
 **************************************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return m_currentStatus;

  PVR_PROPERTIES* pvrProps = reinterpret_cast<PVR_PROPERTIES*>(props);

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SafeDelete(XBMC);
    m_currentStatus = ADDON_STATUS_PERMANENT_FAILURE;
    return m_currentStatus;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SafeDelete(PVR);
    SafeDelete(XBMC);
    m_currentStatus = ADDON_STATUS_PERMANENT_FAILURE;
    return m_currentStatus;
  }

  Logger::Log(LEVEL_DEBUG, "%s - Creating VU+ PVR-Client", __FUNCTION__);

  m_currentStatus = ADDON_STATUS_UNKNOWN;

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([](LogLevel level, const char* message)
  {
    /* Don't log trace messages unless told so */
    if (level == LogLevel::LEVEL_TRACE && !Settings::GetInstance().GetTraceDebug())
      return;

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

    if (addonLevel == addon_log_t::LOG_DEBUG && Settings::GetInstance().GetNoDebug())
      return;

    if (addonLevel == addon_log_t::LOG_DEBUG && Settings::GetInstance().GetDebugNormal())
      addonLevel = addon_log_t::LOG_NOTICE;

    XBMC->Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.vuplus");

  Logger::Log(LogLevel::LEVEL_INFO, "%s starting PVR client...", __FUNCTION__);

  settings.ReadFromAddon();

  enigma = new Enigma2(pvrProps);
  enigma->Start();

  m_currentStatus = ADDON_STATUS_OK;
  m_created = true;
  return m_currentStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
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

  SafeDelete(enigma);
  SafeDelete(PVR);
  SafeDelete(XBMC);

  m_currentStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char* settingName, const void* settingValue)
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
  if (!enigma || !enigma->IsConnected())
    return;

  if (enigma)
    enigma->OnSleep();
}

void OnSystemWake()
{
  if (!enigma || !enigma->IsConnected())
    return;

  if (enigma)
    enigma->OnWake();
}

void OnPowerSavingActivated() {}

void OnPowerSavingDeactivated() {}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsEPGEdl             = false;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsRecordingsUndelete = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bSupportsChannelScan        = false;
  pCapabilities->bSupportsChannelSettings    = false;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = false;
  pCapabilities->bSupportsRecordingPlayCount = settings.SupportsEditingRecordings() && settings.GetStoreRecordingLastPlayedAndCount();
  pCapabilities->bSupportsLastPlayedPosition = settings.SupportsEditingRecordings() && settings.GetStoreRecordingLastPlayedAndCount();
  pCapabilities->bSupportsRecordingEdl       = true;
  pCapabilities->bSupportsRecordingsRename   = settings.SupportsEditingRecordings();
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;
  pCapabilities->bSupportsAsyncEPGTransfer = false;

  return PVR_ERROR_NO_ERROR;
}

const char* GetBackendName()
{
  static const char* backendName = enigma ? enigma->GetServerName() : LocalizedString(30081).c_str(); //unknown
  return backendName;
}

const char* GetBackendVersion()
{
  static const char* backendVersion = enigma ? enigma->GetServerVersion() : LocalizedString(30081).c_str(); //unknown
  return backendVersion;
}

static std::string connectionString;

const char* GetConnectionString()
{
  if (enigma)
    connectionString = StringUtils::Format("%s%s", settings.GetHostname().c_str(), enigma->IsConnected() ? "" : LocalizedString(30082).c_str()); // (Not connected!)
  else
    connectionString = StringUtils::Format("%s (%s!)", settings.GetHostname().c_str(), LocalizedString(30083).c_str()); //addon error
  return connectionString.c_str();
}

const char* GetBackendHostname()
{
  return settings.GetHostname().c_str();
}

PVR_ERROR GetDriveSpace(long long* iTotal, long long* iUsed)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetDriveSpace(iTotal, iUsed);
}


PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& signalStatus)
{
  // SNR = Signal to Noise Ratio - which means signal quality
  // AGC = Automatic Gain Control - which means signal strength
  // BER = Bit Error Rate - which shows the error rate of the signal.
  // UNC = There is not notion of UNC on enigma devices

  // So, SNR and AGC should be as high as possible.
  // BER should be as low as possible, like 0. It can be higher, if your other values are higher.

  enigma->GetTunerSignal(signalStatus);

  Logger::Log(LEVEL_DEBUG, "%s Tuner Details - name: %s, status: %s", __FUNCTION__, signalStatus.strAdapterName, signalStatus.strAdapterStatus);
  Logger::Log(LEVEL_DEBUG, "%s Service Details - service: %s, provider: %s", __FUNCTION__, signalStatus.strServiceName, signalStatus.strProviderName);
  // For some reason the iSNR and iSignal values need to multiplied by 655!
  Logger::Log(LEVEL_DEBUG, "%s Signal - snrPercent: %d, ber: %u, signal strength: %d", __FUNCTION__, signalStatus.iSNR / 655, signalStatus.iBER, signalStatus.iSignal / 655);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * ChannelGroups
 **************************************************************************/

int GetChannelGroupsAmount()
{
  if (!enigma || !enigma->IsConnected())
    return 0;

  return enigma->GetNumChannelGroups();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetChannelGroupMembers(handle, group);
}

/***************************************************************************
 * EPG and Channels
 **************************************************************************/

PVR_ERROR SetEPGTimeFrame(int epgMaxDays)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  enigma->SetEPGTimeFrame(epgMaxDays);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetEPGForChannel(handle, iChannelUid, iStart, iEnd);
}

int GetChannelsAmount()
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

PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!settings.SetStreamProgramID() && !enigma->IsIptvStream(*channel))
    return PVR_ERROR_NOT_IMPLEMENTED;

  //
  // We only use this function to set the program number which comes with every Enigma2 channel. For providers that
  // use MPTS it allows the FFMPEG Demux to identify the correct Program/PID.
  //

  if (!channel || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  *iPropertiesCount = 0;

  if (enigma->IsIptvStream(*channel))
  {
    strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, enigma->GetLiveStreamURL(*channel).c_str(), sizeof(properties[0].strValue) - 1);
    (*iPropertiesCount)++;
  }

  if (settings.SetStreamProgramID())
  {
    const std::string strStreamProgramNumber = std::to_string(enigma->GetChannelStreamProgramNumber(*channel));

    Logger::Log(LEVEL_NOTICE, "%s - for channel: %s, set Stream Program Number to %s - %s", __FUNCTION__, channel->strChannelName, strStreamProgramNumber.c_str(), enigma->GetLiveStreamURL(*channel).c_str());

    strncpy(properties[0].strName, "program", sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, strStreamProgramNumber.c_str(), sizeof(properties[0].strValue) - 1);
    (*iPropertiesCount)++;
  }

  return PVR_ERROR_NO_ERROR;
}

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
bool OpenLiveStream(const PVR_CHANNEL& channel)
{
  if (!enigma || !enigma->IsConnected())
    return false;

  if (!enigma->OpenLiveStream(channel))
    return false;

  /* queue a warning if the timeshift buffer path does not exist */
  if (settings.GetTimeshift() != Timeshift::OFF && !settings.IsTimeshiftBufferPathValid())
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30514).c_str());

  const std::string streamURL = enigma->GetLiveStreamURL(channel);
  streamReader = new StreamReader(streamURL, settings.GetReadTimeoutSecs());
  if (settings.GetTimeshift() == Timeshift::ON_PLAYBACK)
    streamReader = new TimeshiftBuffer(streamReader, settings.GetTimeshiftBufferPath(), settings.GetReadTimeoutSecs());

  return streamReader->Start();
}

void CloseLiveStream()
{
  if (enigma)
    enigma->CloseLiveStream();
  SafeDelete(streamReader);
}

bool IsRealTimeStream()
{
  return (streamReader) ? streamReader->IsRealTime() : false;
}

bool CanPauseStream()
{
  if (!enigma || !enigma->IsConnected())
    return false;

  if (settings.GetTimeshift() != Timeshift::OFF && streamReader)
    return (streamReader->IsTimeshifting() || settings.IsTimeshiftBufferPathValid());

  return false;
}

bool CanSeekStream()
{
  if (!enigma || !enigma->IsConnected())
    return false;

  return (settings.GetTimeshift() != Timeshift::OFF);
}

int ReadLiveStream(unsigned char* buffer, unsigned int size)
{
  return (streamReader) ? streamReader->ReadData(buffer, size) : 0;
}

long long SeekLiveStream(long long position, int whence)
{
  return (streamReader) ? streamReader->Seek(position, whence) : -1;
}

long long LengthLiveStream()
{
  return (streamReader) ? streamReader->Length() : -1;
}

PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES* times)
{
  if (!times)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (streamReader)
  {
    times->startTime = streamReader->TimeStart();
    times->ptsStart = 0;
    times->ptsBegin = 0;
    times->ptsEnd = (!streamReader->IsTimeshifting()) ? 0
      : (streamReader->TimeEnd() - streamReader->TimeStart()) * DVD_TIME_BASE;

    return PVR_ERROR_NO_ERROR;
  }
  else if (recordingReader)
  {
    times->startTime = 0;
    times->ptsStart = 0;
    times->ptsBegin = 0;
    times->ptsEnd = static_cast<int64_t>(recordingReader->CurrentDuration()) * DVD_TIME_BASE;

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

void PauseStream(bool paused)
{
  if (!enigma || !enigma->IsConnected())
    return;

  /* start timeshift on pause */
  if (paused && settings.GetTimeshift() == Timeshift::ON_PAUSE &&
      streamReader && !streamReader->IsTimeshifting() &&
      settings.IsTimeshiftBufferPathValid())
  {
    streamReader = new TimeshiftBuffer(streamReader, settings.GetTimeshiftBufferPath(), settings.GetReadTimeoutSecs());
    streamReader->Start();
  }
}

/***************************************************************************
 * Recordings
 **************************************************************************/

int GetRecordingsAmount(bool deleted)
{
  if (!enigma || !enigma->IsConnected())
    return 0;

  return enigma->GetRecordingsAmount(deleted);
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetRecordings(handle, deleted);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING& recording)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->DeleteRecording(recording);
}

PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->UndeleteRecording(recording);
}

PVR_ERROR DeleteAllRecordingsFromTrash()
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->DeleteAllRecordingsFromTrash();
}

PVR_ERROR GetRecordingEdl(const PVR_RECORDING& recinfo, PVR_EDL_ENTRY edl[], int* size)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (!settings.GetRecordingEDLsEnabled())
  {
    *size = 0;
    return PVR_ERROR_NO_ERROR;
  }

  return enigma->GetRecordingEdl(recinfo, edl, size);
}

PVR_ERROR RenameRecording(const PVR_RECORDING& recording)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->RenameRecording(recording);
}

PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->SetRecordingPlayCount(recording, count);
}

PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastPlayedPosition)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->SetRecordingLastPlayedPosition(recording, lastPlayedPosition);
}

int GetRecordingLastPlayedPosition(const PVR_RECORDING& recording)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->GetRecordingLastPlayedPosition(recording);
}

/***************************************************************************
 * Recording Streams
 **************************************************************************/

PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!settings.SetStreamProgramID())
    return PVR_ERROR_NOT_IMPLEMENTED;

  //
  // We only use this function to set the program number which may comes with every Enigma2 recording. For providers that
  // use MPTS it allows the FFMPEG Demux to identify the correct Program/PID.
  //

  if (!recording || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  if (enigma->HasRecordingStreamProgramNumber(*recording))
  {
    const std::string strStreamProgramNumber = std::to_string(enigma->GetRecordingStreamProgramNumber(*recording));

    Logger::Log(LEVEL_NOTICE, "%s - for recording for channel: %s, set Stream Program Number to %s - %s", __FUNCTION__, recording->strChannelName, strStreamProgramNumber.c_str(), recording->strRecordingId);

    strncpy(properties[0].strName, "program", sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, strStreamProgramNumber.c_str(), sizeof(properties[0].strValue) - 1);
    *iPropertiesCount = 1;
  }

  return PVR_ERROR_NO_ERROR;
}

bool OpenRecordedStream(const PVR_RECORDING& recording)
{
  if (recordingReader)
    SafeDelete(recordingReader);

  if (!enigma || !enigma->IsConnected())
    return false;

  recordingReader = enigma->OpenRecordedStream(recording);
  return recordingReader->Start();
}

void CloseRecordedStream()
{
  if (recordingReader)
    SafeDelete(recordingReader);
}

int ReadRecordedStream(unsigned char* buffer, unsigned int size)
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

long long LengthRecordedStream()
{
  if (!recordingReader)
    return -1;

  return recordingReader->Length();
}

/***************************************************************************
 * Timers
 **************************************************************************/

PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int* size)
{
  *size = 0;
  if (enigma && enigma->IsConnected())
    enigma->GetTimerTypes(types, size);
  return PVR_ERROR_NO_ERROR;
}

int GetTimersAmount()
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

PVR_ERROR AddTimer(const PVR_TIMER& timer)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER& timer, bool bForceDelete)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->DeleteTimer(timer);
}

PVR_ERROR UpdateTimer(const PVR_TIMER& timer)
{
  if (!enigma || !enigma->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return enigma->UpdateTimer(timer);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort() { return; }
DemuxPacket* DemuxRead() { return nullptr; }
void FillBuffer(bool mode) {}
PVR_ERROR OpenDialogChannelScan() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA& item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxReset() {}
void DemuxFlush() {}
bool SeekTime(double, bool, double*) { return false; }
void SetSpeed(int){};
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagPlayable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int* size) { return PVR_ERROR_NOT_IMPLEMENTED; }
}
