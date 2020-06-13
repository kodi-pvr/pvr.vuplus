/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../Channels.h"
#include "../utilities/UpdateState.h"
#include "BaseEntry.h"
#include "Tags.h"

#include <ctime>
#include <string>
#include <type_traits>

#include <kodi/addon-instance/pvr/Timers.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    static const std::string TAG_FOR_AUTOTIMER = "AutoTimer";
    static const std::string TAG_FOR_MANUAL_TIMER = "Manual";
    static const std::string TAG_FOR_EPG_TIMER = "EPG";
    static const std::string TAG_FOR_PADDING = "Padding";

    class ATTRIBUTE_HIDDEN Timer : public EpgEntry, public Tags
    {
    public:

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

      Timer()
      {
        m_updateState = enigma2::utilities::UPDATE_STATE_NEW;
        m_parentClientIndex = PVR_TIMER_NO_PARENT;
      }

      Type GetType() const { return m_type; }
      void SetType(const Type value) { m_type = value; }

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value) { m_channelName = value; }

      time_t GetRealStartTime() const { return m_startTime - (m_paddingStartMins * 60); }
      time_t GetRealEndTime() const { return m_endTime + (m_paddingEndMins * 60); }

      int GetWeekdays() const { return m_weekdays; }
      void SetWeekdays(int value) { m_weekdays = value; }

      PVR_TIMER_STATE GetState() const { return m_state; }
      void SetState(PVR_TIMER_STATE value) { m_state = value; }

      int GetUpdateState() const { return m_updateState; }
      void SetUpdateState(int value) { m_updateState = value; }

      unsigned int GetClientIndex() const { return m_clientIndex; }
      void SetClientIndex(unsigned int value) { m_clientIndex = value; }

      unsigned int GetParentClientIndex() const { return m_parentClientIndex; }
      void SetParentClientIndex(unsigned int value) { m_parentClientIndex = value; }

      int GetPaddingStartMins() const { return m_paddingStartMins; }
      void SetPaddingStartMins(int value) { m_paddingStartMins = value; }

      int GetPaddingEndMins() const { return m_paddingEndMins; }
      void SetPaddingEndMins(int value) { m_paddingEndMins = value; }

      bool IsScheduled() const;
      bool IsRunning(std::time_t* now, std::string* channelName, std::time_t startTime) const;
      bool IsChildOfParent(const Timer& parent) const;

      bool Like(const Timer& right) const;
      bool operator==(const Timer& right) const;
      void UpdateFrom(const Timer& right);
      void UpdateTo(kodi::addon::PVRTimer& right) const;
      bool UpdateFrom(TiXmlElement* timerNode, Channels& channels);

    protected:
      Type m_type = Type::MANUAL_ONCE;
      std::string m_channelName;
      int m_weekdays;
      PVR_TIMER_STATE m_state;
      int m_updateState;
      unsigned int m_clientIndex;
      unsigned int m_parentClientIndex;
      unsigned int m_paddingStartMins = 0;
      unsigned int m_paddingEndMins = 0;
    };
  } //namespace data
} //namespace enigma2
