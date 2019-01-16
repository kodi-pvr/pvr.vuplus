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
#include "ChannelGroups.h"
#include "data/EpgPartialEntry.h"
#include "extract/EpgEntryExtractor.h"

#include "libXBMC_pvr.h"

namespace enigma2
{
  static const std::string INITIAL_EPG_READY_FILE = "special://userdata/addon_data/pvr.vuplus/initialEPGReady";

  class Epg
  {
  public:
    Epg (enigma2::Channels &channels, enigma2::ChannelGroups &channelGroups, enigma2::extract::EpgEntryExtractor &entryExtractor);

    bool IsInitialEpgCompleted();
    void TriggerEpgUpdatesForChannels();
    PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
    std::string LoadEPGEntryShortDescription(const std::string &serviceReference, unsigned int epgUid);
    data::EpgPartialEntry LoadEPGEntryPartialDetails(const std::string &serviceReference, time_t startTime);

  private:   
    void InitialiseEpgReadyFile();

    bool GetInitialEPGForGroup(std::shared_ptr<enigma2::data::ChannelGroup> group);
    PVR_ERROR GetInitialEPGForChannel(ADDON_HANDLE handle, const std::shared_ptr<enigma2::data::Channel> &channel, time_t iStart, time_t iEnd);
    
    enigma2::Channels &m_channels;
    enigma2::ChannelGroups &m_channelGroups;  
    enigma2::extract::EpgEntryExtractor &m_entryExtractor;

    void *m_writeHandle;
    void *m_readHandle;
    bool m_allChannelsHaveInitialEPG = false;
  };
} //namespace enigma2