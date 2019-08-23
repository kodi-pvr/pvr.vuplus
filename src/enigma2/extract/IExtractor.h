#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../Settings.h"
#include "../data/BaseEntry.h"

#include <regex>

namespace enigma2
{
  namespace extract
  {
    class IExtractor
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

        if (regex_match(text, match, pattern))
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

      enigma2::Settings& m_settings = Settings::GetInstance();
    };
  } //namespace extract
} //namespace enigma2