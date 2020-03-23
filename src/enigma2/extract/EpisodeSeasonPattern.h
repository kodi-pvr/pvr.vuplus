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