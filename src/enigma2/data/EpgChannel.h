/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "BaseChannel.h"
#include "EpgEntry.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>
#include <tinyxml.h>

namespace enigma2
{
  namespace data
  {
    class EpgEntry;

    class EpgChannel : public BaseChannel
    {
    public:
      EpgChannel() = default;
      EpgChannel(const EpgChannel& e) : BaseChannel(e){};
      ~EpgChannel() = default;

      bool RequiresInitialEpg() const { return m_requiresInitialEpg; }
      void SetRequiresInitialEpg(bool value) { m_requiresInitialEpg = value; }

      std::vector<EpgEntry>& GetInitialEPG() { return m_initialEPG; }

    private:
      bool m_requiresInitialEpg = true;

      std::vector<EpgEntry> m_initialEPG;
    };
  } //namespace data
} //namespace enigma2