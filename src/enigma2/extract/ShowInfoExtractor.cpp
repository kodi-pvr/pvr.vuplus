#include "ShowInfoExtractor.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;

ShowInfoExtractor::ShowInfoExtractor() 
  : IExtractor()
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

void ShowInfoExtractor::ExtractFromEntry(BaseEntry &entry)
{
  for (const auto& patternSet : episodeSeasonPatterns)
  {
    std::string masterText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), patternSet.masterRegex);

    if (!masterText.empty())
    {
      if (patternSet.hasSeasonRegex && entry.GetSeasonNumber() == 0)
      {
        std::string seasonText = GetMatchTextFromString(masterText, patternSet.seasonRegex);
        if (!seasonText.empty())
        {
          entry.SetSeasonNumber(atoi(seasonText.c_str()));
        }
      }

      if (entry.GetEpisodeNumber() == 0)
      {
        std::string episodeText = GetMatchTextFromString(masterText, patternSet.episodeRegex);      
        if (!episodeText.empty())
        {
          entry.SetEpisodeNumber(atoi(episodeText.c_str()));
        }
      }
    }

    //Once we have at least an episode number we are done
    if (entry.GetEpisodeNumber() != 0)
      break;
  }

  for (const auto& pattern : yearPatterns)
  {
    std::string yearText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), pattern);

    if (!yearText.empty() && entry.GetYear() == 0)
    {
      entry.SetYear(atoi(yearText.c_str()));
    }

    if (entry.GetYear() != 0)
      break;
  }
}