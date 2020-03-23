/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
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