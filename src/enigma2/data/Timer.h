#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <ctime>
#include <type_traits>

#include "../Channels.h"
#include "../utilities/UpdateState.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    static const std::string TAG_FOR_AUTOTIMER = "AutoTimer";
    static const std::string TAG_FOR_MANUAL_TIMER = "Manual";
    static const std::string TAG_FOR_EPG_TIMER = "EPG";

    class Timer
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
      void SetType(const Type value ) { m_type = value; }

      const std::string& GetTitle() const { return m_title; }
      void SetTitle(const std::string& value ) { m_title = value; }

      const std::string& GetPlot() const { return m_plot; }
      void SetPlot(const std::string& value ) { m_plot = value; }

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value ) { m_channelName = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }

      int GetWeekdays() const { return m_weekdays; }
      void SetWeekdays(int value) { m_weekdays = value; }

      unsigned int GetEpgId() const { return m_epgId; }
      void SetEpgId(unsigned int value) { m_epgId = value; }

      PVR_TIMER_STATE GetState() const { return m_state; }
      void SetState(PVR_TIMER_STATE value) { m_state = value; }

      int GetUpdateState() const { return m_updateState; }
      void SetUpdateState(int value) { m_updateState = value; }

      unsigned int GetClientIndex() const { return m_clientIndex; }
      void SetClientIndex(unsigned int value) { m_clientIndex = value; }

      unsigned int GetParentClientIndex() const { return m_parentClientIndex; }
      void SetParentClientIndex(unsigned int value) { m_parentClientIndex = value; }

      const std::string& GetTags() const { return m_tags; }
      void SetTags(const std::string& value ) { m_tags = value; }

      bool isScheduled() const;
      bool isRunning(std::time_t *now, std::string *channelName = nullptr) const;
      bool isChildOfParent(const Timer &parent) const;

      bool Like(const Timer &right) const;
      bool operator==(const Timer &right) const;
      void UpdateFrom(const Timer &right);
      void UpdateTo(PVR_TIMER &right) const;
      bool UpdateFrom(TiXmlElement* timerNode, Channels &channels);
      bool ContainsTag(const std::string &tag) const;

    protected:
      Type m_type = Type::MANUAL_ONCE;
      std::string m_title;
      std::string m_plot;
      int m_channelId;
      std::string m_channelName;
      time_t m_startTime;
      time_t m_endTime;
      int m_weekdays;
      unsigned int m_epgId;
      PVR_TIMER_STATE m_state;
      int m_updateState;
      unsigned int m_clientIndex;
      unsigned int m_parentClientIndex;
      std::string m_tags;
    };
  } //namespace data
} //namespace enigma2