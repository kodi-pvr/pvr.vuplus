#include "Settings.h"

#include "../client.h"
#include "utilities/LocalizedString.h"

#include "p8-platform/util/StringUtils.h"

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon()
{
  char buffer[1024];
  buffer[0] = 0;

  //Connection
  if (XBMC->GetSetting("host", buffer))
    m_strHostname = buffer;
  else
    m_strHostname = DEFAULT_HOST;
  buffer[0] = 0;

  if (!XBMC->GetSetting("webport", &m_iPortWeb))
    m_iPortWeb = DEFAULT_WEB_PORT;
  
  if (!XBMC->GetSetting("use_secure", &m_bUseSecureHTTP))
    m_bUseSecureHTTP = false;
  
  if (XBMC->GetSetting("user", buffer))
    m_strUsername = buffer;
  else
    m_strUsername = "";
  buffer[0] = 0;
  
  if (XBMC->GetSetting("pass", buffer))
    m_strPassword = buffer;
  else
    m_strPassword = "";
  buffer[0] = 0;
  
  if (!XBMC->GetSetting("autoconfig", &m_bAutoConfig))
    m_bAutoConfig = false;
  
  if (!XBMC->GetSetting("streamport", &m_iPortStream))
    m_iPortStream = DEFAULT_STREAM_PORT;

  if (!XBMC->GetSetting("use_secure_stream", &m_bUseSecureHTTPStream))
    m_bUseSecureHTTPStream = false;

  if (!XBMC->GetSetting("use_login_stream", &m_bUseLoginStream))
    m_bUseLoginStream = false;

  //General
  if (!XBMC->GetSetting("onlinepicons", &m_bOnlinePicons))
    m_bOnlinePicons = true;
  
  if (!XBMC->GetSetting("usepiconseuformat", &m_bUsePiconsEuFormat))
    m_bUsePiconsEuFormat = false;

  if (XBMC->GetSetting("iconpath", buffer))
    m_strIconPath = buffer;
  else
    m_strIconPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("updateint", &m_iUpdateInterval))
    m_iUpdateInterval = DEFAULT_UPDATE_INTERVAL;

  //Channels & EPG
  if (!XBMC->GetSetting("onlyonegroup", &m_bOnlyOneGroup))
    m_bOnlyOneGroup = false;
  
  if (XBMC->GetSetting("onegroup", buffer))
    m_strOneGroup = buffer;
  else
    m_strOneGroup = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("zap", &m_bZap))
    m_bZap = false;

  if (!XBMC->GetSetting("extracteventinfo", &m_bExtractExtraEpgInfo))
    m_bExtractExtraEpgInfo = false;

  if (!XBMC->GetSetting("logmissinggenremapping", &m_bLogMissingGenreMappings))
    m_bLogMissingGenreMappings = false;

  //Recording and Timers
  if (XBMC->GetSetting("recordingpath", buffer))
    m_strRecordingPath = buffer;
  else
    m_strRecordingPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("onlycurrent", &m_bOnlyCurrentLocation))
    m_bOnlyCurrentLocation = false;
  
  if (!XBMC->GetSetting("keepfolders", &m_bKeepFolders))
    m_bKeepFolders = false;
  
  if (!XBMC->GetSetting("enablegenrepeattimers", &m_bEnableGenRepeatTimers))
    m_bEnableGenRepeatTimers = true;

  if (!XBMC->GetSetting("numgenrepeattimers", &m_iNumGenRepeatTimers))
    m_iNumGenRepeatTimers = DEFAULT_NUM_GEN_REPEAT_TIMERS;

  if (!XBMC->GetSetting("enableautotimers", &m_bEnableAutoTimers))
    m_bEnableAutoTimers = true;

  if (!XBMC->GetSetting("timerlistcleanup", &m_bAutomaticTimerlistCleanup))
    m_bAutomaticTimerlistCleanup = false;

  //Timeshift
  if (!XBMC->GetSetting("enabletimeshift", &m_timeshift))
    m_timeshift = Timeshift::OFF;

  if (XBMC->GetSetting("timeshiftbufferpath", buffer) && !std::string(buffer).empty())
    m_strTimeshiftBufferPath = buffer;
  else
    m_strTimeshiftBufferPath = DEFAULT_TSBUFFERPATH;
  buffer[0] = 0;

  //Advanced
  if (!XBMC->GetSetting("prependoutline", &m_prependOutline))
    m_prependOutline = PrependOutline::IN_EPG;

  if (!XBMC->GetSetting("setpowerstate", &m_bSetPowerstate))
    m_bSetPowerstate = false;
  
  if (!XBMC->GetSetting("readtimeout", &m_iReadTimeout))
    m_iReadTimeout = 0;

  if (!XBMC->GetSetting("streamreadchunksize", &m_streamReadChunkSize))
    m_streamReadChunkSize = 0;

  // Now that we've read all the settings construct the connection URL
  
  // simply add user@pass in front of the URL if username/password is set
  if ((m_strUsername.length() > 0) && (m_strPassword.length() > 0))
    m_connectionURL = StringUtils::Format("%s:%s@", m_strUsername.c_str(), m_strPassword.c_str());
  if (!m_bUseSecureHTTP)
    m_connectionURL = StringUtils::Format("http://%s%s:%u/", m_connectionURL.c_str(), m_strHostname.c_str(), m_iPortWeb);
  else
    m_connectionURL = StringUtils::Format("https://%s%s:%u/", m_connectionURL.c_str(), m_strHostname.c_str(), m_iPortWeb);  
}

ADDON_STATUS Settings::SetValue(const std::string &settingName, const void *settingValue)
{
  //Connection
  if (settingName == "host")
    return SetStringSetting(settingName, settingValue, m_strHostname, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "webport")
    return SetSetting<int>(settingName, settingValue, m_iPortWeb, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_secure")
    return SetSetting<bool>(settingName, settingValue, m_bUseSecureHTTP, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "user")
    return SetStringSetting(settingName, settingValue, m_strUsername, ADDON_STATUS_OK);
  else if (settingName == "pass")
    return SetStringSetting(settingName, settingValue, m_strPassword, ADDON_STATUS_OK);
  else if (settingName == "autoconfig")
    return SetSetting<bool>(settingName, settingValue, m_bAutoConfig, ADDON_STATUS_OK);
  else if (settingName == "streamport")
    return SetSetting<int>(settingName, settingValue, m_iPortStream, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_secure_stream")
    return SetSetting<bool>(settingName, settingValue, m_bUseSecureHTTPStream, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_login_stream")
    return SetSetting<bool>(settingName, settingValue, m_bUseLoginStream, ADDON_STATUS_NEED_RESTART);
  //General
  else if (settingName == "onlinepicons")
    return SetSetting<bool>(settingName, settingValue, m_bOnlinePicons, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "usepiconseuformat")
    return SetSetting<bool>(settingName, settingValue, m_bUsePiconsEuFormat, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "iconpath")
    return SetStringSetting(settingName, settingValue, m_strIconPath, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "updateint")
    return SetSetting<int>(settingName, settingValue, m_iUpdateInterval, ADDON_STATUS_OK);
  //Channels & EPG
  else if (settingName == "onlyonegroup")
    return SetSetting<bool>(settingName, settingValue, m_bOnlyOneGroup, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "onegroup")
    return SetStringSetting(settingName, settingValue, m_strOneGroup, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "zap")
    return SetSetting<bool>(settingName, settingValue, m_bZap, ADDON_STATUS_OK);
  else if (settingName == "extracteventinfo")
    return SetSetting<bool>(settingName, settingValue, m_bExtractExtraEpgInfo, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "logmissinggenremapping")
    return SetSetting<bool>(settingName, settingValue, m_bLogMissingGenreMappings, ADDON_STATUS_OK);
  //Recordings and Timers
  else if (settingName == "recordingpath")
    return SetStringSetting(settingName, settingValue, m_strRecordingPath, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "onlycurrent")
    return SetSetting<bool>(settingName, settingValue, m_bOnlyCurrentLocation, ADDON_STATUS_OK);
  else if (settingName == "keepfolders")
    return SetSetting<bool>(settingName, settingValue, m_bKeepFolders, ADDON_STATUS_OK);
  else if (settingName == "enablegenrepeattimers")
    return SetSetting<bool>(settingName, settingValue, m_bEnableAutoTimers, ADDON_STATUS_OK);
  else if (settingName == "numgenrepeattimers")
    return SetSetting<int>(settingName, settingValue, m_iNumGenRepeatTimers, ADDON_STATUS_OK);
  else if (settingName == "enableautotimers")
    return SetSetting<bool>(settingName, settingValue, m_bEnableAutoTimers, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "timerlistcleanup")
    return SetSetting<bool>(settingName, settingValue, m_bAutomaticTimerlistCleanup, ADDON_STATUS_OK);
  //Timeshift
  else if (settingName == "enabletimeshift")
    return SetSetting<Timeshift>(settingName, settingValue, m_timeshift, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "timeshiftbufferpath")
    return SetStringSetting(settingName, settingValue, m_strTimeshiftBufferPath, ADDON_STATUS_OK);
  //Advanced
  else if (settingName == "prependoutline")
    return SetSetting<PrependOutline>(settingName, settingValue, m_prependOutline, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "setpowerstate")
    return SetSetting<bool>(settingName, settingValue, m_bSetPowerstate, ADDON_STATUS_OK);
  else if (settingName == "readtimeout")
    return SetSetting<int>(settingName, settingValue, m_iReadTimeout, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "streamreadchunksize")
    return SetSetting<int>(settingName, settingValue, m_streamReadChunkSize, ADDON_STATUS_OK);

  return ADDON_STATUS_OK;
}

ADDON_STATUS Settings::SetStringSetting(const std::string &settingName, const void* settingValue, std::string &currentValue, ADDON_STATUS returnValueIfChanged)
{
  const std::string strSettingValue = static_cast<const char*>(settingValue);

  if (strSettingValue != currentValue)
  {
    Logger::Log(LEVEL_INFO, "%s - Changed Setting '%s' from %s to %s", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
    currentValue = strSettingValue;
    return returnValueIfChanged;
  }

  return ADDON_STATUS_OK;
}

bool Settings::IsTimeshiftBufferPathValid() const
{
  return XBMC->DirectoryExists(m_strTimeshiftBufferPath.c_str());
}
