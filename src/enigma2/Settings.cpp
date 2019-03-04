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

  if (!XBMC->GetSetting("useopenwebifpiconpath", &m_useOpenWebIfPiconPath))
    m_useOpenWebIfPiconPath = false;

  if (XBMC->GetSetting("iconpath", buffer))
    m_iconPath = buffer;
  else
    m_iconPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("updateint", &m_updateInterval))
    m_updateInterval = DEFAULT_UPDATE_INTERVAL;

  if (!XBMC->GetSetting("updatemode", &m_updateMode))
    m_updateMode = UpdateMode::TIMERS_AND_RECORDINGS;

  if (!XBMC->GetSetting("channelandgroupupdatemode", &m_channelAndGroupUpdateMode))
    m_channelAndGroupUpdateMode = ChannelAndGroupUpdateMode::DISABLED;

  if (!XBMC->GetSetting("channelandgroupupdatehour", &m_channelAndGroupUpdateHour))
    m_channelAndGroupUpdateHour = DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR;

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

  if (!XBMC->GetSetting("excludelastscannedtv", &m_excludeLastScannedTVGroup))
    m_excludeLastScannedTVGroup = false;

  if (!XBMC->GetSetting("radiogroupmode", &m_radioChannelGroupMode))
    m_radioChannelGroupMode = ChannelGroupMode::FAVOURITES_GROUP;

  if (XBMC->GetSetting("oneradiogroup", buffer))
    m_oneRadioGroup = buffer;
  else
    m_oneRadioGroup = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("radiofavouritesmode", &m_radioFavouritesMode))
    m_radioFavouritesMode = FavouritesGroupMode::DISABLED;

  if (!XBMC->GetSetting("excludelastscannedradio", &m_excludeLastScannedRadioGroup))
    m_excludeLastScannedRadioGroup = false;

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

  if (!XBMC->GetSetting("epgdelayperchannel", &m_epgDelayPerChannel))
    m_epgDelayPerChannel = 0;

  if (!XBMC->GetSetting("skipinitialepg", &m_skipInitialEpgLoad))
    m_skipInitialEpgLoad = true;

  //Recording
  if (XBMC->GetSetting("recordingpath", buffer))
    m_recordingPath = buffer;
  else
    m_recordingPath = "";
  buffer[0] = 0;

  if (!XBMC->GetSetting("onlycurrent", &m_onlyCurrentLocation))
    m_onlyCurrentLocation = false;

  if (!XBMC->GetSetting("keepfolders", &m_keepFolders))
    m_keepFolders = false;

  if (!XBMC->GetSetting("enablerecordingedls", &m_enableRecordingEDLs))
    m_enableRecordingEDLs = false;

  if (!XBMC->GetSetting("edlpaddingstart", &m_edlStartTimePadding))
    m_edlStartTimePadding = 0;

  if (!XBMC->GetSetting("edlpaddingstop", &m_edlStopTimePadding))
    m_edlStopTimePadding = 0;

  //Timers
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

  if (!XBMC->GetSetting("powerstatemode", &m_powerstateMode))
    m_powerstateMode = PowerstateMode::DISABLED;

  if (!XBMC->GetSetting("readtimeout", &m_readTimeout))
    m_readTimeout = 0;

  if (!XBMC->GetSetting("streamreadchunksize", &m_streamReadChunkSize))
    m_streamReadChunkSize = 0;

  if (!XBMC->GetSetting("debugnormal", &m_debugNormal))
    m_traceDebug = false;

  if (!XBMC->GetSetting("tracedebug", &m_traceDebug))
    m_traceDebug = false;

  // Now that we've read all the settings construct the connection URL

  m_connectionURL.clear();
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
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_hostname, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "webport")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_portWeb, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "use_secure")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useSecureHTTP, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "user")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_username, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "pass")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_password, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "autoconfig")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_autoConfig, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "streamport")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_portStream, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "use_secure_stream")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useSecureHTTPStream, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "use_login_stream")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useLoginStream, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  //General
  else if (settingName == "onlinepicons")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_onlinePicons, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "usepiconseuformat")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_usePiconsEuFormat, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "useopenwebifpiconpath")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useOpenWebIfPiconPath, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "iconpath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_iconPath, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "updateint")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_updateInterval, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "updatemode")
    return SetSetting<UpdateMode, ADDON_STATUS>(settingName, settingValue, m_updateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "channelandgroupupdatemode")
    return SetSetting<ChannelAndGroupUpdateMode, ADDON_STATUS>(settingName, settingValue, m_channelAndGroupUpdateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "channelandgroupupdatehour")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_channelAndGroupUpdateHour, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Channels
  else if (settingName == "zap")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_zap, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tvgroupmode")
    return SetSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "onetvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "tvfavouritesmode")
    return SetSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "excludelastscannedtv")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_excludeLastScannedTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "radiogroupmode")
    return SetSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "oneradiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "radiofavouritesmode")
    return SetSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "excludelastscannedradio")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_excludeLastScannedRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //EPG
  else if (settingName == "extractepginfoenabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_extractShowInfo, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "extractepginfofile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_extractShowInfoFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "genreidmapenabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_mapGenreIds, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "genreidmapfile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_mapGenreIdsFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "rytecgenretextmapenabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_mapRytecTextGenres, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "rytecgenretextmapfile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_mapRytecTextGenresFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "logmissinggenremapping")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_logMissingGenreMappings, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgdelayperchannel")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_epgDelayPerChannel, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "skipinitialepg")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_skipInitialEpgLoad, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Recordings
  else if (settingName == "recordingpath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_recordingPath, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "onlycurrent")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_onlyCurrentLocation, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "keepfolders")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_keepFolders, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "enablerecordingedls")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableRecordingEDLs, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "edlpaddingstart")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_edlStartTimePadding, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "edlpaddingstop")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_edlStopTimePadding, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Timers
  else if (settingName == "enablegenrepeattimers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableAutoTimers, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numgenrepeattimers")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numGenRepeatTimers, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "enableautotimers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableAutoTimers, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "timerlistcleanup")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_automaticTimerlistCleanup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Timeshift
  else if (settingName == "enabletimeshift")
    return SetSetting<Timeshift, ADDON_STATUS>(settingName, settingValue, m_timeshift, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "timeshiftbufferpath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_timeshiftBufferPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Advanced
  else if (settingName == "prependoutline")
    return SetSetting<PrependOutline, ADDON_STATUS>(settingName, settingValue, m_prependOutline, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "powerstatemode")
    return SetSetting<PowerstateMode, ADDON_STATUS>(settingName, settingValue, m_powerstateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "readtimeout")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_readTimeout, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "streamreadchunksize")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_streamReadChunkSize, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "debugnormal")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_debugNormal, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tracedebug")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_traceDebug, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Backend
  else if (settingName == "globalstartpaddingstb")
  {
    if (SetSetting<int, bool>(settingName, settingValue, m_globalStartPaddingStb, true, false))
      m_admin->SendGlobalRecordingStartMarginSetting(m_globalStartPaddingStb);
  }
  else if (settingName == "globalendpaddingstb")
  {
    if (SetSetting<int, bool>(settingName, settingValue, m_globalEndPaddingStb, true, false))
      m_admin->SendGlobalRecordingEndMarginSetting(m_globalEndPaddingStb);
  }

  return ADDON_STATUS_OK;
}

bool Settings::IsTimeshiftBufferPathValid() const
{
  return XBMC->DirectoryExists(m_timeshiftBufferPath.c_str());
}