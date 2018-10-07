#include "ShowInfoExtractor.h"

using namespace VUPLUS;
using namespace ADDON;

ShowInfoExtractor::ShowInfoExtractor(const VUPLUS::Settings &settings) 
  : IExtractor(settings)
{
  episodeSeasonPatterns.emplace_back(
      EpisodeSeasonPattern(MASTER_SEASON_EPISODE_PATTERN, 
        GET_SEASON_PATTERN, GET_EPISODE_PATTERN));
  episodeSeasonPatterns.emplace_back(
      EpisodeSeasonPattern(MASTER_EPISODE_PATTERN, 
        GET_EPISODE_PATTERN));
  episodeSeasonPatterns.emplace_back(
      EpisodeSeasonPattern(MASTER_YEAR_EPISODE_PATTERN, 
        GET_EPISODE_PATTERN));
  episodeSeasonPatterns.emplace_back(
      EpisodeSeasonPattern(MASTER_EPISODE_NO_PREFIX_PATTERN, 
        GET_EPISODE_NO_PREFIX_PATTERN));

  yearPatterns.emplace_back(std::regex(GET_YEAR_PATTERN));
  yearPatterns.emplace_back(std::regex(GET_YEAR_EPISODE_PATTERN));
}

ShowInfoExtractor::~ShowInfoExtractor(void)
{
}

void ShowInfoExtractor::ExtractFromEntry(VuBase &entry)
{
  for (const auto& patternSet : episodeSeasonPatterns)
  {
    std::string masterText = GetMatchedText(entry.strPlotOutline, entry.strPlot, patternSet.masterRegex);

    if (!masterText.empty())
    {
      if (patternSet.hasSeasonRegex && entry.seasonNumber == 0)
      {
        std::string seasonText = GetMatchTextFromString(masterText, patternSet.seasonRegex);
        if (!seasonText.empty())
        {
          entry.seasonNumber = atoi(seasonText.c_str());
        }
      }

      if (entry.episodeNumber == 0)
      {
        std::string episodeText = GetMatchTextFromString(masterText, patternSet.episodeRegex);      
        if (!episodeText.empty())
        {
          entry.episodeNumber = atoi(episodeText.c_str());
        }
      }
    }

    //Once we have at least an episode number we are done
    if (entry.episodeNumber != 0)
      break;
  }

  for (const auto& pattern : yearPatterns)
  {
    std::string yearText = GetMatchedText(entry.strPlotOutline, entry.strPlot, pattern);

    if (!yearText.empty() && entry.year == 0)
    {
      entry.year = atoi(yearText.c_str());
    }

    if (entry.year != 0)
      break;
  }
}