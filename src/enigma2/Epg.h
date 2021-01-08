/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "ChannelGroups.h"
#include "Channels.h"
#include "data/EpgChannel.h"
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

  class ATTRIBUTE_HIDDEN Epg
  {
  public:
    Epg(IConnectionListener& connectionListener, enigma2::extract::EpgEntryExtractor& entryExtractor, int epgMaxPastDays, int epgMaxFutureDays);
    Epg(const enigma2::Epg& epg);

    bool Initialise(enigma2::Channels& channels, enigma2::ChannelGroups& channelGroups);
    bool IsInitialEpgCompleted();
    void TriggerEpgUpdatesForChannels();
    void MarkChannelAsInitialEpgRead(const std::string& serviceReference);
    PVR_ERROR GetEPGForChannel(const std::string& serviceReference, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results);
    void SetEPGMaxPastDays(int epgMaxPastDays);
    void SetEPGMaxFutureDays(int epgMaxFutureDays);
    std::string LoadEPGEntryShortDescription(const std::string& serviceReference, unsigned int epgUid);
    data::EpgPartialEntry LoadEPGEntryPartialDetails(const std::string& serviceReference, time_t startTime);
    data::EpgPartialEntry LoadEPGEntryPartialDetails(const std::string& serviceReference, unsigned int epgUid);
    std::string FindServiceReference(const std::string& title, int epgUid, time_t startTime, time_t endTime) const;
    void UpdateTimerEPGFallbackEntries(const std::vector<enigma2::data::EpgEntry>& timerBasedEntries);

  private:
    PVR_ERROR TransferInitialEPGForChannel(kodi::addon::PVREPGTagsResultSet& results, const std::shared_ptr<data::EpgChannel>& epgChannel, time_t iStart, time_t iEnd);
    std::shared_ptr<data::EpgChannel> GetEpgChannel(const std::string& serviceReference);
    bool LoadInitialEPGForGroup(const std::shared_ptr<enigma2::data::ChannelGroup> group);
    bool ChannelNeedsInitialEpg(const std::string& serviceReference);
    bool InitialEpgLoadedForChannel(const std::string& serviceReference);
    bool InitialEpgReadForChannel(const std::string& serviceReference);
    std::shared_ptr<data::EpgChannel> GetEpgChannelNeedingInitialEpg(const std::string& serviceReference);
    int TransferTimerBasedEntries(kodi::addon::PVREPGTagsResultSet& results, int epgChannelId);

    enigma2::IConnectionListener& m_connectionListener;
    enigma2::extract::EpgEntryExtractor& m_entryExtractor;

    bool m_initialEpgReady = false;
    int m_epgMaxPastDays;
    int m_epgMaxFutureDays;
    long m_epgMaxPastDaysSeconds;
    long m_epgMaxFutureDaysSeconds;

    std::vector<std::shared_ptr<data::EpgChannel>> m_epgChannels;
    std::map<std::string, std::shared_ptr<data::EpgChannel>> m_epgChannelsMap;
    std::map<std::string, std::shared_ptr<data::EpgChannel>> m_readInitialEpgChannelsMap;
    std::map<std::string, std::shared_ptr<data::EpgChannel>> m_needsInitialEpgChannelsMap;

    std::vector<data::EpgEntry> m_timerBasedEntries;

    mutable std::mutex m_mutex;
  };
} //namespace enigma2
