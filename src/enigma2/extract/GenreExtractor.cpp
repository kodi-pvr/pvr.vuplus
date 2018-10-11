#include "GenreExtractor.h"

#include "libXBMC_pvr.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

GenreExtractor::GenreExtractor() 
  : IExtractor()
{
  for (const auto& genreMapEntry : kodiKeyToGenreMap)
  {
    kodiGenreToKeyMap.insert({genreMapEntry.second, genreMapEntry.first});
  }

  genrePattern = std::regex(GENRE_PATTERN);
  genreMajorPattern = std::regex(GENRE_MAJOR_PATTERN);
}

GenreExtractor::~GenreExtractor(void)
{
}

void GenreExtractor::ExtractFromEntry(BaseEntry &entry)
{
  if (entry.GetGenreType() == 0)
  {
    const std::string genreText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), genrePattern);

    if (!genreText.empty() && genreText != GENRE_RESERVED_IGNORE)
    {
      int combinedGenreType = GetGenreTypeFromText(genreText, entry.GetTitle());

      if (combinedGenreType == EPG_EVENT_CONTENTMASK_UNDEFINED)
      {
        if (m_settings.GetLogMissingGenreMappings())
          Logger::Log(LEVEL_NOTICE, "%s: Could not lookup genre using genre description string instead:'%s'", __FUNCTION__, genreText.c_str());

        entry.SetGenreType(EPG_GENRE_USE_STRING);
        entry.SetGenreDescription(genreText);
      }
      else
      {
        entry.SetGenreType(GetGenreTypeFromCombined(combinedGenreType));
        entry.SetGenreSubType(GetGenreSubTypeFromCombined(combinedGenreType));
      }
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
    if (m_settings.GetLogMissingGenreMappings())
      Logger::Log(LEVEL_NOTICE, "%s: Tried to find genre text but no value: '%s', show - '%s'", __FUNCTION__, genreText.c_str(), showName.c_str());

    std::string genreMajorText = GetMatchTextFromString(genreText, genreMajorPattern);
    
    if (!genreMajorText.empty())
    {
      genreType = LookupGenreValueInMaps(genreMajorText);

      if (genreType == EPG_EVENT_CONTENTMASK_UNDEFINED && m_settings.GetLogMissingGenreMappings())
        Logger::Log(LEVEL_NOTICE, "%s: Tried to find major genre text but no value: '%s', show - '%s'", __FUNCTION__, genreMajorText.c_str(), showName.c_str());  
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