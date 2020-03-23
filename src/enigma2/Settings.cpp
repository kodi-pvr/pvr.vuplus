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

#include "Settings.h"

#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/LocalizedString.h"

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>
#include <tinyxml.h>

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

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

  if (!XBMC->GetSetting("connectionchecktimeout", &m_connectioncCheckTimeoutSecs))
    m_connectioncCheckTimeoutSecs = DEFAULT_CONNECTION_CHECK_TIMEOUT_SECS;

  if (!XBMC->GetSetting("connectioncheckinterval", &m_connectioncCheckIntervalSecs))
    m_connectioncCheckIntervalSecs = DEFAULT_CONNECTION_CHECK_INTERVAL_SECS;

  //General
  if (!XBMC->GetSetting("setprogramid", &m_setStreamProgramId))
    m_setStreamProgramId = false;

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
    m_channelAndGroupUpdateMode = ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS;

  if (!XBMC->GetSetting("channelandgroupupdatehour", &m_channelAndGroupUpdateHour))
    m_channelAndGroupUpdateHour = DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR;

  //Channels
  if (!XBMC->GetSetting("zap", &m_zap))
    m_zap = false;

  if (!XBMC->GetSetting("usegroupspecificnumbers", &m_useGroupSpecificChannelNumbers))
    m_useGroupSpecificChannelNumbers = false;

  if (!XBMC->GetSetting("usestandardserviceref", &m_useStandardServiceReference))
    m_useStandardServiceReference = true;

  if (!XBMC->GetSetting("tvgroupmode", &m_tvChannelGroupMode))
    m_tvChannelGroupMode = ChannelGroupMode::ALL_GROUPS;

  if (!XBMC->GetSetting("numtvgroups", &m_numTVGroups))
    m_numTVGroups = DEFAULT_NUM_GROUPS;

  if (XBMC->GetSetting("onetvgroup", buffer))
    m_oneTVGroup = buffer;
  else
    m_oneTVGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("twotvgroup", buffer))
    m_twoTVGroup = buffer;
  else
    m_twoTVGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("threetvgroup", buffer))
    m_threeTVGroup = buffer;
  else
    m_threeTVGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("fourtvgroup", buffer))
    m_fourTVGroup = buffer;
  else
    m_fourTVGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("fivetvgroup", buffer))
    m_fiveTVGroup = buffer;
  else
    m_fiveTVGroup = "";
  buffer[0] = 0;

  if (m_tvChannelGroupMode == ChannelGroupMode::SOME_GROUPS)
  {
    m_customTVChannelGroupNameList.clear();

    if (!m_oneTVGroup.empty() && m_numTVGroups >= 1)
      m_customTVChannelGroupNameList.emplace_back(m_oneTVGroup);
    if (!m_twoTVGroup.empty() && m_numTVGroups >= 2)
      m_customTVChannelGroupNameList.emplace_back(m_twoTVGroup);
    if (!m_threeTVGroup.empty() && m_numTVGroups >= 3)
      m_customTVChannelGroupNameList.emplace_back(m_threeTVGroup);
    if (!m_fourTVGroup.empty() && m_numTVGroups >= 4)
      m_customTVChannelGroupNameList.emplace_back(m_fourTVGroup);
    if (!m_fiveTVGroup.empty() && m_numTVGroups >= 5)
      m_customTVChannelGroupNameList.emplace_back(m_fiveTVGroup);
  }

  if (XBMC->GetSetting("customtvgroupsfile", buffer))
    m_customTVGroupsFile = buffer;
  else
    m_customTVGroupsFile = DEFAULT_CUSTOM_TV_GROUPS_FILE;
  buffer[0] = 0;
  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  if (!XBMC->GetSetting("tvfavouritesmode", &m_tvFavouritesMode))
    m_tvFavouritesMode = FavouritesGroupMode::DISABLED;

  if (!XBMC->GetSetting("excludelastscannedtv", &m_excludeLastScannedTVGroup))
    m_excludeLastScannedTVGroup = true;

  if (!XBMC->GetSetting("radiogroupmode", &m_radioChannelGroupMode))
    m_radioChannelGroupMode = ChannelGroupMode::ALL_GROUPS;

  if (!XBMC->GetSetting("numradiogroups", &m_numRadioGroups))
    m_numRadioGroups = DEFAULT_NUM_GROUPS;

  if (XBMC->GetSetting("oneradiogroup", buffer))
    m_oneRadioGroup = buffer;
  else
    m_oneRadioGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("tworadiogroup", buffer))
    m_twoRadioGroup = buffer;
  else
    m_twoRadioGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("threeradiogroup", buffer))
    m_threeRadioGroup = buffer;
  else
    m_threeRadioGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("fourradiogroup", buffer))
    m_fourRadioGroup = buffer;
  else
    m_fourRadioGroup = "";
  buffer[0] = 0;

  if (XBMC->GetSetting("fiveradiogroup", buffer))
    m_fiveRadioGroup = buffer;
  else
    m_fiveRadioGroup = "";
  buffer[0] = 0;

  if (m_radioChannelGroupMode == ChannelGroupMode::SOME_GROUPS)
  {
    m_customRadioChannelGroupNameList.clear();

    if (!m_oneRadioGroup.empty() && m_numRadioGroups >= 1)
      m_customRadioChannelGroupNameList.emplace_back(m_oneRadioGroup);
    if (!m_twoRadioGroup.empty() && m_numRadioGroups >= 2)
      m_customRadioChannelGroupNameList.emplace_back(m_twoRadioGroup);
    if (!m_threeRadioGroup.empty() && m_numRadioGroups >= 3)
      m_customRadioChannelGroupNameList.emplace_back(m_threeRadioGroup);
    if (!m_fourRadioGroup.empty() && m_numRadioGroups >= 4)
      m_customRadioChannelGroupNameList.emplace_back(m_fourRadioGroup);
    if (!m_fiveRadioGroup.empty() && m_numRadioGroups >= 5)
      m_customRadioChannelGroupNameList.emplace_back(m_fiveRadioGroup);
  }

  if (XBMC->GetSetting("customradiogroupsfile", buffer))
    m_customRadioGroupsFile = buffer;
  else
    m_customRadioGroupsFile = DEFAULT_CUSTOM_RADIO_GROUPS_FILE;
  buffer[0] = 0;
  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  if (!XBMC->GetSetting("radiofavouritesmode", &m_radioFavouritesMode))
    m_radioFavouritesMode = FavouritesGroupMode::DISABLED;

  if (!XBMC->GetSetting("excludelastscannedradio", &m_excludeLastScannedRadioGroup))
    m_excludeLastScannedRadioGroup = true;

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
  if (!XBMC->GetSetting("storeextrarecordinginfo", &m_storeLastPlayedAndCount))
    m_storeLastPlayedAndCount = false;

  if (!XBMC->GetSetting("sharerecordinglastplayed", &m_recordingLastPlayedMode))
    m_recordingLastPlayedMode = RecordingLastPlayedMode::ACROSS_KODI_INSTANCES;

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

  if (!XBMC->GetSetting("timerlistcleanup", &m_automaticTimerlistCleanup))
    m_automaticTimerlistCleanup = false;

  if (!XBMC->GetSetting("enableautotimers", &m_enableAutoTimers))
    m_enableAutoTimers = true;

  if (!XBMC->GetSetting("limitanychannelautotimers", &m_limitAnyChannelAutoTimers))
    m_limitAnyChannelAutoTimers = true;

  if (!XBMC->GetSetting("limitanychannelautotimerstogroups", &m_limitAnyChannelAutoTimersToChannelGroups))
    m_limitAnyChannelAutoTimersToChannelGroups = true;

  //Timeshift
  if (!XBMC->GetSetting("enabletimeshift", &m_timeshift))
    m_timeshift = Timeshift::OFF;

  if (XBMC->GetSetting("timeshiftbufferpath", buffer) && !std::string(buffer).empty())
    m_timeshiftBufferPath = buffer;
  else
    m_timeshiftBufferPath = ADDON_DATA_BASE_DIR;
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

  if (!XBMC->GetSetting("nodebug", &m_noDebug))
    m_noDebug = false;

  if (!XBMC->GetSetting("debugnormal", &m_debugNormal))
    m_debugNormal = false;

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

ADDON_STATUS Settings::SetValue(const std::string& settingName, const void* settingValue)
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
  else if (settingName == "connectionchecktimeout")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_connectioncCheckTimeoutSecs, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "connectioncheckinterval")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_connectioncCheckIntervalSecs, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  //General
  else if (settingName == "setprogramid")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_setStreamProgramId, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
  else if (settingName == "usegroupspecificnumbers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useGroupSpecificChannelNumbers, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "usestandardserviceref")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useStandardServiceReference, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "tvgroupmode")
    return SetSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "numtvgroups")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_numTVGroups, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "onetvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "twotvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_twoTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "threetvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_threeTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "twotvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fourTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "fivetvgroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fiveTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "customtvgroupsfile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_customTVGroupsFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "tvfavouritesmode")
    return SetSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "excludelastscannedtv")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_excludeLastScannedTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "radiogroupmode")
    return SetSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "numradiogroups")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_numRadioGroups, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "oneradiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "tworadiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_twoRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "threeradiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_threeRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "fourradiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fourRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "fiveradiogroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fiveRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "customradiogroupsfile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_customRadioGroupsFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "radiofavouritesmode")
    return SetSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "excludelastscannedradio")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_excludeLastScannedRadioGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
  else if (settingName == "storeextrarecordinginfo")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_storeLastPlayedAndCount, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "sharerecordinglastplayed")
    return SetSetting<RecordingLastPlayedMode, ADDON_STATUS>(settingName, settingValue, m_recordingLastPlayedMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
  else if (settingName == "timerlistcleanup")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_automaticTimerlistCleanup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "enableautotimers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableAutoTimers, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "limitanychannelautotimers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_limitAnyChannelAutoTimers, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "limitanychannelautotimerstogroups")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_limitAnyChannelAutoTimersToChannelGroups, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
  else if (settingName == "nodebug")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_noDebug, ADDON_STATUS_OK, ADDON_STATUS_OK);
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

bool Settings::LoadCustomChannelGroupFile(std::string& xmlFile, std::vector<std::string>& channelGroupNameList)
{
  channelGroupNameList.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __FUNCTION__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __FUNCTION__, xmlFile.c_str());

  const std::string fileContents = FileUtils::ReadXmlFileToString(xmlFile);

  if (fileContents.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s No Content in XML file: %s", __FUNCTION__, xmlFile.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(fileContents.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("customChannelGroups").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <customChannelGroups> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("channelGroupName").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <channelGroupName> element", __FUNCTION__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("channelGroupName"))
  {
    const std::string channelGroupName = pNode->GetText();

    channelGroupNameList.emplace_back(channelGroupName);

    Logger::Log(LEVEL_TRACE, "%s Read Custom ChannelGroup Name: %s, from file: %s", __FUNCTION__, channelGroupName.c_str(), xmlFile.c_str());
  }

  return true;
}
