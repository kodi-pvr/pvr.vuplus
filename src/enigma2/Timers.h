#pragma once

#include "Epg.h"
#include "data/AutoTimer.h"
#include "data/Timer.h"
#include "extract/EpgEntryExtractor.h"
#include "tinyxml.h"
#include "kodi/libXBMC_pvr.h"

#include <atomic>
#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

namespace enigma2
{
  class Timers
  {
  public:
    Timers(Channels& channels, ChannelGroups& channelGroups, std::vector<std::string>& locations, Epg& epg, enigma2::extract::EpgEntryExtractor& entryExtractor)
      : m_channels(channels), m_channelGroups(channelGroups), m_locations(locations), m_epg(epg), m_entryExtractor(entryExtractor)
    {
      m_clientIndexCounter = 1;
    };

    void GetTimerTypes(std::vector<PVR_TIMER_TYPE>& types) const;

    int GetTimerCount() const;
    int GetAutoTimerCount() const;

    void GetTimers(std::vector<PVR_TIMER>& timers) const;
    void GetAutoTimers(std::vector<PVR_TIMER>& timers) const;

    enigma2::data::Timer* GetTimer(std::function<bool(const enigma2::data::Timer&)> func);
    enigma2::data::AutoTimer* GetAutoTimer(std::function<bool(const enigma2::data::AutoTimer&)> func);

    PVR_ERROR AddTimer(const PVR_TIMER& timer);
    PVR_ERROR AddAutoTimer(const PVR_TIMER& timer);

    PVR_ERROR UpdateTimer(const PVR_TIMER& timer);
    PVR_ERROR UpdateAutoTimer(const PVR_TIMER& timer);

    PVR_ERROR DeleteTimer(const PVR_TIMER& timer);
    PVR_ERROR DeleteAutoTimer(const PVR_TIMER& timer);

    void ClearTimers();
    bool TimerUpdates();
    void RunAutoTimerListCleanup();
    void AddTimerChangeWatcher(std::atomic_bool* watcher);

  private:
    //templates
    template<typename T>
    T* GetTimer(std::function<bool(const T&)> func, std::vector<T>& timerlist);

    // functions
    std::vector<enigma2::data::Timer> LoadTimers() const;
    void GenerateChildManualRepeatingTimers(std::vector<enigma2::data::Timer>* timers, enigma2::data::Timer* timer) const;
    static std::string ConvertToAutoTimerTag(std::string tag);
    static std::string RemovePaddingTag(std::string tag);
    std::vector<enigma2::data::AutoTimer> LoadAutoTimers() const;
    bool IsAutoTimer(const PVR_TIMER& timer) const;
    bool TimerUpdatesRegular();
    bool TimerUpdatesAuto();
    std::string BuildAddUpdateAutoTimerLimitGroupsParams(const std::shared_ptr<data::Channel>& channel);
    static std::string BuildAddUpdateAutoTimerIncludeParams(int weekdays);

    // members
    unsigned int m_clientIndexCounter;
    std::vector<std::atomic_bool*> m_timerChangeWatchers;

    enigma2::extract::EpgEntryExtractor& m_entryExtractor;

    enigma2::Settings& m_settings = enigma2::Settings::GetInstance();
    std::vector<enigma2::data::Timer> m_timers;
    std::vector<enigma2::data::AutoTimer> m_autotimers;

    Channels& m_channels;
    ChannelGroups& m_channelGroups;
    std::vector<std::string>& m_locations;
    Epg& m_epg;
  };
} // namespace enigma2