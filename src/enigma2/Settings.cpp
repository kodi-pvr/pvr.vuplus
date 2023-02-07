/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include "utilities/FileUtils.h"
#include "utilities/WebUtils.h"
#include "utilities/XMLUtils.h"

#include <kodi/tools/StringUtils.h>
#include <tinyxml.h>

using namespace enigma2;
using namespace enigma2::utilities;
using namespace kodi::tools;

/***************************************************************************
 * PVR settings
 **************************************************************************/
Settings::Settings()
{
  ReadSettings();
}

void Settings::ReadSettings()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

  //Connection
  m_hostname = kodi::addon::GetSettingString("host", DEFAULT_HOST);
  m_portWeb = kodi::addon::GetSettingInt("webport", DEFAULT_WEB_PORT);
  m_useSecureHTTP = kodi::addon::GetSettingBoolean("use_secure", false);
  m_username = kodi::addon::GetSettingString("user");
  m_password = kodi::addon::GetSettingString("pass");
  m_autoConfig = kodi::addon::GetSettingBoolean("autoconfig", false);
  m_portStream = kodi::addon::GetSettingInt("streamport", DEFAULT_STREAM_PORT);
  m_useSecureHTTPStream = kodi::addon::GetSettingBoolean("use_secure_stream", false);
  m_useLoginStream = kodi::addon::GetSettingBoolean("use_login_stream", false);
  m_connectioncCheckTimeoutSecs = kodi::addon::GetSettingInt("connectionchecktimeout", DEFAULT_CONNECTION_CHECK_TIMEOUT_SECS);
  m_connectioncCheckIntervalSecs = kodi::addon::GetSettingInt("connectioncheckinterval", DEFAULT_CONNECTION_CHECK_INTERVAL_SECS);

  //General
  m_setStreamProgramId = kodi::addon::GetSettingBoolean("setprogramid", false);
  m_onlinePicons = kodi::addon::GetSettingBoolean("onlinepicons", true);
  m_usePiconsEuFormat = kodi::addon::GetSettingBoolean("usepiconseuformat", false);
  m_useOpenWebIfPiconPath = kodi::addon::GetSettingBoolean("useopenwebifpiconpath", false);
  m_iconPath = kodi::addon::GetSettingString("iconpath");
  m_updateInterval = kodi::addon::GetSettingInt("updateint", DEFAULT_UPDATE_INTERVAL);
  m_updateMode = kodi::addon::GetSettingEnum<UpdateMode>("updatemode", UpdateMode::TIMERS_AND_RECORDINGS);
  m_channelAndGroupUpdateMode = kodi::addon::GetSettingEnum<ChannelAndGroupUpdateMode>("channelandgroupupdatemode", ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS);
  m_channelAndGroupUpdateHour = kodi::addon::GetSettingInt("channelandgroupupdatehour", DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR);

  //Channels
  m_zap = kodi::addon::GetSettingBoolean("zap", false);
  m_useGroupSpecificChannelNumbers = kodi::addon::GetSettingBoolean("usegroupspecificnumbers", false);
  m_useStandardServiceReference = kodi::addon::GetSettingBoolean("usestandardserviceref", true);
  m_retrieveProviderNameForChannels = kodi::addon::GetSettingBoolean("retrieveprovidername", true);
  m_defaultProviderName = kodi::addon::GetSettingString("defaultprovidername", "");
  m_mapProviderNameFile = kodi::addon::GetSettingString("providermapfile", DEFAULT_PROVIDER_NAME_MAP_FILE);
  m_tvChannelGroupMode = kodi::addon::GetSettingEnum<ChannelGroupMode>("tvgroupmode", ChannelGroupMode::ALL_GROUPS);
  m_numTVGroups = kodi::addon::GetSettingInt("numtvgroups", DEFAULT_NUM_GROUPS);
  m_oneTVGroup = kodi::addon::GetSettingString("onetvgroup");
  m_twoTVGroup = kodi::addon::GetSettingString("twotvgroup");
  m_threeTVGroup = kodi::addon::GetSettingString("threetvgroup");
  m_fourTVGroup = kodi::addon::GetSettingString("fourtvgroup");
  m_fiveTVGroup = kodi::addon::GetSettingString("fivetvgroup");

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

  m_customTVGroupsFile = kodi::addon::GetSettingString("customtvgroupsfile", DEFAULT_CUSTOM_TV_GROUPS_FILE);

  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_tvFavouritesMode = kodi::addon::GetSettingEnum<FavouritesGroupMode>("tvfavouritesmode", FavouritesGroupMode::DISABLED);
  m_excludeLastScannedTVGroup = kodi::addon::GetSettingBoolean("excludelastscannedtv", true);
  m_radioChannelGroupMode = kodi::addon::GetSettingEnum<ChannelGroupMode>("radiogroupmode", ChannelGroupMode::ALL_GROUPS);
  m_numRadioGroups = kodi::addon::GetSettingInt("numradiogroups", DEFAULT_NUM_GROUPS);
  m_oneRadioGroup = kodi::addon::GetSettingString("oneradiogroup");
  m_twoRadioGroup = kodi::addon::GetSettingString("tworadiogroup");
  m_threeRadioGroup = kodi::addon::GetSettingString("threeradiogroup");
  m_fourRadioGroup = kodi::addon::GetSettingString("fourradiogroup");
  m_fiveRadioGroup = kodi::addon::GetSettingString("fiveradiogroup");

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

  m_customRadioGroupsFile = kodi::addon::GetSettingString("customradiogroupsfile", DEFAULT_CUSTOM_RADIO_GROUPS_FILE);

  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  m_radioFavouritesMode = kodi::addon::GetSettingEnum<FavouritesGroupMode>("radiofavouritesmode", FavouritesGroupMode::DISABLED);
  m_excludeLastScannedRadioGroup = kodi::addon::GetSettingBoolean("excludelastscannedradio", true);

  //EPG
  m_extractShowInfo = kodi::addon::GetSettingBoolean("extractshowinfoenabled", false);
  m_extractShowInfoFile = kodi::addon::GetSettingString("extractshowinfofile", DEFAULT_SHOW_INFO_FILE);
  m_mapGenreIds = kodi::addon::GetSettingBoolean("genreidmapenabled", false);
  m_mapGenreIdsFile = kodi::addon::GetSettingString("genreidmapfile", DEFAULT_GENRE_ID_MAP_FILE);
  m_mapRytecTextGenres = kodi::addon::GetSettingBoolean("rytecgenretextmapenabled", false);
  m_mapRytecTextGenresFile = kodi::addon::GetSettingString("rytecgenretextmapfile", DEFAULT_GENRE_ID_MAP_FILE);
  m_logMissingGenreMappings = kodi::addon::GetSettingBoolean("logmissinggenremapping", false);
  m_epgDelayPerChannel = kodi::addon::GetSettingInt("epgdelayperchannel", 0);

  //Recording
  m_storeLastPlayedAndCount = kodi::addon::GetSettingBoolean("storeextrarecordinginfo", false);
  m_recordingLastPlayedMode = kodi::addon::GetSettingEnum<RecordingLastPlayedMode>("sharerecordinglastplayed", RecordingLastPlayedMode::ACROSS_KODI_INSTANCES);
  m_onlyCurrentLocation = kodi::addon::GetSettingBoolean("onlycurrent", false);
  m_virtualFolders = kodi::addon::GetSettingBoolean("virtualfolders", false);
  m_keepFolders = kodi::addon::GetSettingBoolean("keepfolders", false);
  m_keepFoldersOmitLocation = kodi::addon::GetSettingBoolean("keepfoldersomitlocation", false);
  m_recordingsRecursive = kodi::addon::GetSettingBoolean("recordingsrecursive", false);
  m_enableRecordingEDLs = kodi::addon::GetSettingBoolean("enablerecordingedls", false);
  m_edlStartTimePadding = kodi::addon::GetSettingInt("edlpaddingstart", 0);
  m_edlStopTimePadding = kodi::addon::GetSettingInt("edlpaddingstop", 0);

  //Timers
  m_enableGenRepeatTimers = kodi::addon::GetSettingBoolean("enablegenrepeattimers", true);
  m_numGenRepeatTimers = kodi::addon::GetSettingInt("numgenrepeattimers", DEFAULT_NUM_GEN_REPEAT_TIMERS);
  m_automaticTimerlistCleanup = kodi::addon::GetSettingBoolean("timerlistcleanup", false);
  m_newTimerRecordingPath = kodi::addon::GetSettingString("recordingpath");
  m_enableAutoTimers = kodi::addon::GetSettingBoolean("enableautotimers", true);
  m_limitAnyChannelAutoTimers = kodi::addon::GetSettingBoolean("limitanychannelautotimers", true);
  m_limitAnyChannelAutoTimersToChannelGroups = kodi::addon::GetSettingBoolean("limitanychannelautotimerstogroups", true);

  //Timeshift
  m_timeshift = kodi::addon::GetSettingEnum<Timeshift>("enabletimeshift", Timeshift::OFF);
  m_timeshiftBufferPath = kodi::addon::GetSettingString("timeshiftbufferpath", ADDON_DATA_BASE_DIR);
  m_enableTimeshiftDiskLimit = kodi::addon::GetSettingBoolean("enabletimeshiftdisklimit", false);
  m_timeshiftDiskLimitGB = kodi::addon::GetSettingFloat("timeshiftdisklimit", 4.0f);
  m_timeshiftEnabledIptv = kodi::addon::GetSettingBoolean("timeshiftEnabledIptv", true);
  m_useFFmpegReconnect = kodi::addon::GetSettingBoolean("useFFmpegReconnect", true);
  m_useMpegtsForUnknownStreams = kodi::addon::GetSettingBoolean("useMpegtsForUnknownStreams", true);

  //Backend
  m_wakeOnLanMac = kodi::addon::GetSettingString("wakeonlanmac");
  m_powerstateMode = kodi::addon::GetSettingEnum<PowerstateMode>("powerstatemode", PowerstateMode::DISABLED);

  //Advanced
  m_prependOutline = kodi::addon::GetSettingEnum<PrependOutline>("prependoutline", PrependOutline::IN_EPG);
  m_readTimeout = kodi::addon::GetSettingInt("readtimeout", 0);
  m_streamReadChunkSize = kodi::addon::GetSettingInt("streamreadchunksize", 0);
  m_noDebug = kodi::addon::GetSettingBoolean("nodebug", false);
  m_debugNormal = kodi::addon::GetSettingBoolean("debugnormal", false);
  m_traceDebug = kodi::addon::GetSettingBoolean("tracedebug", false);

  // Now that we've read all the settings construct the connection URL

  m_connectionURL.clear();
  // simply add user@pass in front of the URL if username/password is set
  if ((m_username.length() > 0) && (m_password.length() > 0))
    m_connectionURL = StringUtils::Format("%s:%s@", WebUtils::URLEncodeInline(m_username).c_str(), WebUtils::URLEncodeInline(m_password).c_str());
  if (!m_useSecureHTTP)
    m_connectionURL = StringUtils::Format("http://%s%s:%u/", m_connectionURL.c_str(), m_hostname.c_str(), m_portWeb);
  else
    m_connectionURL = StringUtils::Format("https://%s%s:%u/", m_connectionURL.c_str(), m_hostname.c_str(), m_portWeb);
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
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
  else if (settingName == "retrieveprovidername")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_retrieveProviderNameForChannels, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "defaultprovidername")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_defaultProviderName, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "providermapfile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_mapProviderNameFile, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
  //Recordings
  else if (settingName == "storeextrarecordinginfo")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_storeLastPlayedAndCount, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "sharerecordinglastplayed")
    return SetEnumSetting<RecordingLastPlayedMode, ADDON_STATUS>(settingName, settingValue, m_recordingLastPlayedMode, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "onlycurrent")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_onlyCurrentLocation, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "virtualfolders")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_virtualFolders, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "keepfolders")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_keepFolders, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "keepfoldersomitlocation")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_keepFoldersOmitLocation, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "recordingsrecursive")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_recordingsRecursive, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "enablerecordingedls")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableRecordingEDLs, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
  else if (settingName == "edlpaddingstart")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_edlStartTimePadding, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "edlpaddingstop")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_edlStopTimePadding, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Timers
  else if (settingName == "enablegenrepeattimers")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableGenRepeatTimers, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numgenrepeattimers")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numGenRepeatTimers, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timerlistcleanup")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_automaticTimerlistCleanup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "recordingpath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_newTimerRecordingPath, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
  else if (settingName == "enabletimeshiftdisklimit")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableTimeshiftDiskLimit, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftdisklimit")
    return SetSetting<float, ADDON_STATUS>(settingName, settingValue, m_timeshiftDiskLimitGB, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftEnabledIptv")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabledIptv, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "useFFmpegReconnect")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useFFmpegReconnect, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "useMpegtsForUnknownStreams")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useMpegtsForUnknownStreams, ADDON_STATUS_OK, ADDON_STATUS_OK);
  //Backend
  else if (settingName == "wakeonlanmac")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_wakeOnLanMac, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "globalstartpaddingstb")
  {
    if (m_admin && m_deviceSettingsSet && SetSetting<int, bool>(settingName, settingValue, m_globalStartPaddingStb, true, false))
      m_admin->SendGlobalRecordingStartMarginSetting(m_globalStartPaddingStb);
  }
  else if (settingName == "globalendpaddingstb")
  {
    if (m_admin && m_deviceSettingsSet && SetSetting<int, bool>(settingName, settingValue, m_globalEndPaddingStb, true, false))
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
