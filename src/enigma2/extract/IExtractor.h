/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../Settings.h"
#include "../data/BaseEntry.h"

#include <regex>

namespace enigma2
{
  namespace extract
  {
    class ATTRIBUTE_HIDDEN IExtractor
    {
    public:
      IExtractor() = default;
      virtual ~IExtractor() = default;
      virtual void ExtractFromEntry(enigma2::data::BaseEntry& entry) = 0;
      virtual bool IsEnabled() = 0;

    protected:
      static std::string GetMatchTextFromString(const std::string& text, const std::regex& pattern)
      {
        std::string matchText = "";
        std::smatch match;

        if (std::regex_match(text, match, pattern))
        {
          if (match.size() == 2)
          {
            std::ssub_match base_sub_match = match[1];
            matchText = base_sub_match.str();
          }
        }

        return matchText;
      };

      static std::string GetMatchedText(const std::string& firstText, const std::string& secondText, const std::regex& pattern)
      {
        std::string matchedText = GetMatchTextFromString(firstText, pattern);

        if (matchedText.empty())
          matchedText = GetMatchTextFromString(secondText, pattern);

        return matchedText;
      }

      static bool Matches(const std::string& text, const std::regex& pattern)
      {
        std::smatch match;

        std::regex_match(text, match, pattern);
        return match.size() != 0;
      };

      static bool Matches(const std::string& firstText, const std::string& secondText, const std::regex& pattern)
      {
        bool matches = Matches(firstText, pattern);

        if (!matches)
          matches = Matches(secondText, pattern);

        return matches;
      };

      enigma2::Settings& m_settings = Settings::GetInstance();
    };
  } //namespace extract
} //namespace enigma2
