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

#include "BaseEntry.h"
#include "Channel.h"
#include "../Channels.h"

#include "libXBMC_pvr.h"
#include "tinyxml.h"
namespace enigma2
{
  namespace data
  {
    class RecordingEntry : public BaseEntry
    {
    public:
      const std::string& GetRecordingId() const { return m_recordingId; }
      void SetRecordingId(const std::string& value ) { m_recordingId = value; }      

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }  
      
      int GetDuration() const { return m_duration; }
      void SetDuration(int value) { m_duration = value; }      

      int GetLastPlayedPosition() const { return m_lastPlayedPosition; }
      void SetLastPlayedPosition(int value) { m_lastPlayedPosition = value; }      

      const std::string& GetStreamURL() const { return m_streamURL; }
      void SetStreamURL(const std::string& value ) { m_streamURL = value; }           

      const std::string& GetChannelName() const { return m_channelName; }
      void SetChannelName(const std::string& value ) { m_channelName = value; }      

      const std::string& GetDirectory() const { return m_directory; }
      void SetDirectory(const std::string& value ) { m_directory = value; }      

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value ) { m_iconPath = value; }    

      bool UpdateFrom(TiXmlElement* recordingNode, const std::string &directory, enigma2::Channels &channels); 
      void UpdateTo(PVR_RECORDING &left, Channels &channels, bool isInRecordingFolder);

    private:
      long TimeStringToSeconds(const std::string &timeString);

      std::string m_recordingId;
      time_t m_startTime;
      int m_duration;
      int m_lastPlayedPosition;
      std::string m_streamURL;
      std::string m_channelName;
      std::string m_directory;
      std::string m_iconPath;
    };
  } //namespace data
} //namespace enigma2