/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "ChannelGroups.h"
#include "Channels.h"
#include "data/EpgPartialEntry.h"
#include "extract/EpgEntryExtractor.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace enigma2
{
  static const float LAST_SCANNED_INITIAL_EPG_SUCCESS_PERCENT = 0.99f;
  static const int DEFAULT_EPG_MAX_DAYS = 3;

  class IConnectionListener;

  class ATTR_DLL_LOCAL Epg
  {
  public:
    Epg(IConnectionListener& connectionListener, enigma2::Channels& channels, enigma2::extract::EpgEntryExtractor& entryExtractor, int epgMaxPastDays, int epgMaxFutureDays);
    Epg(const enigma2::Epg& epg);

    bool Initialise(enigma2::Channels& channels, enigma2::ChannelGroups& channelGroups);
    bool IsInitialEpgCompleted();
    void TriggerEpgUpdatesForChannels();
    PVR_ERROR GetEPGForChannel(const std::string& serviceReference, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results);
    void SetEPGMaxPastDays(int epgMaxPastDays);
    void SetEPGMaxFutureDays(int epgMaxFutureDays);
    std::string LoadEPGEntryShortDescription(const std::string& serviceReference, unsigned int epgUid);
    data::EpgPartialEntry LoadEPGEntryPartialDetails(const std::string& serviceReference, time_t startTime);
    data::EpgPartialEntry LoadEPGEntryPartialDetails(const std::string& serviceReference, unsigned int epgUid);
    std::string FindServiceReference(const std::string& title, int epgUid, time_t startTime, time_t endTime) const;
    void UpdateTimerEPGFallbackEntries(const std::vector<enigma2::data::EpgEntry>& timerBasedEntries);

  private:
    int TransferTimerBasedEntries(kodi::addon::PVREPGTagsResultSet& results, int channelId);

    enigma2::IConnectionListener& m_connectionListener;
    enigma2::extract::EpgEntryExtractor& m_entryExtractor;

    int m_epgMaxPastDays;
    int m_epgMaxFutureDays;
    long m_epgMaxPastDaysSeconds;
    long m_epgMaxFutureDaysSeconds;

    Channels& m_channels;
    std::unordered_map<std::string, std::shared_ptr<enigma2::data::Channel>> m_channelsMap;

    std::vector<data::EpgEntry> m_timerBasedEntries;

    mutable std::mutex m_mutex;
  };
} //namespace enigma2
