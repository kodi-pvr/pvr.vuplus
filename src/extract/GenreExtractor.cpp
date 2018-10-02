#include "GenreExtractor.h"
#include <regex>

using namespace vuplus;
using namespace ADDON;

GenreExtractor::GenreExtractor()
{
  for (std::map<int, std::string>::const_iterator it = kodiKeyToGenreMap.begin(); it != kodiKeyToGenreMap.end(); it++)
  {
    kodiGenreToKeyMap.insert({it->second, it->first});
  }

  genrePattern = std::regex(GENRE_PATTERN);
  genreMajorPattern = std::regex(GENRE_MAJOR_PATTERN);
}

GenreExtractor::~GenreExtractor(void)
{
}

void GenreExtractor::ExtractFromEntry(VuBase &entry)
{
  std::string genreText = GetMatchedText(entry.strPlotOutline, entry.strPlot, genrePattern);

  if (!genreText.empty() && genreText != GENRE_RESERVED_IGNORE)
  {
    int combinedGenreType = GetGenreTypeFromText(genreText, entry.strTitle);

    if (combinedGenreType == EPG_EVENT_CONTENTMASK_UNDEFINED)
    {
      if (g_bLogMissingGenreMappings)
        XBMC->Log(LOG_NOTICE, "%s: Could not lookup genre using genre description string instead:'%s'", __FUNCTION__, genreText.c_str());

      entry.genreType = EPG_GENRE_USE_STRING;
      entry.genreDescription = genreText;
    }
    else
    {
      entry.genreType = GetGenreTypeFromCombined(combinedGenreType);
      entry.genreSubType = GetGenreSubTypeFromCombined(combinedGenreType);
    }
  }
}

int GenreExtractor::GetGenreTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0xF0;
}

int GenreExtractor::GetGenreSubTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0x0F;
}

int GenreExtractor::GetGenreTypeFromText(const std::string &genreText, const std::string &showName)
{
  int genreType = LookupGenreValueInMaps(genreText);

  if (genreType == EPG_EVENT_CONTENTMASK_UNDEFINED) 
  {
    if (g_bLogMissingGenreMappings)
      XBMC->Log(LOG_NOTICE, "%s: Tried to find genre text but no value: '%s', show - '%s'", __FUNCTION__, genreText.c_str(), showName.c_str());

    std::string genreMajorText = GetMatchTextFromString(genreText, genreMajorPattern);
    
    if (!genreMajorText.empty())
    {
      genreType = LookupGenreValueInMaps(genreMajorText);

      if (genreType == EPG_EVENT_CONTENTMASK_UNDEFINED && g_bLogMissingGenreMappings)
        XBMC->Log(LOG_NOTICE, "%s: Tried to find major genre text but no value: '%s', show - '%s'", __FUNCTION__, genreMajorText.c_str(), showName.c_str());  
    }
  } 

  return genreType;
}

int GenreExtractor::LookupGenreValueInMaps(const std::string &genreText)
{
  int genreType = EPG_EVENT_CONTENTMASK_UNDEFINED;

  auto genreMapSearch = genreMap.find(genreText);
  if (genreMapSearch != genreMap.end()) 
  {
    genreType = genreMapSearch->second;
  } 
  else
  {
    auto kodiGenreMapSearch = kodiGenreToKeyMap.find(genreText);
    if (kodiGenreMapSearch != kodiGenreToKeyMap.end()) 
    {
      genreType = kodiGenreMapSearch->second;
    }     
  }

  return genreType;
}