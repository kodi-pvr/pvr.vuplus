/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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
      PREMIERE,
      FINALE
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