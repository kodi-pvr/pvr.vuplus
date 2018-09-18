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
    MANUAL_ONCE      = PVR_TIMER_TYPE_NONE + 1,
    MANUAL_REPEATING = PVR_TIMER_TYPE_NONE + 2,
    EPG_ONCE         = PVR_TIMER_TYPE_NONE + 3,
    EPG_REPEATING    = PVR_TIMER_TYPE_NONE + 4, //Can't be created on Kodi, only on the engima2 box
    EPG_AUTO_SEARCH  = PVR_TIMER_TYPE_NONE + 5, //Not supporterd yet
    EPG_AUTO_ONCE    = PVR_TIMER_TYPE_NONE + 6, //Not supporterd yet
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
  std::string tags;

  Timer()
  {
    iUpdateState = VU_UPDATE_STATE_NEW;
  }

  bool isScheduled() const;
  bool isRunning(std::time_t *now, std::string *channelName = nullptr) const;

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
    bChanged = bChanged && (! strTitle.compare(right.strTitle));
    bChanged = bChanged && (! strPlot.compare(right.strPlot));

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
  Vu &vuData;

  //templates
  template <typename T>
  T *GetTimer(std::function<bool (const T&)> func,
      std::vector<T> &timerlist);

  // functions
  std::vector<Timer> LoadTimers();
  static bool FindTagInTimerTags(std::string tag, std::string tags);

public:

  Timers(Vu &data)
    : vuData(data)
  {
      m_iClientIndexCounter = 1;
  };
  
  void GetTimerTypes(std::vector<PVR_TIMER_TYPE> &types);
  int GetTimerCount();
  void GetTimers(std::vector<PVR_TIMER> &timers);

  Timer *GetTimer(std::function<bool (const Timer&)> func);

  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);

  void ClearTimers();
  void TimerUpdates();
};
} // end vuplus