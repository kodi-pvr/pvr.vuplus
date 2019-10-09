#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
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

#include "Channels.h"
#include "data/RecordingEntry.h"
#include "extract/EpgEntryExtractor.h"

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <kodi/libXBMC_pvr.h>

namespace enigma2
{
  static int64_t PTS_PER_SECOND = 90000;
  static int CUTS_LAST_PLAYED_TYPE = 3;
  static int E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MIN = 300;
  static int E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MAX = 600;
  static constexpr const char* TRASH_FOLDER = ".Trash";

  class Recordings
  {
  public:
    Recordings(Channels& channels, enigma2::extract::EpgEntryExtractor& entryExtractor);
    void GetRecordings(std::vector<PVR_RECORDING>& recordings, bool deleted);
    int GetNumRecordings(bool deleted) const;
    void ClearRecordings(bool deleted);
    void GetRecordingEdl(const std::string& recordingId, std::vector<PVR_EDL_ENTRY>& edlEntries) const;
    PVR_ERROR RenameRecording(const PVR_RECORDING& recording);
    PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count);
    PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastplayedposition);
    int GetRecordingLastPlayedPosition(const PVR_RECORDING& recording);
    const std::string GetRecordingURL(const PVR_RECORDING& recinfo);
    PVR_ERROR DeleteRecording(const PVR_RECORDING& recinfo);
    PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording);
    PVR_ERROR DeleteAllRecordingsFromTrash();
    bool HasRecordingStreamProgramNumber(const PVR_RECORDING& recording);
    int GetRecordingStreamProgramNumber(const PVR_RECORDING& recording);

    std::vector<std::string>& GetLocations();
    void ClearLocations();

    bool LoadLocations();
    void LoadRecordings(bool deleted);

  private:
    static const std::string FILE_NOT_FOUND_RESPONSE_SUFFIX;

    bool GetRecordingsFromLocation(const std::string recordingFolder, bool deleted, std::vector<data::RecordingEntry>& recordings, std::unordered_map<std::string, enigma2::data::RecordingEntry>& recordingsIdMap);
    data::RecordingEntry GetRecording(const std::string& recordingId) const;
    bool ReadExtaRecordingCutsInfo(const data::RecordingEntry& recordingEntry, std::vector<std::pair<int, int64_t>>& cuts, std::vector<std::string>& tags);
    bool ReadExtraRecordingPlayCountInfo(const data::RecordingEntry& recordingEntry, std::vector<std::string>& tags);
    void SetRecordingNextSyncTime(data::RecordingEntry& recordingEntry, time_t nextSyncTime, std::vector<std::string>& oldTags);
    bool IsInRecordingFolder(const std::string& strRecordingFolder, bool deleted) const;

    std::mt19937 m_randomGenerator;
    std::uniform_int_distribution<> m_randomDistribution;

    std::vector<enigma2::data::RecordingEntry> m_recordings;
    std::vector<enigma2::data::RecordingEntry> m_deletedRecordings;
    std::unordered_map<std::string, enigma2::data::RecordingEntry> m_recordingsIdMap;
    std::vector<std::string> m_locations;

    Channels& m_channels;
    enigma2::extract::EpgEntryExtractor& m_entryExtractor;
  };
} //namespace enigma2