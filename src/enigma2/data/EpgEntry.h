/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "BaseEntry.h"
#include "Channel.h"

#include <map>
#include <string>

#include <kodi/addon-instance/pvr/EPG.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class ATTR_DLL_LOCAL EpgEntry : public BaseEntry
    {
    public:
      unsigned int GetEpgId() const { return m_epgId; }
      void SetEpgId(int value) { m_epgId = value; }

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value) { m_serviceReference = value; }

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }

      const std::string& GetStartTimeW3CDate() const { return m_startTimeW3CDateString; }
      void SetStartTimeW3CDate(const std::string& value) { m_startTimeW3CDateString = value; }

      void UpdateTo(kodi::addon::PVREPGTag& left) const;
      bool UpdateFrom(TiXmlElement* eventNode, std::map<std::string, std::shared_ptr<Channel>>& m_channelsMap);
      bool UpdateFrom(TiXmlElement* eventNode, const std::shared_ptr<Channel>& channel, time_t iStart, time_t iEnd);

    protected:
      unsigned int m_epgId;
      std::string m_serviceReference;
      int m_channelId;
      time_t m_startTime = 0;
      time_t m_endTime = 0;
      std::string m_startTimeW3CDateString;
    };
  } //namespace data
} //namespace enigma2
