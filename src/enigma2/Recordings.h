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
#include <vector>

#include "Channels.h"
#include "data/RecordingEntry.h"
#include "extract/EpgEntryExtractor.h"

#include "libXBMC_pvr.h"

namespace enigma2
{
  class Recordings
  {
  public:
    Recordings(Channels &channels, enigma2::extract::EpgEntryExtractor &entryExtractor)
      : m_channels(channels), m_entryExtractor(entryExtractor) {};
    void GetRecordings(std::vector<PVR_RECORDING> &recordings);
    int GetNumRecordings() const;
    void ClearRecordings();
    bool IsInRecordingFolder(const std::string &strRecordingFolder) const;
    const std::string GetRecordingURL(const PVR_RECORDING &recinfo);
    PVR_ERROR DeleteRecording(const PVR_RECORDING &recinfo);

    std::vector<std::string>& GetLocations();
    void ClearLocations();

    bool LoadLocations();
    void LoadRecordings();

  private:   
    bool GetRecordingsFromLocation(std::string recordingFolder);

    std::vector<enigma2::data::RecordingEntry> m_recordings;
    std::vector<std::string> m_locations;

    Channels &m_channels;
    enigma2::extract::EpgEntryExtractor &m_entryExtractor;
  };
} //namespace enigma2