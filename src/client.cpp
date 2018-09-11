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
#include "xbmc_pvr_dll.h"
#include <stdlib.h>
#include "VuData.h"
#include <p8-platform/util/StringUtils.h>
#include "p8-platform/util/util.h"
#include "StreamReader.h"
#include "TimeshiftBuffer.h"
#include "LocalizedString.h"
#include "RecordingReader.h"

#include <stdlib.h>

using namespace std;
using namespace vuplus;
using namespace ADDON;

bool            m_bCreated  = false;
ADDON_STATUS    m_CurStatus = ADDON_STATUS_UNKNOWN;
IStreamReader   *strReader  = nullptr;
int             m_streamReadChunkSize = 64;
RecordingReader *recReader  = nullptr;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strHostname             = DEFAULT_HOST;
int         g_iConnectTimeout         = DEFAULT_CONNECT_TIMEOUT;
int         g_iPortStream             = DEFAULT_STREAM_PORT;
int         g_iPortWeb                = DEFAULT_WEB_PORT;
int         g_iUpdateInterval         = DEFAULT_UPDATE_INTERVAL;
std::string g_strUsername             = "";
std::string g_strRecordingPath        = "";
std::string g_strPassword             = "";
std::string g_szUserPath              = "";
std::string g_strIconPath             = "";
bool        g_bAutomaticTimerlistCleanup = false;
bool        g_bZap                    = false;
bool        g_bOnlyCurrentLocation    = false;
bool        g_bSetPowerstate          = false;
bool        g_bOnlyOneGroup           = false;
bool        g_bOnlinePicons           = true;
bool        g_bUseSecureHTTP          = false;
bool        g_bAutoConfig             = false;
bool        g_bKeepFolders            = false;
std::string g_strOneGroup             = "";
std::string g_szClientPath            = "";
int         g_iEnableTimeshift        = TIMESHIFT_OFF;
std::string g_strTimeshiftBufferPath  = DEFAULT_TSBUFFERPATH;
int         g_iReadTimeout            = 0;

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
Vu                *VuData             = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  /* read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;
  else
    g_strHostname = DEFAULT_HOST;
  buffer[0] = 0; /* Set the end of string */

  /* read setting "user" from settings.xml */
  if (XBMC->GetSetting("user", buffer))
    g_strUsername = buffer;
  else
    g_strUsername = "";
  buffer[0] = 0; /* Set the end of string */
  
  /* read setting "recordingpath" from settings.xml */
  if (XBMC->GetSetting("recordingpath", buffer))
    g_strRecordingPath = buffer;
  else
    g_strRecordingPath = "";
  buffer[0] = 0; /* Set the end of string */

  /* read setting "pass" from settings.xml */
  if (XBMC->GetSetting("pass", buffer))
    g_strPassword = buffer;
  else
    g_strPassword = "";
  
  /* read setting "use_secure" from settings.xml */
  if (!XBMC->GetSetting("use_secure", &g_bUseSecureHTTP))
    g_bUseSecureHTTP = false;
  
  /* read setting "autoconfig" from settings.xml */
  if (!XBMC->GetSetting("autoconfig", &g_bAutoConfig))
    g_bAutoConfig = false;
  
  /* read setting "streamport" from settings.xml */
  if (!XBMC->GetSetting("streamport", &g_iPortStream))
    g_iPortStream = DEFAULT_STREAM_PORT;
  
  /* read setting "webport" from settings.xml */
  if (!XBMC->GetSetting("webport", &g_iPortWeb))
    g_iPortWeb = DEFAULT_WEB_PORT;
  
  /* read setting "onlinepicons" from settings.xml */
  if (!XBMC->GetSetting("onlinepicons", &g_bOnlinePicons))
    g_bOnlinePicons = true;
  
  /* read setting "onlycurrent" from settings.xml */
  if (!XBMC->GetSetting("onlycurrent", &g_bOnlyCurrentLocation))
    g_bOnlyCurrentLocation = false;
  
  /* read setting "setpowerstate" from settings.xml */
  if (!XBMC->GetSetting("setpowerstate", &g_bSetPowerstate))
    g_bSetPowerstate = false;
  
  /* read setting "keepfolders" from settings.xml */
  if (!XBMC->GetSetting("keepfolders", &g_bKeepFolders))
    g_bKeepFolders = false;
  
  /* read setting "zap" from settings.xml */
  if (!XBMC->GetSetting("zap", &g_bZap))
    g_bZap = false;

  /* read setting "onlyonegroup" from settings.xml */
  if (!XBMC->GetSetting("onlyonegroup", &g_bOnlyOneGroup))
    g_bOnlyOneGroup = false;
  
  /* read setting "onegroup" from settings.xml */
  if (XBMC->GetSetting("onegroup", buffer))
    g_strOneGroup = buffer;
  else
    g_strOneGroup = "";

  /* read setting "timerlistcleanup" from settings.xml */
  if (!XBMC->GetSetting("timerlistcleanup", &g_bAutomaticTimerlistCleanup))
    g_bAutomaticTimerlistCleanup = false;

  /* read setting "updateint" from settings.xml */
  if (!XBMC->GetSetting("updateint", &g_iUpdateInterval))
    g_iConnectTimeout = DEFAULT_UPDATE_INTERVAL;

  /* read setting "iconpath" from settings.xml */
  if (XBMC->GetSetting("iconpath", buffer))
    g_strIconPath = buffer;
  else
    g_strIconPath = "";

  // read setting "enabletimeshift" from settings.xml 
  if (!XBMC->GetSetting("enabletimeshift", &g_iEnableTimeshift))
    g_iEnableTimeshift = TIMESHIFT_OFF;

    // read setting "timeshiftbufferpath" from settings.xml 
  if (XBMC->GetSetting("timeshiftbufferpath", buffer) && !std::string(buffer).empty())
    g_strTimeshiftBufferPath = buffer;
  else
    g_strTimeshiftBufferPath = DEFAULT_TSBUFFERPATH;

  // read setting "enabletimeshift" from settings.xml 
  if (!XBMC->GetSetting("readtimeout", &g_iReadTimeout))
    g_iReadTimeout = 0;

  free (buffer);
}

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

  XBMC->Log(LOG_DEBUG, "%s - Creating VU+ PVR-Client", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  //g_iClientId     = pvrprops->iClientId; //removed from Frodo PVR API
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath  = pvrprops->strClientPath;

  ADDON_ReadSettings();

  VuData = new Vu;
  if (!VuData->Open()) 
  {
    SAFE_DELETE(VuData);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && !VuData->IsConnected())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (m_bCreated)
  {
    m_bCreated = false;
  }

  if (VuData)
  {
    VuData->SendPowerstate();
  }
  
  SAFE_DELETE(VuData);
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'host' from %s to %s", __FUNCTION__, g_strHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_strHostname;
    g_strHostname = (const char*) settingValue;
    if (tmp_sHostname != g_strHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "user")
  {
    string tmp_sUsername = g_strUsername;
    g_strUsername = (const char*) settingValue;
    if (tmp_sUsername != g_strUsername)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'user'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "pass")
  {
    string tmp_sPassword = g_strPassword;
    g_strPassword = (const char*) settingValue;
    if (tmp_sPassword != g_strPassword)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'pass'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "streamport")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iPortStream != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'streamport' from %u to %u", __FUNCTION__, g_iPortStream, iNewValue);
      g_iPortStream = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "webport")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iPortWeb != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'webport' from %u to %u", __FUNCTION__, g_iPortWeb, iNewValue);
      g_iPortWeb = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "enabletimeshift")
  {
    int iNewValue = *(int*) settingValue;

    if (g_iEnableTimeshift != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'enabletimeshift' from %u to %u", __FUNCTION__, g_iEnableTimeshift, iNewValue);
      g_iEnableTimeshift = iNewValue;
      return ADDON_STATUS_OK;
    }
  }

  return ADDON_STATUS_OK;
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
  static const char *strBackendName = VuData ? VuData->GetServerName() : "unknown";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static const char *strBackendVersion = "UNKNOWN";
  return strBackendVersion;
}

static std::string strConnectionString;

const char *GetConnectionString(void)
{
  if (VuData)
    strConnectionString = StringUtils::Format("%s%s", g_strHostname.c_str(), VuData->IsConnected() ? "" : " (Not connected!)");
  else
    strConnectionString = StringUtils::Format("%s (addon error!)", g_strHostname.c_str());
  return strConnectionString.c_str();
}

const char *GetBackendHostname(void)
{
  return g_strHostname.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!VuData || !VuData->IsConnected())
    return 0;

  return VuData->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetChannels(handle, bRadio);
}

/***************************************************************************
 * Recordings
 **************************************************************************/

int GetRecordingsAmount(bool deleted)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->DeleteRecording(recording);
}

/***************************************************************************
 * Recording Streams
 **************************************************************************/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (recReader)
    SAFE_DELETE(recReader);
  recReader = VuData->OpenRecordedStream(recording);
  return recReader->Start();
}

void CloseRecordedStream(void)
{
  if (recReader)
    SAFE_DELETE(recReader);
}

int ReadRecordedStream(unsigned char *buffer, unsigned int size)
{
  if (!recReader)
    return 0;

  return recReader->ReadData(buffer, size);
}

long long SeekRecordedStream(long long position, int whence)
{
  if (!recReader)
    return 0;

  return recReader->Seek(position, whence);
}

long long LengthRecordedStream(void)
{
  if (!recReader)
    return -1;

  return recReader->Length();
}

PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  *size = 0;
  if (VuData && VuData->IsConnected())
    VuData->GetTimerTypes(types, size);
  return PVR_ERROR_NO_ERROR;
}

int GetTimersAmount(void)
{
  if (!VuData || !VuData->IsConnected())
    return 0;

  return VuData->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->DeleteTimer(timer);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->UpdateTimer(timer);
}

int GetChannelGroupsAmount(void)
{
  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetNumChannelGroups();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetChannelGroups(handle);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (group.bIsRadio)
    return PVR_ERROR_NO_ERROR;

  if (!VuData || !VuData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return VuData->GetChannelGroupMembers(handle, group);
}

PVR_ERROR GetStreamReadChunkSize(int* chunksize)
{
  if (!chunksize)
    return PVR_ERROR_INVALID_PARAMETERS;
  *chunksize = m_streamReadChunkSize;
  return PVR_ERROR_NO_ERROR;
}

/* live stream functions */
bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (!VuData || !VuData->IsConnected())
    return false;

  if (!VuData->OpenLiveStream(channel))
    return false;

  /* queue a warning if the timeshift buffer path does not exist */
  if (g_iEnableTimeshift != TIMESHIFT_OFF
      && !XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()))
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30514).c_str());

  std::string streamURL = VuData->GetLiveStreamURL(channel);
  strReader = new StreamReader(streamURL, g_iReadTimeout);
  if (g_iEnableTimeshift == TIMESHIFT_ON_PLAYBACK)// || g_iEnableTimeshift == TIMESHIFT_ON_PAUSE)
    strReader = new TimeshiftBuffer(strReader, g_strTimeshiftBufferPath, g_iReadTimeout);
  
  return strReader->Start();
}

void CloseLiveStream(void)
{
  VuData->CloseLiveStream();
  SAFE_DELETE(strReader);
}

bool IsRealTimeStream()
{
  return (strReader) ? strReader->IsRealTime() : false;
}

bool CanPauseStream(void)
{
  if (!VuData)
    return false;

  if (g_iEnableTimeshift != TIMESHIFT_OFF && strReader)
    return (strReader->IsTimeshifting() || XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()));

  return false;
}

bool CanSeekStream(void)
{
  if (!VuData)
    return false;

  return (g_iEnableTimeshift != TIMESHIFT_OFF);
}

int ReadLiveStream(unsigned char *buffer, unsigned int size)
{
  return (strReader) ? strReader->ReadData(buffer, size) : 0;
}

long long SeekLiveStream(long long position, int whence)
{
  return (strReader) ? strReader->Seek(position, whence) : -1;
}

long long LengthLiveStream(void)
{
  return (strReader) ? strReader->Length() : -1;
}

bool IsTimeshifting(void)
{
  return (strReader && strReader->IsTimeshifting());
}

PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times)
{
  if (!times)
    return PVR_ERROR_INVALID_PARAMETERS;
  if (strReader)
  {
    times->startTime = strReader->TimeStart();
    times->ptsStart  = 0;
    times->ptsBegin  = 0;
    times->ptsEnd    = (!strReader->IsTimeshifting()) ? 0
      : (strReader->TimeEnd() - strReader->TimeStart()) * DVD_TIME_BASE;
    
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_NOT_IMPLEMENTED;
}

void PauseStream(bool paused)
{
  if (!VuData)
    return;

  /* start timeshift on pause */
  if (paused && g_iEnableTimeshift == TIMESHIFT_ON_PAUSE
      && strReader && !strReader->IsTimeshifting()
      && XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()))
  {
    strReader = new TimeshiftBuffer(strReader, g_strTimeshiftBufferPath ,g_iReadTimeout);
    (void)strReader->Start();
  }
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  // the RS api doesn't provide information about signal quality (yet)

  PVR_STRCPY(signalStatus.strAdapterName, "VUPlus Media Server");
  PVR_STRCPY(signalStatus.strAdapterStatus, "OK");
  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; } 
PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) { return; }
DemuxPacket* DemuxRead(void) { return NULL; }
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
