#pragma once

#include <regex>

namespace enigma2
{
  namespace extract
  {
    struct EpisodeSeasonPattern
    {
      EpisodeSeasonPattern(const std::string &masterPattern, const std::string &seasonPattern, const std::string &episodePattern)
      {
        masterRegex = std::regex(masterPattern);
        seasonRegex = std::regex(seasonPattern);
        episodeRegex = std::regex(episodePattern);
        hasSeasonRegex = true;
      }

      EpisodeSeasonPattern(const std::string &masterPattern, const std::string &episodePattern)
      {
        masterRegex = std::regex(masterPattern);
        episodeRegex = std::regex(episodePattern);
        hasSeasonRegex = false;
      }

      std::regex masterRegex;
      std::regex seasonRegex;
      std::regex episodeRegex;
      bool hasSeasonRegex;
    };
  } //namespace extract
} //namespace enigma2