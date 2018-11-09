#pragma once

#include <string>

#include "Admin.h"
#include "utilities/Logger.h"
#include "utilities/DeviceInfo.h"
#include "utilities/DeviceSettings.h"

#include "xbmc_addon_types.h"

class Vu;

namespace enigma2
{
  static const std::string DEFAULT_HOST = "127.0.0.1";
  static const int DEFAULT_CONNECT_TIMEOUT = 30;
  static const int DEFAULT_STREAM_PORT = 8001;
  static const int DEFAULT_WEB_PORT = 80;
  static const int DEFAULT_UPDATE_INTERVAL = 2;
  static const std::string DEFAULT_TSBUFFERPATH = "special://userdata/addon_data/pvr.vuplus";
  static const std::string DEFAULT_SHOW_INFO_FILE = "special://userdata/addon_data/pvr.vuplus/showInfo/English-ShowInfo.xml";
  static const std::string DEFAULT_GENRE_ID_MAP_FILE = "special://userdata/addon_data/pvr.vuplus/genres/genreIdMappings/Sky-UK.xml";
  static const std::string DEFAULT_GENRE_TEXT_MAP_FILE = "special://userdata/addon_data/pvr.vuplus/genres/genreTextMappings/Rytec-UK-Ireland";
  static const int DEFAULT_NUM_GEN_REPEAT_TIMERS = 1;

  enum class ChannelGroupMode
    : int // same type as addon settings
  {
    ALL_GROUPS = 0,
    ONLY_ONE_GROUP,
    FAVOURITES_GROUP
  };

  enum class FavouritesGroupMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    AS_FIRST_GROUP,
    AS_LAST_GROUP
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

  class Settings
  {
  public:
    /**
     * Singleton getter for the instance
     */
    static Settings& GetInstance()
    {
      static Settings settings;
      return settings;
    }

    void ReadFromAddon();
    ADDON_STATUS SetValue(const std::string &settingName, const void *settingValue);

    //Connection
    const std::string& GetHostname() const { return m_hostname; }
    int GetWebPortNum() const { return m_portWeb; }
    bool GetUseSecureConnection() const { return m_useSecureHTTP; }
    const std::string& GetUsername() const {return m_username; }
    const std::string& GetPassword() const { return m_password; }
    bool GetAutoConfigLiveStreamsEnabled() const { return m_autoConfig; }
    int GetStreamPortNum() const { return m_portStream; }
    bool UseSecureConnectionStream() const { return m_useSecureHTTPStream; }
    bool UseLoginStream() const { return m_useLoginStream; }

    //General
    bool UseOnlinePicons() const { return m_onlinePicons; }
    bool UsePiconsEuFormat() const { return m_usePiconsEuFormat; }
    const std::string& GetIconPath() const { return m_iconPath; }
    int GetUpdateIntervalMins() const { return m_updateInterval; }

    //Channel
    bool GetZapBeforeChannelSwitch() const { return m_zap; }
    const ChannelGroupMode& GetTVChannelGroupMode() const { return m_tvChannelGroupMode; }
    const std::string& GetOneTVGroupName() const { return m_oneTVGroup; }
    const FavouritesGroupMode& GetTVFavouritesMode() const { return m_tvFavouritesMode; }
    const ChannelGroupMode& GetRadioChannelGroupMode() const { return m_radioChannelGroupMode; }
    const std::string& GetOneRadioGroupName() const { return m_oneRadioGroup; }
    const FavouritesGroupMode& GetRadioFavouritesMode() const { return m_radioFavouritesMode; }
   
    //EPG
    bool GetExtractShowInfo() const { return m_extractShowInfo; }
    const std::string& GetExtractShowInfoFile() const { return m_extractShowInfoFile; }
    bool GetMapGenreIds() const { return m_mapGenreIds; }
    const std::string& GetMapGenreIdsFile() const { return m_mapGenreIdsFile; }
    bool GetMapRytecTextGenres() const { return m_mapRytecTextGenres; }
    const std::string& GetMapRytecTextGenresFile() const { return m_mapRytecTextGenresFile; }
    bool GetLogMissingGenreMappings() const { return m_logMissingGenreMappings; }

    //Recordings and Timers
    const std::string& GetRecordingPath() const { return m_recordingPath; }
    bool GetRecordingsFromCurrentLocationOnly() const { return m_onlyCurrentLocation; }
    bool GetKeepRecordingsFolders() const { return m_keepFolders; }
    bool GetGenRepeatTimersEnabled() const { return m_enableGenRepeatTimers; }
    int GetNumGenRepeatTimers() const { return m_numGenRepeatTimers; }
    bool GetAutoTimersEnabled() const { return m_enableAutoTimers; }
    bool GetAutoTimerListCleanupEnabled() const { return m_automaticTimerlistCleanup; }

    //Timeshift
    const Timeshift& GetTimeshift() const { return m_timeshift; }
    const std::string& GetTimeshiftBufferPath() const { return m_timeshiftBufferPath; }
    bool IsTimeshiftBufferPathValid() const;

    //Advanced
    const PrependOutline& GetPrependOutline() const { return m_prependOutline; }
    PowerstateMode GetPowerstateModeOnAddonExit() const { return m_powerstateMode; }
    int GetReadTimeoutSecs() const { return m_readTimeout; }
    int GetStreamReadChunkSizeKb() const { return m_streamReadChunkSize; }

    const std::string& GetConnectionURL() const { return m_connectionURL; }

    unsigned int GetWebIfVersionAsNum() const { return m_deviceInfo->GetWebIfVersionAsNum(); }

    const enigma2::utilities::DeviceInfo* GetDeviceInfo() const { return m_deviceInfo; }
    void SetDeviceInfo(enigma2::utilities::DeviceInfo* deviceInfo) { m_deviceInfo = deviceInfo; }    

    const enigma2::utilities::DeviceSettings* GetDeviceSettings() const { return m_deviceSettings; }
    void SetDeviceSettings(enigma2::utilities::DeviceSettings* deviceSettings) 
    { 
      m_deviceSettings = deviceSettings; 
      m_globalStartPaddingStb = deviceSettings->GetGlobalRecordingStartMargin();
      m_globalEndPaddingStb = deviceSettings->GetGlobalRecordingEndMargin();
    }    

    void SetAdmin(enigma2::Admin* admin) { m_admin = admin; }    

    inline unsigned int GenerateWebIfVersionAsNum(unsigned int major, unsigned int minor, unsigned int patch)
    {
      return (major << 16 | minor << 8 | patch);
    };

  private:
    Settings() = default;
    
    Settings(Settings const &) = delete;
    void operator=(Settings const &) = delete;

    template <typename T, typename V>
    V SetSetting(const std::string& settingName, const void* settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue =  *static_cast<const T*>(settingValue);
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_NOTICE, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    };

    template <typename V>
    V SetStringSetting(const std::string &settingName, const void* settingValue, std::string &currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      const std::string strSettingValue = static_cast<const char*>(settingValue);

      if (strSettingValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_NOTICE, "%s - Changed Setting '%s' from %s to %s", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
        currentValue = strSettingValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

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

    //General
    bool m_onlinePicons = true;
    bool m_usePiconsEuFormat = false;
    std::string m_iconPath = "";
    int m_updateInterval = DEFAULT_UPDATE_INTERVAL;
    
    //Channel
    bool m_zap = false;
    ChannelGroupMode m_tvChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
    std::string m_oneTVGroup = "";
    FavouritesGroupMode m_tvFavouritesMode = FavouritesGroupMode::DISABLED;
    ChannelGroupMode m_radioChannelGroupMode = ChannelGroupMode::FAVOURITES_GROUP;
    std::string m_oneRadioGroup = "";
    FavouritesGroupMode m_radioFavouritesMode = FavouritesGroupMode::DISABLED;

    //EPG
    bool m_extractShowInfo = true;
    std::string m_extractShowInfoFile;
    bool m_mapGenreIds = true;
    std::string m_mapGenreIdsFile;
    bool m_mapRytecTextGenres = true;
    std::string m_mapRytecTextGenresFile;
    bool m_logMissingGenreMappings = true;

    //Recordings and Timers
    std::string m_recordingPath = "";
    bool m_onlyCurrentLocation = false;
    bool m_keepFolders = false;
    bool m_enableGenRepeatTimers = true;
    int  m_numGenRepeatTimers = DEFAULT_NUM_GEN_REPEAT_TIMERS;
    bool m_enableAutoTimers = true;
    bool m_automaticTimerlistCleanup = false;

    //Timeshift
    Timeshift m_timeshift = Timeshift::OFF;
    std::string m_timeshiftBufferPath = DEFAULT_TSBUFFERPATH;

    //Advanced
    PrependOutline m_prependOutline = PrependOutline::IN_EPG;
    PowerstateMode m_powerstateMode = PowerstateMode::DISABLED;
    int m_readTimeout = 0;
    int m_streamReadChunkSize = 0;

    //Backend
    int m_globalStartPaddingStb = 0;
    int m_globalEndPaddingStb = 0;

    std::string m_connectionURL;
    enigma2::utilities::DeviceInfo* m_deviceInfo;
    enigma2::utilities::DeviceSettings* m_deviceSettings;
    enigma2::Admin* m_admin;

    //PVR Props
    std::string m_szUserPath = "";
    std::string m_szClientPath = "";
  };
} //namespace enigma2
