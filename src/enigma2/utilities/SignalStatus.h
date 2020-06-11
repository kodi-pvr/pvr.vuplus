/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace enigma2
{
  namespace utilities
  {
    static const int POLL_INTERVAL_SECONDS = 10;

    struct SignalStatus
    {
      int m_snrPercentage;
      long m_ber;
      int m_signalStrength;
      std::string m_adapterName;
      std::string m_adapterStatus;
    };
  } //namespace utilities
} //namespace enigma2
