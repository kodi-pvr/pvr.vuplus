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

#include "BaseEntry.h"

#include <string>

namespace enigma2
{
  namespace data
  {
    class EpgPartialEntry : public BaseEntry
    {
    public:
      unsigned int GetEpgUid() const { return m_epgUid; }
      void SetEpgUid(unsigned int value) { m_epgUid = value; }

      bool EntryFound() const { return m_epgUid != 0; };

    private:
      unsigned int m_epgUid = 0;
    };
  } //namespace data
} //namespace enigma2