/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <regex>

namespace enigma2
{
  namespace extract
  {
    struct EpisodeSeasonPattern
    {
      EpisodeSeasonPattern(const std::string& masterPattern, const std::string& seasonPattern, const std::string& episodePattern)
      {
        m_masterRegex = std::regex(masterPattern);
        m_seasonRegex = std::regex(seasonPattern);
        m_episodeRegex = std::regex(episodePattern);
        m_hasSeasonRegex = true;
      }

      EpisodeSeasonPattern(const std::string& masterPattern, const std::string& episodePattern)
      {
        m_masterRegex = std::regex(masterPattern);
        m_episodeRegex = std::regex(episodePattern);
        m_hasSeasonRegex = false;
      }

      std::regex m_masterRegex;
      std::regex m_seasonRegex;
      std::regex m_episodeRegex;
      bool m_hasSeasonRegex;
    };
  } //namespace extract
} //namespace enigma2
