/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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