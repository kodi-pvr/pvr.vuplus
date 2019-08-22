#pragma once
/*
 *      Copyleft (C) 2005-2015 Team XBMC
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

#include <map>
#include <string>

#include "BaseEntry.h"
#include "EpgChannel.h"

#include "kodi/libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    class EpgEntry : public BaseEntry
    {
    public:
      unsigned int GetEpgId() const { return m_epgId; }
      void SetEpgId(int value) { m_epgId = value; }

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }

      void UpdateTo(EPG_TAG &left) const;
      bool UpdateFrom(TiXmlElement* eventNode, std::map<std::string, std::shared_ptr<EpgChannel>> &m_epgChannelsMap);
      bool UpdateFrom(TiXmlElement* eventNode, const std::shared_ptr<EpgChannel> &epgChannel, time_t iStart, time_t iEnd);

    protected:
      unsigned int m_epgId;
      std::string m_serviceReference;
      int m_channelId;
      time_t m_startTime;
      time_t m_endTime;
    };
  } //namespace data
} //namespace enigma2