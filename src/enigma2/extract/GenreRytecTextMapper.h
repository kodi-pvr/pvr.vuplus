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

#include "IExtractor.h"

#include <map>
#include <regex>
#include <string>

namespace enigma2
{
  namespace extract
  {
    static const std::string GENRE_PATTERN = "^\\[([a-zA-Z /]{3}[a-zA-Z ./]+)\\][^]*";
    static const std::string GENRE_MAJOR_PATTERN = "^([a-zA-Z /]{3,})\\.?.*";
    static const std::string GENRE_RESERVED_IGNORE = "reserved";

    static const std::string GENRE_KODI_DVB_FILEPATH = "special://userdata/addon_data/pvr.vuplus/genres/kodiDvbGenres.xml";

    class GenreRytecTextMapper : public IExtractor
    {
    public:
      GenreRytecTextMapper();
      ~GenreRytecTextMapper();

      void ExtractFromEntry(enigma2::data::BaseEntry& entry);
      bool IsEnabled();

    private:
      static int GetGenreTypeFromCombined(int combinedGenreType);
      static int GetGenreSubTypeFromCombined(int combinedGenreType);

      int GetGenreTypeFromText(const std::string& genreText, const std::string& showName);
      int LookupGenreValueInMaps(const std::string& genreText);

      void LoadGenreTextMappingFiles();
      bool LoadTextToIdGenreFile(const std::string& xmlFile, std::map<std::string, int>& map);
      void CreateGenreAddonDataDirectories();

      std::regex m_genrePattern;
      std::regex m_genreMajorPattern;
      std::map<std::string, int> m_kodiGenreTextToDvbIdMap;
      std::map<int, std::string> m_kodiDvbIdToGenreTextMap;
      std::map<std::string, int> m_genreMap;
    };
  } //namespace extract
} //namespace enigma2