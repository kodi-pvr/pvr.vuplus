#pragma once

#include "utilities/Logger.h"

#include <string>

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
  static const int DEFAULT_NUM_GEN_REPEAT_TIMERS = 1;

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
    const std::string& GetHostname() const { return m_strHostname; }
    int GetWebPortNum() const { return m_iPortWeb; }
    bool GetUseSecureConnection() const { return m_bUseSecureHTTP; }
    const std::string& GetUsername() const {return m_strUsername; }
    const std::string& GetPassword() const { return m_strPassword; }
    bool GetAutoConfigLiveStreamsEnabled() const { return m_bAutoConfig; }
    int GetStreamPortNum() const { return m_iPortStream; }
    bool UseSecureConnectionStream() const { return m_bUseSecureHTTPStream; }
    bool UseLoginStream() const { return m_bUseLoginStream; }

    //General
    bool GetUseOnlinePicons() const { return m_bOnlinePicons; }
    bool GetUsePiconsEuFormat() const { return m_bUsePiconsEuFormat; }
    const std::string& GetIconPath() const { return m_strIconPath; }
    int GetUpdateIntervalMins() const { return m_iUpdateInterval; }

    //Channel & EPG
    bool GetOneGroupOnly() const { return m_bOnlyOneGroup; }
    const std::string& GetOneGroupName() const { return m_strOneGroup; }
    bool GetZapBeforeChannelSwitch() const { return m_bZap; }
    bool GetExtractExtraEpgInfo() const { return m_bExtractExtraEpgInfo; }
    bool GetLogMissingGenreMappings() const { return m_bLogMissingGenreMappings; }

    //Recordings and Timers
    const std::string& GetRecordingPath() const { return m_strRecordingPath; }
    bool GetRecordingsFromCurrentLocationOnly() const { return m_bOnlyCurrentLocation; }
    bool GetKeepRecordingsFolders() const { return m_bKeepFolders; }
    bool GetGenRepeatTimersEnabled() const { return m_bEnableGenRepeatTimers; }
    int GetNumGenRepeatTimers() const { return m_iNumGenRepeatTimers; }
    bool GetAutoTimersEnabled() const { return m_bEnableAutoTimers; }
    bool GetAutoTimerListCleanupEnabled() const { return m_bAutomaticTimerlistCleanup; }

    //Timeshift
    const Timeshift& GetTimeshift() const { return m_timeshift; }
    const std::string& GetTimeshiftBufferPath() const { return m_strTimeshiftBufferPath; }
    bool IsTimeshiftBufferPathValid() const;

    //Advanced
    const PrependOutline& GetPrependOutline() const { return m_prependOutline; }
    bool GetDeepStandbyOnAddonExit() const { return m_bSetPowerstate; }
    int GetReadTimeoutSecs() const { return m_iReadTimeout; }
    int GetStreamReadChunkSizeKb() const { return m_streamReadChunkSize; }

    const std::string& GetConnectionURL() const { return m_connectionURL; }

    unsigned int GetWebIfVersionAsNum() const { return m_webIfVersion; }
    void SetWebIfVersionAsNum(unsigned int value) { m_webIfVersion = value; }

    inline unsigned int GenerateWebIfVersionAsNum(unsigned int major, unsigned int minor, unsigned int patch)
    {
      return (major << 16 | minor << 8 | patch);
    };

  private:
    Settings() = default;
    
    Settings(Settings const &) = delete;
    void operator=(Settings const &) = delete;

    template <typename T>
    ADDON_STATUS SetSetting(const std::string& settingName, const void* settingValue, T& currentValue, ADDON_STATUS returnValueIfChanged)
    {
      T newValue =  *static_cast<const T*>(settingValue);
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_NOTICE, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }
      return ADDON_STATUS_OK;
    };

    ADDON_STATUS SetStringSetting(const std::string &settingName, const void* settingValue, std::string &currentValue, ADDON_STATUS returnValueIfChanged);

    //Connection
    std::string m_strHostname = DEFAULT_HOST;
    int m_iPortWeb = DEFAULT_WEB_PORT;
    bool m_bUseSecureHTTP = false;
    std::string m_strUsername = "";
    std::string m_strPassword = "";
    bool m_bAutoConfig = false;
    int m_iPortStream = DEFAULT_STREAM_PORT;
    bool m_bUseSecureHTTPStream = false;
    bool m_bUseLoginStream = false;

    //General
    bool m_bOnlinePicons = true;
    bool m_bUsePiconsEuFormat = false;
    std::string m_strIconPath = "";
    int m_iUpdateInterval = DEFAULT_UPDATE_INTERVAL;
    
    //Channel & EPG
    bool m_bOnlyOneGroup = false;
    std::string m_strOneGroup = "";
    bool m_bZap = false;
    bool m_bExtractExtraEpgInfo = true;
    bool m_bLogMissingGenreMappings = true;

    //Recordings and Timers
    std::string m_strRecordingPath = "";
    bool m_bOnlyCurrentLocation = false;
    bool m_bKeepFolders = false;
    bool m_bEnableGenRepeatTimers = true;
    int  m_iNumGenRepeatTimers = DEFAULT_NUM_GEN_REPEAT_TIMERS;
    bool m_bEnableAutoTimers = true;
    bool m_bAutomaticTimerlistCleanup = false;

    //Timeshift
    Timeshift m_timeshift = Timeshift::OFF;
    std::string m_strTimeshiftBufferPath = DEFAULT_TSBUFFERPATH;

    //Advanced
    PrependOutline m_prependOutline = PrependOutline::IN_EPG;
    bool m_bSetPowerstate = false;
    int m_iReadTimeout = 0;
    int m_streamReadChunkSize = 0;

    std::string m_connectionURL;
    unsigned int m_webIfVersion;

    //PVR Props
    std::string m_szUserPath = "";
    std::string m_szClientPath = "";
  };
} //namespace enigma2
