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

#include <string>

#include "BaseEntry.h"
#include "Channel.h"
#include "../Channels.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"

namespace enigma2
{
  namespace data
  {
    class EpgEntry : public BaseEntry
    {
    public:
      int GetEventId() const { return m_eventId; }
      void SetEventId(int value) { m_eventId = value; }       

      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }          

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }       

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }      

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }    

      void UpdateTo(EPG_TAG &left) const;
      bool UpdateFrom(TiXmlElement* eventNode, enigma2::Channels &channels); 
      bool UpdateFrom(TiXmlElement* eventNode, const ChannelPtr channel, time_t iStart, time_t iEnd);

    private:    
      int m_eventId;
      std::string m_serviceReference;
      int m_channelId;
      time_t m_startTime;
      time_t m_endTime;
    };
  } //namespace data
} //namespace enigma2