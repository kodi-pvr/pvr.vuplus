/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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

    class ATTR_DLL_LOCAL GenreRytecTextMapper : public IExtractor
    {
    public:
      GenreRytecTextMapper(const std::shared_ptr<enigma2::InstanceSettings>& settings);
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
