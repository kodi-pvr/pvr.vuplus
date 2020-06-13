/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include "utilities/FileUtils.h"
#include "utilities/StringUtils.h"
#include "utilities/XMLUtils.h"

#include <tinyxml.h>

using namespace enigma2;
using namespace enigma2::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

  //Connection
  m_hostname = kodi::GetSettingString("host", DEFAULT_HOST);
  m_portWeb = kodi::GetSettingInt("webport", DEFAULT_WEB_PORT);
  m_useSecureHTTP = kodi::GetSettingBoolean("use_secure", false);
  m_username = kodi::GetSettingString("user");
  m_password = kodi::GetSettingString("pass");
  m_autoConfig = kodi::GetSettingBoolean("autoconfig", false);
  m_portStream = kodi::GetSettingInt("streamport", DEFAULT_STREAM_PORT);
  m_useSecureHTTPStream = kodi::GetSettingBoolean("use_secure_stream", false);
  m_useLoginStream = kodi::GetSettingBoolean("use_login_stream", false);
  m_connectioncCheckTimeoutSecs = kodi::GetSettingInt("connectionchecktimeout", DEFAULT_CONNECTION_CHECK_TIMEOUT_SECS);
  m_connectioncCheckIntervalSecs = kodi::GetSettingInt("connectioncheckinterval", DEFAULT_CONNECTION_CHECK_INTERVAL_SECS);

  //General
  m_setStreamProgramId = kodi::GetSettingBoolean("setprogramid", false);
  m_onlinePicons = kodi::GetSettingBoolean("onlinepicons", true);
  m_usePiconsEuFormat = kodi::GetSettingBoolean("usepiconseuformat", false);
  m_useOpenWebIfPiconPath = kodi::GetSettingBoolean("useopenwebifpiconpath", false);
  m_iconPath = kodi::GetSettingString("iconpath");
  m_updateInterval = kodi::GetSettingInt("updateint", DEFAULT_UPDATE_INTERVAL);
  m_updateMode = kodi::GetSettingEnum<UpdateMode>("updatemode", UpdateMode::TIMERS_AND_RECORDINGS);
  m_channelAndGroupUpdateMode = kodi::GetSettingEnum<ChannelAndGroupUpdateMode>("channelandgroupupdatemode", ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS);
  m_channelAndGroupUpdateHour = kodi::GetSettingInt("channelandgroupupdatehour", DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR);

  //Channels
  m_zap = kodi::GetSettingBoolean("zap", false);
  m_useGroupSpecificChannelNumbers = kodi::GetSettingBoolean("usegroupspecificnumbers", false);
  m_useStandardServiceReference = kodi::GetSettingBoolean("usestandardserviceref", true);
  m_tvChannelGroupMode = kodi::GetSettingEnum<ChannelGroupMode>("tvgroupmode", ChannelGroupMode::ALL_GROUPS);
  m_numTVGroups = kodi::GetSettingInt("numtvgroups", DEFAULT_NUM_GROUPS);
  m_oneTVGroup = kodi::GetSettingString("onetvgroup");
  m_twoTVGroup = kodi::GetSettingString("twotvgroup");
  m_threeTVGroup = kodi::GetSettingString("threetvgroup");
  m_fourTVGroup = kodi::GetSettingString("fourtvgroup");
  m_fiveTVGroup = kodi::GetSettingString("fivetvgroup");

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

  m_customTVGroupsFile = kodi::GetSettingString("customtvgroupsfile", DEFAULT_CUSTOM_TV_GROUPS_FILE);

  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_tvFavouritesMode = kodi::GetSettingEnum<FavouritesGroupMode>("tvfavouritesmode", FavouritesGroupMode::DISABLED);
  m_excludeLastScannedTVGroup = kodi::GetSettingBoolean("excludelastscannedtv", true);
  m_radioChannelGroupMode = kodi::GetSettingEnum<ChannelGroupMode>("radiogroupmode", ChannelGroupMode::ALL_GROUPS);
  m_numRadioGroups = kodi::GetSettingInt("numradiogroups", DEFAULT_NUM_GROUPS);
  m_oneRadioGroup = kodi::GetSettingString("oneradiogroup");
  m_twoRadioGroup = kodi::GetSettingString("tworadiogroup");
  m_threeRadioGroup = kodi::GetSettingString("threeradiogroup");
  m_fourRadioGroup = kodi::GetSettingString("fourradiogroup");
  m_fiveRadioGroup = kodi::GetSettingString("fiveradiogroup");

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

  m_customRadioGroupsFile = kodi::GetSettingString("customradiogroupsfile", DEFAULT_CUSTOM_RADIO_GROUPS_FILE);

  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  m_radioFavouritesMode = kodi::GetSettingEnum<FavouritesGroupMode>("radiofavouritesmode", FavouritesGroupMode::DISABLED);
  m_excludeLastScannedRadioGroup = kodi::GetSettingBoolean("excludelastscannedradio", true);

  //EPG
  m_extractShowInfo = kodi::GetSettingBoolean("extractshowinfoenabled", false);
  m_extractShowInfoFile = kodi::GetSettingString("extractshowinfofile", DEFAULT_SHOW_INFO_FILE);
  m_mapGenreIds = kodi::GetSettingBoolean("genreidmapenabled", false);
  m_mapGenreIdsFile = kodi::GetSettingString("genreidmapfile", DEFAULT_GENRE_ID_MAP_FILE);
  m_mapRytecTextGenres = kodi::GetSettingBoolean("rytecgenretextmapenabled", false);
  m_mapRytecTextGenresFile = kodi::GetSettingString("rytecgenretextmapfile", DEFAULT_GENRE_ID_MAP_FILE);
  m_logMissingGenreMappings = kodi::GetSettingBoolean("logmissinggenremapping", false);
  m_epgDelayPerChannel = kodi::GetSettingInt("epgdelayperchannel", 0);
  m_skipInitialEpgLoad = kodi::GetSettingBoolean("skipinitialepg", true);

  //Recording
  m_storeLastPlayedAndCount = kodi::GetSettingBoolean("storeextrarecordinginfo", false);
  m_recordingLastPlayedMode = kodi::GetSettingEnum<RecordingLastPlayedMode>("sharerecordinglastplayed", RecordingLastPlayedMode::ACROSS_KODI_INSTANCES);
  m_recordingPath = kodi::GetSettingString("recordingpath");
  m_onlyCurrentLocation = kodi::GetSettingBoolean("onlycurrent", false);
  m_keepFolders = kodi::GetSettingBoolean("keepfolders", false);
  m_enableRecordingEDLs = kodi::GetSettingBoolean("enablerecordingedls", false);
  m_edlStartTimePadding = kodi::GetSettingInt("edlpaddingstart", 0);
  m_edlStopTimePadding = kodi::GetSettingInt("edlpaddingstop", 0);

  //Timers
  m_enableGenRepeatTimers = kodi::GetSettingBoolean("enablegenrepeattimers", true);
  m_numGenRepeatTimers = kodi::GetSettingInt("numgenrepeattimers", DEFAULT_NUM_GEN_REPEAT_TIMERS);
  m_automaticTimerlistCleanup = kodi::GetSettingBoolean("timerlistcleanup", false);
  m_enableAutoTimers = kodi::GetSettingBoolean("enableautotimers", true);
  m_limitAnyChannelAutoTimers = kodi::GetSettingBoolean("limitanychannelautotimers", true);
  m_limitAnyChannelAutoTimersToChannelGroups = kodi::GetSettingBoolean("limitanychannelautotimerstogroups", true);

  //Timeshift
  m_timeshift = kodi::GetSettingEnum<Timeshift>("enabletimeshift", Timeshift::OFF);
  m_timeshiftBufferPath = kodi::GetSettingString("timeshiftbufferpath", ADDON_DATA_BASE_DIR);

  //Backend
  m_wakeOnLanMac = kodi::GetSettingString("wakeonlanmac");
  m_powerstateMode = kodi::GetSettingEnum<PowerstateMode>("powerstatemode", PowerstateMode::DISABLED);

  //Advanced
  m_prependOutline = kodi::GetSettingEnum<PrependOutline>("prependoutline", PrependOutline::IN_EPG);
  m_readTimeout = kodi::GetSettingInt("readtimeout", 0);
  m_streamReadChunkSize = kodi::GetSettingInt("streamreadchunksize", 0);
  m_noDebug = kodi::GetSettingBoolean("nodebug", false);
  m_debugNormal = kodi::GetSettingBoolean("debugnormal", false);
  m_traceDebug = kodi::GetSettingBoolean("tracedebug", false);

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

ADDON_STATUS Settings::SetValue(const std::string& settingName, const kodi::CSettingValue& settingValue)
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
    return SetEnumSetting<UpdateMode, ADDON_STATUS>(settingName, settingValue, m_updateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "channelandgroupupdatemode")
    return SetEnumSetting<ChannelAndGroupUpdateMode, ADDON_STATUS>(settingName, settingValue, m_channelAndGroupUpdateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
    return SetEnumSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
    return SetEnumSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "excludelastscannedtv")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_excludeLastScannedTVGroup, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "radiogroupmode")
    return SetEnumSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioChannelGroupMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
    return SetEnumSetting<FavouritesGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioFavouritesMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
    return SetEnumSetting<RecordingLastPlayedMode, ADDON_STATUS>(settingName, settingValue, m_recordingLastPlayedMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
    return SetEnumSetting<Timeshift, ADDON_STATUS>(settingName, settingValue, m_timeshift, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "timeshiftbufferpath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_timeshiftBufferPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Backend
  else if (settingName == "wakeonlanmac")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_wakeOnLanMac, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
  //Advanced
  else if (settingName == "prependoutline")
    return SetEnumSetting<PrependOutline, ADDON_STATUS>(settingName, settingValue, m_prependOutline, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "powerstatemode")
    return SetEnumSetting<PowerstateMode, ADDON_STATUS>(settingName, settingValue, m_powerstateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
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

  return ADDON_STATUS_OK;
}

bool Settings::IsTimeshiftBufferPathValid() const
{
  return kodi::vfs::DirectoryExists(m_timeshiftBufferPath);
}

bool Settings::LoadCustomChannelGroupFile(std::string& xmlFile, std::vector<std::string>& channelGroupNameList)
{
  channelGroupNameList.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __func__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __func__, xmlFile.c_str());

  const std::string fileContents = FileUtils::ReadXmlFileToString(xmlFile);

  if (fileContents.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s No Content in XML file: %s", __func__, xmlFile.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(fileContents.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("customChannelGroups").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <customChannelGroups> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("channelGroupName").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <channelGroupName> element", __func__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("channelGroupName"))
  {
    const std::string channelGroupName = pNode->GetText();

    channelGroupNameList.emplace_back(channelGroupName);

    Logger::Log(LEVEL_TRACE, "%s Read Custom ChannelGroup Name: %s, from file: %s", __func__, channelGroupName.c_str(), xmlFile.c_str());
  }

  return true;
}
