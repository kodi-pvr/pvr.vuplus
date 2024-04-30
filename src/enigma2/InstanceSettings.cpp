/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "InstanceSettings.h"

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
InstanceSettings::InstanceSettings(kodi::addon::IAddonInstance& instance)
  : m_instance(instance)
{
  ReadSettings();
}

void InstanceSettings::ReadSettings()
{
  //Connection
  m_instance.CheckInstanceSettingString("host", m_hostname);
  m_instance.CheckInstanceSettingInt("webport", m_portWeb);
  m_instance.CheckInstanceSettingBoolean("use_secure", m_useSecureHTTP);
  m_instance.CheckInstanceSettingString("user", m_username);
  m_instance.CheckInstanceSettingString("pass", m_password);
  m_instance.CheckInstanceSettingBoolean("autoconfig", m_autoConfig);
  m_instance.CheckInstanceSettingInt("streamport", m_portStream);
  m_instance.CheckInstanceSettingBoolean("use_secure_stream", m_useSecureHTTPStream);
  m_instance.CheckInstanceSettingBoolean("use_login_stream", m_useLoginStream);
  m_instance.CheckInstanceSettingInt("connectionchecktimeout", m_connectioncCheckTimeoutSecs);
  m_instance.CheckInstanceSettingInt("connectioncheckinterval", m_connectioncCheckIntervalSecs);

  //General
  m_instance.CheckInstanceSettingBoolean("setprogramid", m_setStreamProgramId);
  m_instance.CheckInstanceSettingBoolean("onlinepicons", m_onlinePicons);
  m_instance.CheckInstanceSettingBoolean("usepiconseuformat", m_usePiconsEuFormat);
  m_instance.CheckInstanceSettingBoolean("useopenwebifpiconpath", m_useOpenWebIfPiconPath);
  m_instance.CheckInstanceSettingString("iconpath", m_iconPath);
  m_instance.CheckInstanceSettingInt("updateint", m_updateInterval);
  m_instance.CheckInstanceSettingEnum<UpdateMode>("updatemode", m_updateMode);
  m_instance.CheckInstanceSettingEnum<ChannelAndGroupUpdateMode>("channelandgroupupdatemode", m_channelAndGroupUpdateMode);
  m_instance.CheckInstanceSettingInt("channelandgroupupdatehour", m_channelAndGroupUpdateHour);

  //Channels
  m_instance.CheckInstanceSettingBoolean("zap", m_zap);
  m_instance.CheckInstanceSettingBoolean("usegroupspecificnumbers", m_useGroupSpecificChannelNumbers);
  m_instance.CheckInstanceSettingBoolean("usestandardserviceref", m_useStandardServiceReference);
  m_instance.CheckInstanceSettingBoolean("retrieveprovidername", m_retrieveProviderNameForChannels);
  m_instance.CheckInstanceSettingString("defaultprovidername", m_defaultProviderName);
  m_instance.CheckInstanceSettingString("providermapfile", m_mapProviderNameFile);
  m_instance.CheckInstanceSettingEnum<ChannelGroupMode>("tvgroupmode", m_tvChannelGroupMode);
  m_instance.CheckInstanceSettingInt("numtvgroups", m_numTVGroups);
  m_instance.CheckInstanceSettingString("onetvgroup", m_oneTVGroup);
  m_instance.CheckInstanceSettingString("twotvgroup", m_twoTVGroup);
  m_instance.CheckInstanceSettingString("threetvgroup", m_threeTVGroup);
  m_instance.CheckInstanceSettingString("fourtvgroup", m_fourTVGroup);
  m_instance.CheckInstanceSettingString("fivetvgroup", m_fiveTVGroup);

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

  m_instance.CheckInstanceSettingString("customtvgroupsfile", m_customTVGroupsFile);

  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_instance.CheckInstanceSettingEnum<FavouritesGroupMode>("tvfavouritesmode", m_tvFavouritesMode);
  m_instance.CheckInstanceSettingBoolean("excludelastscannedtv", m_excludeLastScannedTVGroup);
  m_instance.CheckInstanceSettingEnum<ChannelGroupMode>("radiogroupmode", m_radioChannelGroupMode);
  m_instance.CheckInstanceSettingInt("numradiogroups", m_numRadioGroups);
  m_instance.CheckInstanceSettingString("oneradiogroup", m_oneRadioGroup);
  m_instance.CheckInstanceSettingString("tworadiogroup", m_twoRadioGroup);
  m_instance.CheckInstanceSettingString("threeradiogroup", m_threeRadioGroup);
  m_instance.CheckInstanceSettingString("fourradiogroup", m_fourRadioGroup);
  m_instance.CheckInstanceSettingString("fiveradiogroup", m_fiveRadioGroup);

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

  m_instance.CheckInstanceSettingString("customradiogroupsfile", m_customRadioGroupsFile);

  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  m_instance.CheckInstanceSettingEnum<FavouritesGroupMode>("radiofavouritesmode", m_radioFavouritesMode);
  m_instance.CheckInstanceSettingBoolean("excludelastscannedradio", m_excludeLastScannedRadioGroup);

  //EPG
  m_instance.CheckInstanceSettingBoolean("extractshowinfoenabled", m_extractShowInfo);
  m_instance.CheckInstanceSettingString("extractshowinfofile", m_extractShowInfoFile);
  m_instance.CheckInstanceSettingBoolean("genreidmapenabled", m_mapGenreIds);
  m_instance.CheckInstanceSettingString("genreidmapfile", m_mapGenreIdsFile);
  m_instance.CheckInstanceSettingBoolean("rytecgenretextmapenabled", m_mapRytecTextGenres);
  m_instance.CheckInstanceSettingString("rytecgenretextmapfile", m_mapRytecTextGenresFile);
  m_instance.CheckInstanceSettingBoolean("logmissinggenremapping", m_logMissingGenreMappings);
  m_instance.CheckInstanceSettingInt("epgdelayperchannel", m_epgDelayPerChannel);

  //Recording
  m_instance.CheckInstanceSettingBoolean("storeextrarecordinginfo", m_storeLastPlayedAndCount);
  m_instance.CheckInstanceSettingEnum<RecordingLastPlayedMode>("sharerecordinglastplayed", m_recordingLastPlayedMode);
  m_instance.CheckInstanceSettingBoolean("onlycurrent", m_onlyCurrentLocation);
  m_instance.CheckInstanceSettingBoolean("virtualfolders", m_virtualFolders);
  m_instance.CheckInstanceSettingBoolean("keepfolders", m_keepFolders);
  m_instance.CheckInstanceSettingBoolean("keepfoldersomitlocation", m_keepFoldersOmitLocation);
  m_instance.CheckInstanceSettingBoolean("recordingsrecursive", m_recordingsRecursive);
  m_instance.CheckInstanceSettingBoolean("enablerecordingedls", m_enableRecordingEDLs);
  m_instance.CheckInstanceSettingInt("edlpaddingstart", m_edlStartTimePadding);
  m_instance.CheckInstanceSettingInt("edlpaddingstop", m_edlStopTimePadding);

  //Timers
  m_instance.CheckInstanceSettingBoolean("enablegenrepeattimers", m_enableGenRepeatTimers);
  m_instance.CheckInstanceSettingInt("numgenrepeattimers", m_numGenRepeatTimers);
  m_instance.CheckInstanceSettingBoolean("timerlistcleanup", m_automaticTimerlistCleanup);
  m_instance.CheckInstanceSettingString("recordingpath", m_newTimerRecordingPath);
  m_instance.CheckInstanceSettingBoolean("enableautotimers", m_enableAutoTimers);
  m_instance.CheckInstanceSettingBoolean("limitanychannelautotimers", m_limitAnyChannelAutoTimers);
  m_instance.CheckInstanceSettingBoolean("limitanychannelautotimerstogroups", m_limitAnyChannelAutoTimersToChannelGroups);

  //Timeshift
  m_instance.CheckInstanceSettingEnum<Timeshift>("enabletimeshift", m_timeshift);
  m_instance.CheckInstanceSettingString("timeshiftbufferpath", m_timeshiftBufferPath);
  m_instance.CheckInstanceSettingBoolean("enabletimeshiftdisklimit", m_enableTimeshiftDiskLimit);
  m_instance.CheckInstanceSettingFloat("timeshiftdisklimit", m_timeshiftDiskLimitGB);
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabledIptv", m_timeshiftEnabledIptv);
  m_instance.CheckInstanceSettingBoolean("useFFmpegReconnect", m_useFFmpegReconnect);
  m_instance.CheckInstanceSettingBoolean("useMpegtsForUnknownStreams", m_useMpegtsForUnknownStreams);

  //Backend
  m_instance.CheckInstanceSettingString("wakeonlanmac", m_wakeOnLanMac);
  m_instance.CheckInstanceSettingEnum<PowerstateMode>("powerstatemode", m_powerstateMode);

  //Advanced
  m_instance.CheckInstanceSettingEnum<PrependOutline>("prependoutline", m_prependOutline);
  m_instance.CheckInstanceSettingInt("readtimeout", m_readTimeout);
  m_instance.CheckInstanceSettingInt("streamreadchunksize", m_streamReadChunkSize);

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

ADDON_STATUS InstanceSettings::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_updateInterval, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "updatemode")
    return SetEnumSetting<UpdateMode, ADDON_STATUS>(settingName, settingValue, m_updateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "channelandgroupupdatemode")
    return SetEnumSetting<ChannelAndGroupUpdateMode, ADDON_STATUS>(settingName, settingValue, m_channelAndGroupUpdateMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "channelandgroupupdatehour")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_channelAndGroupUpdateHour, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numTVGroups, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numRadioGroups, ADDON_STATUS_NEED_RESTART, ADDON_STATUS_OK);
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

  return ADDON_STATUS_OK;
}

bool InstanceSettings::IsTimeshiftBufferPathValid() const
{
  return kodi::vfs::DirectoryExists(m_timeshiftBufferPath) || kodi::vfs::DirectoryExists(ADDON_DATA_BASE_DIR);
}

bool InstanceSettings::LoadCustomChannelGroupFile(std::string& xmlFile, std::vector<std::string>& channelGroupNameList)
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
