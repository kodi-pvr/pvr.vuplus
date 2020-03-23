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

#include "EpisodeSeasonPattern.h"
#include "IExtractor.h"

#include <map>
#include <regex>
#include <string>
#include <vector>

namespace enigma2
{
  namespace extract
  {
    enum class TextPropertyType
      : int // same type as addon settings
    {
      NEW = 0,
      LIVE,
      PREMIERE
    };

    static const std::map<std::string, TextPropertyType> m_textPropetyTypeMap{{"new", TextPropertyType::NEW}, {"live", TextPropertyType::LIVE}, {"premiere", TextPropertyType::PREMIERE}};

    // (S4E37) (S04E37) (S2 Ep3/6) (S2 Ep7)
    static const std::string MASTER_SEASON_EPISODE_PATTERN = "^.*\\(?([sS]\\.?[0-9]+ ?[eE][pP]?\\.?[0-9]+/?[0-9]*)\\)?[^]*$";
    // (E130) (Ep10) (E7/9) (Ep7/10) (Ep.25)
    static const std::string MASTER_EPISODE_PATTERN = "^.*\\(?([eE][pP]?\\.?[0-9]+/?[0-9]*)\\)?[^]*$";
    // (2015E22) (2007E3) (2007E3/6)
    static const std::string MASTER_YEAR_EPISODE_PATTERN = "^.*\\(?([12][0-9][0-9][0-9][eE][pP]?\\.?[0-9]+\\.?/?[0-9]*)\\)?[^]*$";
    // Starts with 2/4 6/6, no prefix
    static const std::string MASTER_EPISODE_NO_PREFIX_PATTERN = "^.*([0-9]+/[0-9]+)\\.? +[^]*$";

    // Get from matster match, prefixed by S,s,E,e,Ep
    static const std::string GET_SEASON_PATTERN = "^.*[sS]\\.?([0-9][0-9]*).*$";
    static const std::string GET_EPISODE_PATTERN = "^.*[eE][pP]?\\.?([0-9][0-9]*).*$";
    // Get from master match, no prefix
    static const std::string GET_EPISODE_NO_PREFIX_PATTERN = "^([0-9]+)/[0-9]+";

    // (2018)
    static const std::string GET_YEAR_PATTERN = "^.*\\(([12][0-9][0-9][0-9])\\)[^]*$";
    // (2018E25)
    static const std::string GET_YEAR_EPISODE_PATTERN = "^.*\\(([12][0-9][0-9][0-9])[eE][pP]?\\.?[0-9]+/?[0-9]*\\)[^]*$";

    class ShowInfoExtractor : public IExtractor
    {
    public:
      ShowInfoExtractor();
      ~ShowInfoExtractor();

      void ExtractFromEntry(enigma2::data::BaseEntry& entry);
      bool IsEnabled();

    private:
      bool LoadShowInfoPatternsFile(const std::string& xmlFile, std::vector<EpisodeSeasonPattern>& episodeSeasonPatterns, std::vector<std::regex>& yearPatterns, std::vector<std::pair<TextPropertyType, std::regex>>& titleTextPatterns, std::vector<std::pair<TextPropertyType, std::regex>>& descTextPatterns);

      std::vector<EpisodeSeasonPattern> m_episodeSeasonPatterns;
      std::vector<std::regex> m_yearPatterns;
      std::vector<std::pair<TextPropertyType, std::regex>> m_titleTextPatterns;
      std::vector<std::pair<TextPropertyType, std::regex>> m_descriptionTextPatterns;
    };
  } //namespace extract
} //namespace enigma2