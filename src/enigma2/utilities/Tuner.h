/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
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
    struct Tuner
    {
      Tuner(int tunerNumber, const std::string& tunerName, const std::string& tunerModel)
        : m_tunerNumber(tunerNumber), m_tunerName(tunerName), m_tunerModel(tunerModel){};

      int m_tunerNumber;
      std::string m_tunerName;
      std::string m_tunerModel;
    };
  } //namespace utilities
} //namespace enigma2
