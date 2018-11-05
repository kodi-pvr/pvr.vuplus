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
    m_hostname = buffer;
  else
    m_hostname = DEFAULT_HOST;
  buffer[0] = 0;

  if (!XBMC->GetSetting("webport", &m_portWeb))
    m_portWeb = DEFAULT_WEB_PORT;
  
  if (!XBMC->GetSetting("use_secure", &m_useSecureHTTP))
    m_useSecureHTTP = false;
  
  if (XBMC->GetSetting("user", buffer))
    m_username = buffer;
  else
    m_username = "";
  buffer[0] = 0;
  
  if (XBMC->GetSetting("pass", buffer))
    m_password = buffer;
  else
    m_password = "";
  buffer[0] = 0;
  
  if (!XBMC->GetSetting("autoconfig", &m_autoConfig))
    m_autoConfig = false;
  
  if (!XBMC->GetSetting("streamport", &m_portStream))
    m_portStream = DEFAULT_STREAM_PORT;

  if (!XBMC->GetSetting("use_secure_stream", &m_useSecureHTTPStream))
    m_useSecureHTTPStream = false;

  if (!XBMC->GetSetting("use_login_stream", &m_useLoginStream))
    m_useLoginStream = false;

  //General
  if (!XBMC->GetSetting("onlinepicons", &m_onlinePicons))
    m_onlinePicons = true;
  
  if (!XBMC->GetSetting("usepiconseuformat", &m_usePiconsEuFormat))
    m_usePiconsEuFormat = false;

  if (XBMC->GetSetting("iconpath", buffer))
    m_iconPath = buffer;
  else
    m_iconPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("updateint", &m_updateInterval))
    m_updateInterval = DEFAULT_UPDATE_INTERVAL;

  //Channels
  if (!XBMC->GetSetting("zap", &m_zap))
    m_zap = false;

  if (!XBMC->GetSetting("tvgroupmode", &m_tvChannelGroupMode))
    m_tvChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
  
  if (XBMC->GetSetting("onetvgroup", buffer))
    m_oneTVGroup = buffer;
  else
    m_oneTVGroup = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("tvfavouritesmode", &m_tvFavouritesMode))
    m_tvFavouritesMode = FavouritesGroupMode::DISABLED;

  if (!XBMC->GetSetting("radiogroupmode", &m_radioChannelGroupMode))
    m_radioChannelGroupMode = ChannelGroupMode::FAVOURITES_GROUP;
  
  if (XBMC->GetSetting("oneradiogroup", buffer))
    m_oneRadioGroup = buffer;
  else
    m_oneRadioGroup = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("radiofavouritesmode", &m_radioFavouritesMode))
    m_radioFavouritesMode = FavouritesGroupMode::DISABLED;

  //EPG
  if (!XBMC->GetSetting("extractshowinfoenabled", &m_extractShowInfo))
    m_extractShowInfo = false;

  if (XBMC->GetSetting("extractshowinfofile", buffer))
    m_extractShowInfoFile = buffer;
  else
    m_extractShowInfoFile = DEFAULT_SHOW_INFO_FILE;
  buffer[0] = 0;

  if (!XBMC->GetSetting("genreidmapenabled", &m_mapGenreIds))
    m_mapGenreIds = false;

  if (XBMC->GetSetting("genreidmapfile", buffer))
    m_mapGenreIdsFile = buffer;
  else
    m_mapGenreIdsFile = DEFAULT_GENRE_ID_MAP_FILE;
  buffer[0] = 0;

  if (!XBMC->GetSetting("rytecgenretextmapenabled", &m_mapRytecTextGenres))
    m_mapRytecTextGenres = false;

  if (XBMC->GetSetting("rytecgenretextmapfile", buffer))
    m_mapRytecTextGenresFile = buffer;
  else
    m_mapRytecTextGenresFile = DEFAULT_GENRE_ID_MAP_FILE;
  buffer[0] = 0;

  if (!XBMC->GetSetting("logmissinggenremapping", &m_logMissingGenreMappings))
    m_logMissingGenreMappings = false;

  //Recording and Timers
  if (XBMC->GetSetting("recordingpath", buffer))
    m_recordingPath = buffer;
  else
    m_recordingPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("onlycurrent", &m_onlyCurrentLocation))
    m_onlyCurrentLocation = false;
  
  if (!XBMC->GetSetting("keepfolders", &m_keepFolders))
    m_keepFolders = false;
  
  if (!XBMC->GetSetting("enablegenrepeattimers", &m_enableGenRepeatTimers))
    m_enableGenRepeatTimers = true;

  if (!XBMC->GetSetting("numgenrepeattimers", &m_numGenRepeatTimers))
    m_numGenRepeatTimers = DEFAULT_NUM_GEN_REPEAT_TIMERS;

  if (!XBMC->GetSetting("enableautotimers", &m_enableAutoTimers))
    m_enableAutoTimers = true;

  if (!XBMC->GetSetting("timerlistcleanup", &m_automaticTimerlistCleanup))
    m_automaticTimerlistCleanup = false;

  //Timeshift
  if (!XBMC->GetSetting("enabletimeshift", &m_timeshift))
    m_timeshift = Timeshift::OFF;

  if (XBMC->GetSetting("timeshiftbufferpath", buffer) && !std::string(buffer).empty())
    m_timeshiftBufferPath = buffer;
  else
    m_timeshiftBufferPath = DEFAULT_TSBUFFERPATH;
  buffer[0] = 0;

  //Advanced
  if (!XBMC->GetSetting("prependoutline", &m_prependOutline))
    m_prependOutline = PrependOutline::IN_EPG;

  if (!XBMC->GetSetting("setpowerstate", &m_setPowerstate))
    m_setPowerstate = false;
  
  if (!XBMC->GetSetting("readtimeout", &m_readTimeout))
    m_readTimeout = 0;

  if (!XBMC->GetSetting("streamreadchunksize", &m_streamReadChunkSize))
    m_streamReadChunkSize = 0;

  // Now that we've read all the settings construct the connection URL
  
  // simply add user@pass in front of the URL if username/password is set
  if ((m_username.length() > 0) && (m_password.length() > 0))
    m_connectionURL = StringUtils::Format("%s:%s@", m_username.c_str(), m_password.c_str());
  if (!m_useSecureHTTP)
    m_connectionURL = StringUtils::Format("http://%s%s:%u/", m_connectionURL.c_str(), m_hostname.c_str(), m_portWeb);
  else
    m_connectionURL = StringUtils::Format("https://%s%s:%u/", m_connectionURL.c_str(), m_hostname.c_str(), m_portWeb);  
}

ADDON_STATUS Settings::SetValue(const std::string &settingName, const void *settingValue)
{
  //Connection
  if (settingName == "host")
    return SetStringSetting(settingName, settingValue, m_hostname, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "webport")
    return SetSetting<int>(settingName, settingValue, m_portWeb, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_secure")
    return SetSetting<bool>(settingName, settingValue, m_useSecureHTTP, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "user")
    return SetStringSetting(settingName, settingValue, m_username, ADDON_STATUS_OK);
  else if (settingName == "pass")
    return SetStringSetting(settingName, settingValue, m_password, ADDON_STATUS_OK);
  else if (settingName == "autoconfig")
    return SetSetting<bool>(settingName, settingValue, m_autoConfig, ADDON_STATUS_OK);
  else if (settingName == "streamport")
    return SetSetting<int>(settingName, settingValue, m_portStream, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_secure_stream")
    return SetSetting<bool>(settingName, settingValue, m_useSecureHTTPStream, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "use_login_stream")
    return SetSetting<bool>(settingName, settingValue, m_useLoginStream, ADDON_STATUS_NEED_RESTART);
  //General
  else if (settingName == "onlinepicons")
    return SetSetting<bool>(settingName, settingValue, m_onlinePicons, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "usepiconseuformat")
    return SetSetting<bool>(settingName, settingValue, m_usePiconsEuFormat, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "iconpath")
    return SetStringSetting(settingName, settingValue, m_iconPath, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "updateint")
    return SetSetting<int>(settingName, settingValue, m_updateInterval, ADDON_STATUS_OK);
  //Channels
  else if (settingName == "zap")
    return SetSetting<bool>(settingName, settingValue, m_zap, ADDON_STATUS_OK);
  else if (settingName == "tvgroupmode")
    return SetSetting<ChannelGroupMode>(settingName, settingValue, m_tvChannelGroupMode, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "onetvgroup")
    return SetStringSetting(settingName, settingValue, m_oneTVGroup, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "tvfavouritesmode")
    return SetSetting<FavouritesGroupMode>(settingName, settingValue, m_tvFavouritesMode, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "radiogroupmode")
    return SetSetting<ChannelGroupMode>(settingName, settingValue, m_radioChannelGroupMode, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "oneradiogroup")
    return SetStringSetting(settingName, settingValue, m_oneRadioGroup, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "radiofavouritesmode")
    return SetSetting<FavouritesGroupMode>(settingName, settingValue, m_radioFavouritesMode, ADDON_STATUS_NEED_RESTART);
  //EPG
  else if (settingName == "extractepginfoenabled")
    return SetSetting<bool>(settingName, settingValue, m_extractShowInfo, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "extractepginfofile")
    return SetStringSetting(settingName, settingValue, m_extractShowInfoFile, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "genreidmapenabled")
    return SetSetting<bool>(settingName, settingValue, m_mapGenreIds, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "genreidmapfile")
    return SetStringSetting(settingName, settingValue, m_mapGenreIdsFile, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "rytecgenretextmapenabled")
    return SetSetting<bool>(settingName, settingValue, m_mapRytecTextGenres, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "rytecgenretextmapfile")
    return SetStringSetting(settingName, settingValue, m_mapRytecTextGenresFile, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "logmissinggenremapping")
    return SetSetting<bool>(settingName, settingValue, m_logMissingGenreMappings, ADDON_STATUS_OK);
  //Recordings and Timers
  else if (settingName == "recordingpath")
    return SetStringSetting(settingName, settingValue, m_recordingPath, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "onlycurrent")
    return SetSetting<bool>(settingName, settingValue, m_onlyCurrentLocation, ADDON_STATUS_OK);
  else if (settingName == "keepfolders")
    return SetSetting<bool>(settingName, settingValue, m_keepFolders, ADDON_STATUS_OK);
  else if (settingName == "enablegenrepeattimers")
    return SetSetting<bool>(settingName, settingValue, m_enableAutoTimers, ADDON_STATUS_OK);
  else if (settingName == "numgenrepeattimers")
    return SetSetting<int>(settingName, settingValue, m_numGenRepeatTimers, ADDON_STATUS_OK);
  else if (settingName == "enableautotimers")
    return SetSetting<bool>(settingName, settingValue, m_enableAutoTimers, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "timerlistcleanup")
    return SetSetting<bool>(settingName, settingValue, m_automaticTimerlistCleanup, ADDON_STATUS_OK);
  //Timeshift
  else if (settingName == "enabletimeshift")
    return SetSetting<Timeshift>(settingName, settingValue, m_timeshift, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "timeshiftbufferpath")
    return SetStringSetting(settingName, settingValue, m_timeshiftBufferPath, ADDON_STATUS_OK);
  //Advanced
  else if (settingName == "prependoutline")
    return SetSetting<PrependOutline>(settingName, settingValue, m_prependOutline, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "setpowerstate")
    return SetSetting<bool>(settingName, settingValue, m_setPowerstate, ADDON_STATUS_OK);
  else if (settingName == "readtimeout")
    return SetSetting<int>(settingName, settingValue, m_readTimeout, ADDON_STATUS_NEED_RESTART);
  else if (settingName == "streamreadchunksize")
    return SetSetting<int>(settingName, settingValue, m_streamReadChunkSize, ADDON_STATUS_OK);

  return ADDON_STATUS_OK;
}

ADDON_STATUS Settings::SetStringSetting(const std::string &settingName, const void* settingValue, std::string &currentValue, ADDON_STATUS returnValueIfChanged)
{
  const std::string strSettingValue = static_cast<const char*>(settingValue);

  if (strSettingValue != currentValue)
  {
    Logger::Log(LEVEL_NOTICE, "%s - Changed Setting '%s' from %s to %s", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
    currentValue = strSettingValue;
    return returnValueIfChanged;
  }

  return ADDON_STATUS_OK;
}

bool Settings::IsTimeshiftBufferPathValid() const
{
  return XBMC->DirectoryExists(m_timeshiftBufferPath.c_str());
}
