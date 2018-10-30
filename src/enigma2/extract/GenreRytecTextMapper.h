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

    class GenreRytecTextMapper
      : public IExtractor
    {
    public:
      GenreRytecTextMapper();
      ~GenreRytecTextMapper();

      void ExtractFromEntry(enigma2::data::BaseEntry &entry);
      bool IsEnabled();

    private:
      static int GetGenreTypeFromCombined(int combinedGenreType);
      static int GetGenreSubTypeFromCombined(int combinedGenreType);

      int GetGenreTypeFromText(const std::string &genreText, const std::string &showName);
      int LookupGenreValueInMaps(const std::string &genreText);

      void LoadGenreTextMappingFiles();
      bool LoadTextToIdGenreFile(const std::string &xmlFile, std::map<std::string, int> &map);
      void CreateGenreAddonDataDirectories();

      std::regex genrePattern;
      std::regex genreMajorPattern;
      std::map<std::string, int> kodiGenreTextToDvbIdMap;
      std::map<int, std::string> kodiDvbIdToGenreTextMap;
      std::map<std::string, int> genreMap;
    };
  } //namespace extract
} //namespace enigma2