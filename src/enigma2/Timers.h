#pragma once

#include <string>
#include <memory>
#include <functional>
#include <ctime>
#include <type_traits>

#include "data/Timer.h"
#include "data/AutoTimer.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"

/* forward declaration */
class Enigma2;

namespace enigma2
{
  static const std::string AUTOTIMER_SEARCH_CASE_SENSITIVE = "sensitive";
  static const std::string AUTOTIMER_SEARCH_CASE_INSENITIVE = "";

  static const std::string AUTOTIMER_ENABLED_YES = "yes";
  static const std::string AUTOTIMER_ENABLED_NO = "no";

  static const std::string AUTOTIMER_ENCODING = "UTF-8";

  static const std::string AUTOTIMER_SEARCH_TYPE_EXACT = "exact";
  static const std::string AUTOTIMER_SEARCH_TYPE_DESCRIPTION = "description";
  static const std::string AUTOTIMER_SEARCH_TYPE_START = "start";
  static const std::string AUTOTIMER_SEARCH_TYPE_PARTIAL = "";

  static const std::string AUTOTIMER_AVOID_DUPLICATE_DISABLED = "";                       //Require Description to be unique - No 
  static const std::string AUTOTIMER_AVOID_DUPLICATE_SAME_SERVICE = "1";                  //Require Description to be unique - On same service
  static const std::string AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE = "2";                   //Require Description to be unique - On any service
  static const std::string AUTOTIMER_AVOID_DUPLICATE_ANY_SERVICE_OR_RECORDING = "3";      //Require Description to be unique - On any service/recording

  static const std::string AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE = "0";                 //Check for uniqueness in - Title
  static const std::string AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_SHORT_DESC = "1";  //Check for uniqueness in - Title and Short description
  static const std::string AUTOTIMER_CHECK_SEARCH_FOR_DUP_IN_TITLE_AND_ALL_DESCS = "2";   //Check for uniqueness in - Title and all descpritions

  static const std::string AUTOTIMER_DEFAULT = "";

  static const std::string TAG_FOR_AUTOTIMER = "AutoTimer";
  static const std::string TAG_FOR_MANUAL_TIMER = "Manual";
  static const std::string TAG_FOR_EPG_TIMER = "EPG";

  static const int DAYS_IN_WEEK = 7;

  class Timers
  {
  public:
    Timers(Enigma2 &data)
      : enigma(data)
    {
        m_iClientIndexCounter = 1;
    };
    
    void GetTimerTypes(std::vector<PVR_TIMER_TYPE> &types) const;

    int GetTimerCount() const;
    int GetAutoTimerCount() const;

    void GetTimers(std::vector<PVR_TIMER> &timers) const;
    void GetAutoTimers(std::vector<PVR_TIMER> &timers) const;

    enigma2::data::Timer *GetTimer(std::function<bool (const enigma2::data::Timer&)> func);
    enigma2::data::AutoTimer *GetAutoTimer(std::function<bool (const enigma2::data::AutoTimer&)> func);

    PVR_ERROR AddTimer(const PVR_TIMER &timer);
    PVR_ERROR AddAutoTimer(const PVR_TIMER &timer);

    PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
    PVR_ERROR UpdateAutoTimer(const PVR_TIMER &timer);

    PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
    PVR_ERROR DeleteAutoTimer(const PVR_TIMER &timer);

    void ClearTimers();
    void TimerUpdates();

  private:
    //templates
    template <typename T>
    T *GetTimer(std::function<bool (const T&)> func,
        std::vector<T> &timerlist);

    // functions
    std::vector<enigma2::data::Timer> LoadTimers() const;
    void GenerateChildManualRepeatingTimers(std::vector<enigma2::data::Timer> *timers, enigma2::data::Timer *timer) const;
    static bool FindTagInTimerTags(std::string tag, std::string tags);
    static std::string ConvertToAutoTimerTag(std::string tag);
    std::vector<enigma2::data::AutoTimer> LoadAutoTimers() const;
    bool CanAutoTimers() const;
    bool IsAutoTimer(const PVR_TIMER &timer) const;
    bool TimerUpdatesRegular();
    bool TimerUpdatesAuto();
    void ParseTime(const std::string &time, std::tm &timeinfo) const;
    static std::string BuildAddUpdateAutoTimerIncludeParams(int weekdays);

    // members
    unsigned int m_iUpdateTimer;
    unsigned int m_iClientIndexCounter;
    std::vector<enigma2::data::Timer> m_timers;
    std::vector<enigma2::data::AutoTimer> m_autotimers;
    Enigma2 &enigma;
  };
} // namespace enigma2