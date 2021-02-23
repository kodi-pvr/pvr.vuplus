/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Channels.h"
#include "data/RecordingEntry.h"
#include "extract/EpgEntryExtractor.h"

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <kodi/addon-instance/pvr/EDL.h>

namespace enigma2
{
  static int64_t PTS_PER_SECOND = 90000;
  static int CUTS_LAST_PLAYED_TYPE = 3;
  static int E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MIN = 300;
  static int E2_DEVICE_LAST_PLAYED_SYNC_INTERVAL_MAX = 600;

  class IConnectionListener;

  class ATTRIBUTE_HIDDEN Recordings
  {
  public:
    Recordings(IConnectionListener& connectionListener, Channels& channels, enigma2::extract::EpgEntryExtractor& entryExtractor);
    void GetRecordings(std::vector<kodi::addon::PVRRecording>& recordings, bool deleted);
    int GetNumRecordings(bool deleted) const;
    void ClearRecordings(bool deleted);
    void GetRecordingEdl(const std::string& recordingId, std::vector<kodi::addon::PVREDLEntry>& edlEntries) const;
    PVR_ERROR RenameRecording(const kodi::addon::PVRRecording& recording);
    PVR_ERROR SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count);
    PVR_ERROR SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording, int lastplayedposition);
    int GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording);
    PVR_ERROR GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& sizeInBytes);
    const std::string GetRecordingURL(const kodi::addon::PVRRecording& recinfo);
    PVR_ERROR DeleteRecording(const kodi::addon::PVRRecording& recinfo);
    PVR_ERROR UndeleteRecording(const kodi::addon::PVRRecording& recording);
    PVR_ERROR DeleteAllRecordingsFromTrash();
    bool HasRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording);
    int GetRecordingStreamProgramNumber(const kodi::addon::PVRRecording& recording);

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
    bool IsInVirtualRecordingFolder(const data::RecordingEntry& recordingEntry, bool deleted) const;
    bool UpdateRecordingSizeFromMovieDetails(data::RecordingEntry& recordingEntry);

    std::mt19937 m_randomGenerator;
    std::uniform_int_distribution<> m_randomDistribution;

    std::vector<enigma2::data::RecordingEntry> m_recordings;
    std::vector<enigma2::data::RecordingEntry> m_deletedRecordings;
    std::unordered_map<std::string, enigma2::data::RecordingEntry> m_recordingsIdMap;
    std::vector<std::string> m_locations;

    IConnectionListener& m_connectionListener;
    Channels& m_channels;
    enigma2::extract::EpgEntryExtractor& m_entryExtractor;
  };
} //namespace enigma2
