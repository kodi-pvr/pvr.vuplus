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

#include "libXBMC_pvr.h"

namespace enigma2
{
  namespace data
  {
    class EPGEntry : public BaseEntry
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

      void UpdateTo(EPG_TAG &left) const
      {
        left.iUniqueBroadcastId  = m_eventId;
        left.strTitle            = m_title.c_str();
        left.iUniqueChannelId    = m_channelId;
        left.startTime           = m_startTime;
        left.endTime             = m_endTime;
        left.strPlotOutline      = m_plotOutline.c_str();
        left.strPlot             = m_plot.c_str();
        left.strOriginalTitle    = nullptr; // unused
        left.strCast             = nullptr; // unused
        left.strDirector         = nullptr; // unused
        left.strWriter           = nullptr; // unused
        left.iYear               = m_year;
        left.strIMDBNumber       = nullptr; // unused
        left.strIconPath         = ""; // unused
        left.iGenreType          = m_genreType;
        left.iGenreSubType       = m_genreSubType;
        left.strGenreDescription = m_genreDescription.c_str();
        left.firstAired          = 0;  // unused
        left.iParentalRating     = 0;  // unused
        left.iStarRating         = 0;  // unused
        left.bNotify             = false;
        left.iSeriesNumber       = m_seasonNumber;
        left.iEpisodeNumber      = m_episodeNumber;
        left.iEpisodePartNumber  = m_episodePartNumber;
        left.strEpisodeName      = ""; // unused
        left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
      }

    private:    
      int m_eventId;
      std::string m_serviceReference;
      int m_channelId;
      time_t m_startTime;
      time_t m_endTime;
    };
  } //namespace data
} //namespace enigma2