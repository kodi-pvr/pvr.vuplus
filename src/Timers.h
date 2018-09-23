#pragma once

#include "libXBMC_pvr.h"
#include "tinyxml.h"

#include <string>
#include <memory>
#include <functional>
#include <ctime>
#include <type_traits>

/* forward declaration */
class Vu;

namespace vuplus
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

static const int DAYS_IN_WEEK = 7;

typedef enum VU_UPDATE_STATE
{
    VU_UPDATE_STATE_NONE,
    VU_UPDATE_STATE_FOUND,
    VU_UPDATE_STATE_UPDATED,
    VU_UPDATE_STATE_NEW
} VU_UPDATE_STATE;

struct Timer
{
  enum Type
    : unsigned int // same type as PVR_TIMER_TYPE.iId
  {
    MANUAL_ONCE             = PVR_TIMER_TYPE_NONE + 1,
    MANUAL_REPEATING        = PVR_TIMER_TYPE_NONE + 2,
    READONLY_REPEATING_ONCE = PVR_TIMER_TYPE_NONE + 3,
    EPG_ONCE                = PVR_TIMER_TYPE_NONE + 4,
    EPG_REPEATING           = PVR_TIMER_TYPE_NONE + 5, //Can't be created on Kodi, only on the engima2 box
    EPG_AUTO_SEARCH         = PVR_TIMER_TYPE_NONE + 6, 
    EPG_AUTO_ONCE           = PVR_TIMER_TYPE_NONE + 7, 
  };

  Type type = Type::MANUAL_ONCE; 
  std::string strTitle;
  std::string strPlot;
  int iChannelId;
  std::string strChannelName;
  time_t startTime;
  time_t endTime;
  int iWeekdays;
  unsigned int iEpgID;
  PVR_TIMER_STATE state; 
  int iUpdateState;
  unsigned int iClientIndex;  
  unsigned int iParentClientIndex;  
  std::string tags;

  Timer()
  {
    iUpdateState = VU_UPDATE_STATE_NEW;
    iParentClientIndex = PVR_TIMER_NO_PARENT;
  }

  bool isScheduled() const;
  bool isRunning(std::time_t *now, std::string *channelName = nullptr) const;
  bool isChildOfParent(const Timer &parent) const;

  bool like(const Timer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime == right.startTime); 
    bChanged = bChanged && (endTime == right.endTime); 
    bChanged = bChanged && (iChannelId == right.iChannelId); 
    bChanged = bChanged && (iWeekdays == right.iWeekdays); 
    bChanged = bChanged && (iEpgID == right.iEpgID); 

    return bChanged;
  }
  
  bool operator==(const Timer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime == right.startTime); 
    bChanged = bChanged && (endTime == right.endTime); 
    bChanged = bChanged && (iChannelId == right.iChannelId); 
    bChanged = bChanged && (iWeekdays == right.iWeekdays); 
    bChanged = bChanged && (iEpgID == right.iEpgID); 
    bChanged = bChanged && (state == right.state); 
    bChanged = bChanged && (!strTitle.compare(right.strTitle));
    bChanged = bChanged && (!strPlot.compare(right.strPlot));

    return bChanged;
  }
};

struct AutoTimer
  : public Timer
{
  enum DeDup
    : unsigned int  // same type as PVR_TIMER_TYPE.iPreventDuplicateEpisodes
  {
    DISABLED                         = 0,
    CHECK_TITLE                      = 1,
    CHECK_TITLE_AND_SHORT_DESC       = 2,
    //Below is unsupported currently due to bug in the OpenWebIf API, the value cannot be unset if it's currently has a value other than disabled
    //Workaround is to disable, hit ok and then select this
    CHECK_TITLE_AND_ALL_DESCS        = 3
  };

  std::string searchPhrase;
  std::string encoding;
  std::string searchCase;
  std::string searchType;
  unsigned int backendId;
  bool searchFulltext = false;
  bool startAnyTime = false;
  bool endAnyTime   = false;
  bool anyChannel = false;
  std::underlying_type<DeDup>::type deDup = DeDup::DISABLED;

  AutoTimer() = default;

  bool like(const AutoTimer &right) const
  {
    return backendId == right.backendId;;
  }

  bool operator==(const AutoTimer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (searchPhrase == right.searchPhrase); 
    bChanged = bChanged && (searchType == right.searchType); 
    bChanged = bChanged && (searchCase == right.searchCase); 
    bChanged = bChanged && (startTime == right.startTime); 
    bChanged = bChanged && (endTime == right.endTime); 
    bChanged = bChanged && (iChannelId == right.iChannelId); 
    bChanged = bChanged && (iWeekdays == right.iWeekdays); 
    bChanged = bChanged && (state == right.state); 
    bChanged = bChanged && (searchFulltext == right.searchFulltext); 
    bChanged = bChanged && (startAnyTime == right.startAnyTime); 
    bChanged = bChanged && (endAnyTime == right.endAnyTime); 
    bChanged = bChanged && (anyChannel == right.anyChannel); 
    bChanged = bChanged && (deDup == right.deDup); 
    bChanged = bChanged && (!strTitle.compare(right.strTitle));

    return bChanged;
  }
};

class Timers
{
private:

  // members
  unsigned int m_iUpdateTimer;
  unsigned int m_iClientIndexCounter;
  std::vector<Timer> m_timers;
  std::vector<AutoTimer> m_autotimers;
  Vu &vuData;

  //templates
  template <typename T>
  T *GetTimer(std::function<bool (const T&)> func,
      std::vector<T> &timerlist);

  // functions
  std::vector<Timer> LoadTimers() const;
  void GenerateChildManualRepeatingTimers(std::vector<Timer> *timers, Timer *timer) const;
  static bool FindTagInTimerTags(std::string tag, std::string tags);
  static std::string ConvertToAutoTimerTag(std::string tag);
  std::vector<AutoTimer> LoadAutoTimers() const;
  bool CanAutoTimers() const;
  bool IsAutoTimer(const PVR_TIMER &timer) const;
  bool TimerUpdatesRegular();
  bool TimerUpdatesAuto();
  void ParseTime(const std::string &time, std::tm &timeinfo) const;
  static std::string BuildAddUpdateAutoTimerIncludeParams(int weekdays);

public:

  Timers(Vu &data)
    : vuData(data)
  {
      m_iClientIndexCounter = 1;
  };
  
  void GetTimerTypes(std::vector<PVR_TIMER_TYPE> &types) const;

  int GetTimerCount() const;
  int GetAutoTimerCount() const;

  void GetTimers(std::vector<PVR_TIMER> &timers) const;
  void GetAutoTimers(std::vector<PVR_TIMER> &timers) const;

  Timer *GetTimer(std::function<bool (const Timer&)> func);
  AutoTimer *GetAutoTimer(std::function<bool (const AutoTimer&)> func);

  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR AddAutoTimer(const PVR_TIMER &timer);

  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateAutoTimer(const PVR_TIMER &timer);

  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteAutoTimer(const PVR_TIMER &timer);

  void ClearTimers();
  void TimerUpdates();
};
} // end vuplus