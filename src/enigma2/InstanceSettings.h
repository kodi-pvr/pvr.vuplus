/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Admin.h"
#include "utilities/DeviceInfo.h"
#include "utilities/DeviceSettings.h"
#include "utilities/Logger.h"

#include <string>
#include <vector>

#include <kodi/tools/StringUtils.h>
#include <kodi/AddonBase.h>

class Vu;

namespace enigma2
{
  static const std::string DEFAULT_HOST = "127.0.0.1";
  static const int DEFAULT_CONNECT_TIMEOUT = 30;
  static const int DEFAULT_STREAM_PORT = 8001;
  static const int DEFAULT_WEB_PORT = 80;
  static const int DEFAULT_CONNECTION_CHECK_TIMEOUT_SECS = 10;
  static const int DEFAULT_CONNECTION_CHECK_INTERVAL_SECS = 1;
  static const int DEFAULT_UPDATE_INTERVAL = 2;
  static const int DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR = 4;
  static const int DEFAULT_NUM_GROUPS = 1;
  static const std::string ADDON_DATA_BASE_DIR = "special://userdata/addon_data/pvr.vuplus";
  static const std::string DEFAULT_PROVIDER_NAME_MAP_FILE = ADDON_DATA_BASE_DIR + "/providers/providerMappings.xml";
  static const std::string DEFAULT_SHOW_INFO_FILE = ADDON_DATA_BASE_DIR + "/showInfo/English-ShowInfo.xml";
  static const std::string DEFAULT_GENRE_ID_MAP_FILE = ADDON_DATA_BASE_DIR + "/genres/genreIdMappings/Sky-UK.xml";
  static const std::string DEFAULT_GENRE_TEXT_MAP_FILE = ADDON_DATA_BASE_DIR + "/genres/genreRytecTextMappings/Rytec-UK-Ireland.xml";
  static const std::string DEFAULT_CUSTOM_TV_GROUPS_FILE = ADDON_DATA_BASE_DIR + "/channelGroups/customTVGroups-example.xml";
  static const std::string DEFAULT_CUSTOM_RADIO_GROUPS_FILE = ADDON_DATA_BASE_DIR + "/channelGroups/customRadioGroups-example.xml";
  static const int DEFAULT_NUM_GEN_REPEAT_TIMERS = 1;

  enum class UpdateMode
    : int // same type as addon settings
  {
    TIMERS_AND_RECORDINGS = 0,
    TIMERS_ONLY
  };

  enum class ChannelAndGroupUpdateMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    NOTIFY_AND_LOG,
    RELOAD_CHANNELS_AND_GROUPS
  };

  enum class ChannelGroupMode
    : int // same type as addon settings
  {
    ALL_GROUPS = 0,
    SOME_GROUPS,
    FAVOURITES_GROUP,
    CUSTOM_GROUPS
  };

  enum class FavouritesGroupMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    AS_FIRST_GROUP,
    AS_LAST_GROUP
  };

  enum class RecordingLastPlayedMode
    : int // same type as addon settings
  {
    ACROSS_KODI_INSTANCES = 0,
    ACROSS_KODI_AND_E2_INSTANCES
  };

  enum class Timeshift
    : int // same type as addon settings
  {
    OFF = 0,
    ON_PLAYBACK,
    ON_PAUSE
  };

  enum class PrependOutline
    : int // same type as addon settings
  {
    NEVER = 0,
    IN_EPG,
    IN_RECORDINGS,
    ALWAYS
  };

  enum class PowerstateMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    STANDBY,
    DEEP_STANDBY,
    WAKEUP_THEN_STANDBY
  };

  class ATTR_DLL_LOCAL InstanceSettings
  {
  public:
    explicit InstanceSettings(kodi::addon::IAddonInstance& instance);

    void ReadSettings();
    ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

    //Connection
    const std::string& GetHostname() const { return m_hostname; }
    int GetWebPortNum() const { return m_portWeb; }
    bool GetUseSecureConnection() const { return m_useSecureHTTP; }
    const std::string& GetUsername() const { return m_username; }
    const std::string& GetPassword() const { return m_password; }
    bool GetAutoConfigLiveStreamsEnabled() const { return m_autoConfig; }
    int GetStreamPortNum() const { return m_portStream; }
    bool UseSecureConnectionStream() const { return m_useSecureHTTPStream; }
    bool UseLoginStream() const { return m_useLoginStream; }
    int GetConnectioncCheckTimeoutSecs() const { return m_connectioncCheckTimeoutSecs; }
    int GetConnectioncCheckIntervalSecs() const { return m_connectioncCheckIntervalSecs; }

    //General
    bool SetStreamProgramID() const { return m_setStreamProgramId; }
    bool UseOnlinePicons() const { return m_onlinePicons; }
    bool UsePiconsEuFormat() const { return m_usePiconsEuFormat; }
    bool UseOpenWebIfPiconPath() const { return m_useOpenWebIfPiconPath; }
    const std::string& GetIconPath() const { return m_iconPath; }
    int GetUpdateIntervalMins() const { return m_updateInterval; }
    UpdateMode GetUpdateMode() const { return m_updateMode; }
    int GetChannelAndGroupUpdateHour() const { return m_channelAndGroupUpdateHour; }
    ChannelAndGroupUpdateMode GetChannelAndGroupUpdateMode() const { return m_channelAndGroupUpdateMode; }

    //Channel
    bool GetZapBeforeChannelSwitch() const { return m_zap; }
    bool UseGroupSpecificChannelNumbers() const { return m_useGroupSpecificChannelNumbers; }
    bool UseStandardServiceReference() const { return m_useStandardServiceReference; }
    bool RetrieveProviderNameForChannels() const { return m_retrieveProviderNameForChannels; }
    bool HasDefaultProviderName() const { return !m_defaultProviderName.empty(); }
    const std::string& GetDefaultProviderName() const { return m_defaultProviderName; }
    const std::string& GetProviderNameMapFile() const { return m_mapProviderNameFile; }
    const ChannelGroupMode& GetTVChannelGroupMode() const { return m_tvChannelGroupMode; }
    const std::string& GetCustomTVGroupsFile() const { return m_customTVGroupsFile; }
    const FavouritesGroupMode& GetTVFavouritesMode() const { return m_tvFavouritesMode; }
    bool ExcludeLastScannedTVGroup() const { return m_excludeLastScannedTVGroup; }
    const ChannelGroupMode& GetRadioChannelGroupMode() const { return m_radioChannelGroupMode; }
    const std::string& GetCustomRadioGroupsFile() const { return m_customRadioGroupsFile; }
    const FavouritesGroupMode& GetRadioFavouritesMode() const { return m_radioFavouritesMode; }
    bool ExcludeLastScannedRadioGroup() const { return m_excludeLastScannedRadioGroup; }

    //EPG
    bool GetExtractShowInfo() const { return m_extractShowInfo; }
    const std::string& GetExtractShowInfoFile() const { return m_extractShowInfoFile; }
    bool GetMapGenreIds() const { return m_mapGenreIds; }
    const std::string& GetMapGenreIdsFile() const { return m_mapGenreIdsFile; }
    bool GetMapRytecTextGenres() const { return m_mapRytecTextGenres; }
    const std::string& GetMapRytecTextGenresFile() const { return m_mapRytecTextGenresFile; }
    bool GetLogMissingGenreMappings() const { return m_logMissingGenreMappings; }
    int GetEPGDelayPerChannelDelay() const { return m_epgDelayPerChannel; }

    //Recordings
    bool GetStoreRecordingLastPlayedAndCount() const { return m_storeLastPlayedAndCount; }
    const RecordingLastPlayedMode& GetRecordingLastPlayedMode() const { return m_recordingLastPlayedMode; }
    bool GetRecordingsFromCurrentLocationOnly() const { return m_onlyCurrentLocation; }
    bool GetVirtualRecordingsFolders() const { return m_virtualFolders; }
    bool GetKeepRecordingsFolders() const { return m_keepFolders; }
    bool GetRecordingsFoldersOmitLocation() const { return m_keepFoldersOmitLocation; }
    bool GetRecordingsRecursively() const { return m_recordingsRecursive; }
    bool GetRecordingEDLsEnabled() const { return m_enableRecordingEDLs; }
    int GetEDLStartTimePadding() const { return m_edlStartTimePadding; }
    int GetEDLStopTimePadding() const { return m_edlStopTimePadding; }

    //Timers
    bool GetGenRepeatTimersEnabled() const { return m_enableGenRepeatTimers; }
    int GetNumGenRepeatTimers() const { return m_numGenRepeatTimers; }
    bool GetAutomaticTimerListCleanupEnabled() const { return m_automaticTimerlistCleanup; }
    const std::string& GetNewTimerRecordingPath() const { return m_newTimerRecordingPath; }
    bool GetAutoTimersEnabled() const { return m_enableAutoTimers; }
    bool GetLimitAnyChannelAutoTimers() const { return m_limitAnyChannelAutoTimers; }
    bool GetLimitAnyChannelAutoTimersToChannelGroups() const { return m_limitAnyChannelAutoTimersToChannelGroups; }

    //Timeshift
    const Timeshift& GetTimeshift() const { return m_timeshift; }
    const std::string& GetTimeshiftBufferPath() const { return m_timeshiftBufferPath; }
    bool IsTimeshiftBufferPathValid() const;
    bool EnableTimeshiftDiskLimit() const { return m_enableTimeshiftDiskLimit; };
    float GetTimeshiftDiskLimitGB() const { return m_timeshiftDiskLimitGB; };
    uint64_t GetTimeshiftDiskLimitBytes() const { return static_cast<uint64_t>(1024LL * 1024LL * 1024LL * m_timeshiftDiskLimitGB); }
    bool IsTimeshiftEnabledIptv() const { return m_timeshiftEnabledIptv; }
    bool UseFFmpegReconnect() const { return m_useFFmpegReconnect; }
    bool UseMpegtsForUnknownStreams() const { return m_useMpegtsForUnknownStreams; }

    //Backend
    const std::string& GetWakeOnLanMac() const { return m_wakeOnLanMac; }
    PowerstateMode GetPowerstateModeOnAddonExit() const { return m_powerstateMode; }

    //Advanced
    const PrependOutline& GetPrependOutline() const { return m_prependOutline; }
    int GetReadTimeoutSecs() const { return m_readTimeout; }
    int GetStreamReadChunkSizeKb() const { return m_streamReadChunkSize; }

    const std::string& GetConnectionURL() const { return m_connectionURL; }

    unsigned int GetWebIfVersionAsNum() const { return m_deviceInfo->GetWebIfVersionAsNum(); }
    const std::string& GetWebIfVersion() const { return m_deviceInfo->GetWebIfVersion(); }

    const enigma2::utilities::DeviceInfo* GetDeviceInfo() const { return m_deviceInfo; }
    void SetDeviceInfo(enigma2::utilities::DeviceInfo* deviceInfo)
    {
      m_deviceInfo = deviceInfo;
      m_deviceInfoSet = true;
    }

    const enigma2::utilities::DeviceSettings* GetDeviceSettings() const { return m_deviceSettings; }
    void SetDeviceSettings(enigma2::utilities::DeviceSettings* deviceSettings)
    {
      m_deviceSettings = deviceSettings;
      m_globalStartPaddingStb = deviceSettings->GetGlobalRecordingStartMargin();
      m_globalEndPaddingStb = deviceSettings->GetGlobalRecordingEndMargin();
      m_deviceSettingsSet = true;
    }

    void SetAdmin(enigma2::Admin* admin) { m_admin = admin; }

    inline unsigned int GenerateWebIfVersionAsNum(unsigned int major, unsigned int minor, unsigned int patch) const
    {
      return (major << 16 | minor << 8 | patch);
    };

    bool CheckOpenWebIfVersion(unsigned int major, unsigned int minor, unsigned int patch) const
    {
      return m_deviceInfoSet ? GetWebIfVersionAsNum() >= GenerateWebIfVersionAsNum(major, minor, patch) && kodi::tools::StringUtils::StartsWith(GetWebIfVersion(), "OWIF") : m_deviceInfoSet;
    }

    bool IsOpenWebIf() const { return kodi::tools::StringUtils::StartsWith(GetWebIfVersion(), "OWIF"); }
    bool SupportsEditingRecordings() const { return CheckOpenWebIfVersion(1, 3, 6); }
    bool SupportsAutoTimers() const { return CheckOpenWebIfVersion(1, 3, 0); }
    bool SupportsTunerDetails() const { return CheckOpenWebIfVersion(1, 3, 5); }
    bool SupportsProviderNumberAndPiconForChannels() const { return CheckOpenWebIfVersion(1, 3, 5); }
    bool SupportsChannelNumberGroupStartPos() const { return CheckOpenWebIfVersion(1, 3, 8); }
    bool SupportsRecordingSizes() const { return CheckOpenWebIfVersion(1, 3, 9); }
    bool SupportsMovieListRecursive() const { return CheckOpenWebIfVersion(1, 4, 4); }
    bool SupportsMovieListOWFInternal() const { return CheckOpenWebIfVersion(1, 4, 6); }

    bool UsesLastScannedChannelGroup() const { return m_usesLastScannedChannelGroup; }
    void SetUsesLastScannedChannelGroup(bool value) { m_usesLastScannedChannelGroup = value; }

    std::vector<std::string>& GetCustomTVChannelGroupNameList() { return m_customTVChannelGroupNameList; }
    std::vector<std::string>& GetCustomRadioChannelGroupNameList() { return m_customRadioChannelGroupNameList; }

  private:
    InstanceSettings(const InstanceSettings&) = delete;
    void operator=(const InstanceSettings&) = delete;

    template<typename T, typename V>
    V SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue;
      if (std::is_same<T, float>::value)
        newValue = static_cast<T>(settingValue.GetFloat());
      else if (std::is_same<T, bool>::value)
        newValue = static_cast<T>(settingValue.GetBoolean());
      else if (std::is_same<T, unsigned int>::value)
        newValue = static_cast<T>(settingValue.GetUInt());
      else
        newValue = static_cast<T>(settingValue.GetInt());

      if (newValue != currentValue)
      {
        std::string formatString = "%s - Changed Setting '%s' from %d to %d";
        if (std::is_same<T, float>::value)
          formatString = "%s - Changed Setting '%s' from %f to %f";
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, formatString.c_str(), __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename T, typename V>
    V SetEnumSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue = settingValue.GetEnum<T>();
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename V>
    V SetStringSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, std::string& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      const std::string strSettingValue = settingValue.GetString();

      if (strSettingValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from '%s' to '%s'", __func__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
        currentValue = strSettingValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    static bool LoadCustomChannelGroupFile(std::string& file, std::vector<std::string>& channelGroupNameList);

    //Connection
    std::string m_hostname = DEFAULT_HOST;
    int m_portWeb = DEFAULT_WEB_PORT;
    bool m_useSecureHTTP = false;
    std::string m_username = "";
    std::string m_password = "";
    bool m_autoConfig = false;
    int m_portStream = DEFAULT_STREAM_PORT;
    bool m_useSecureHTTPStream = false;
    bool m_useLoginStream = false;
    int m_connectioncCheckTimeoutSecs = DEFAULT_CONNECTION_CHECK_TIMEOUT_SECS;
    int m_connectioncCheckIntervalSecs = DEFAULT_CONNECTION_CHECK_INTERVAL_SECS;

    //General
    bool m_setStreamProgramId = false;
    bool m_onlinePicons = true;
    bool m_usePiconsEuFormat = false;
    bool m_useOpenWebIfPiconPath = false;
    std::string m_iconPath = "";
    int m_updateInterval = DEFAULT_UPDATE_INTERVAL;
    UpdateMode m_updateMode = UpdateMode::TIMERS_AND_RECORDINGS;
    ChannelAndGroupUpdateMode m_channelAndGroupUpdateMode = ChannelAndGroupUpdateMode::RELOAD_CHANNELS_AND_GROUPS;
    int m_channelAndGroupUpdateHour = DEFAULT_CHANNEL_AND_GROUP_UPDATE_HOUR;

    //Channel
    bool m_zap = false;
    bool m_useGroupSpecificChannelNumbers = false;
    bool m_useStandardServiceReference = true;
    bool m_retrieveProviderNameForChannels = true;
    std::string m_defaultProviderName;
    std::string m_mapProviderNameFile = DEFAULT_PROVIDER_NAME_MAP_FILE;
    ChannelGroupMode m_tvChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
    int m_numTVGroups = DEFAULT_NUM_GROUPS;
    std::string m_oneTVGroup = "";
    std::string m_twoTVGroup = "";
    std::string m_threeTVGroup = "";
    std::string m_fourTVGroup = "";
    std::string m_fiveTVGroup = "";
    std::string m_customTVGroupsFile;
    FavouritesGroupMode m_tvFavouritesMode = FavouritesGroupMode::DISABLED;
    bool m_excludeLastScannedTVGroup = true;
    ChannelGroupMode m_radioChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
    int m_numRadioGroups = DEFAULT_NUM_GROUPS;
    std::string m_oneRadioGroup = "";
    std::string m_twoRadioGroup = "";
    std::string m_threeRadioGroup = "";
    std::string m_fourRadioGroup = "";
    std::string m_fiveRadioGroup = "";
    std::string m_customRadioGroupsFile;
    FavouritesGroupMode m_radioFavouritesMode = FavouritesGroupMode::DISABLED;
    bool m_excludeLastScannedRadioGroup = true;

    //EPG
    bool m_extractShowInfo = true;
    std::string m_extractShowInfoFile = DEFAULT_SHOW_INFO_FILE;
    bool m_mapGenreIds = true;
    std::string m_mapGenreIdsFile = DEFAULT_GENRE_ID_MAP_FILE;
    bool m_mapRytecTextGenres = false;
    std::string m_mapRytecTextGenresFile = DEFAULT_GENRE_TEXT_MAP_FILE;
    bool m_logMissingGenreMappings = true;
    int m_epgDelayPerChannel = 0;

    //Recordings
    bool m_storeLastPlayedAndCount = true;
    RecordingLastPlayedMode m_recordingLastPlayedMode = RecordingLastPlayedMode::ACROSS_KODI_INSTANCES;
    std::string m_newTimerRecordingPath = "";
    bool m_onlyCurrentLocation = false;
    bool m_virtualFolders = true;
    bool m_keepFolders = true;
    bool m_keepFoldersOmitLocation = true;
    bool m_recordingsRecursive = true;
    bool m_enableRecordingEDLs = false;
    int m_edlStartTimePadding = 0;
    int m_edlStopTimePadding = 0;

    //Timers
    bool m_enableGenRepeatTimers = true;
    int m_numGenRepeatTimers = DEFAULT_NUM_GEN_REPEAT_TIMERS;
    bool m_automaticTimerlistCleanup = false;
    bool m_enableAutoTimers = true;
    bool m_limitAnyChannelAutoTimers = true;
    bool m_limitAnyChannelAutoTimersToChannelGroups = true;

    //Timeshift
    Timeshift m_timeshift = Timeshift::OFF;
    std::string m_timeshiftBufferPath = ADDON_DATA_BASE_DIR;
    bool m_enableTimeshiftDiskLimit = false;
    float m_timeshiftDiskLimitGB = 4.0f;
    bool m_timeshiftEnabledIptv = true;
    bool m_useFFmpegReconnect = true;
    bool m_useMpegtsForUnknownStreams = true;

    //Backend
    std::string m_wakeOnLanMac;
    PowerstateMode m_powerstateMode = PowerstateMode::DISABLED;
    int m_globalStartPaddingStb = 0;
    int m_globalEndPaddingStb = 0;

    //Advanced
    PrependOutline m_prependOutline = PrependOutline::IN_EPG;
    int m_readTimeout = 0;
    int m_streamReadChunkSize = 0;

    //Last Scanned
    bool m_usesLastScannedChannelGroup = false;

    std::string m_connectionURL;
    enigma2::utilities::DeviceInfo* m_deviceInfo;
    enigma2::utilities::DeviceSettings* m_deviceSettings;
    enigma2::Admin* m_admin = nullptr;
    bool m_deviceInfoSet = false;
    bool m_deviceSettingsSet = false;

    std::vector<std::string> m_customTVChannelGroupNameList;
    std::vector<std::string> m_customRadioChannelGroupNameList;

    //PVR Props
    std::string m_szUserPath = "";
    std::string m_szClientPath = "";

    kodi::addon::IAddonInstance& m_instance;
  };
} //namespace enigma2
