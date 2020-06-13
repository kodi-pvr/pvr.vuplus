/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../data/BaseEntry.h"

#include <string>

namespace enigma2
{
  namespace utilities
  {
    typedef enum UPDATE_STATE
    {
      UPDATE_STATE_NONE,
      UPDATE_STATE_FOUND,
      UPDATE_STATE_UPDATED,
      UPDATE_STATE_NEW
    } UPDATE_STATE;
  } // namespace utilities
} //namespace enigma2
