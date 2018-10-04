#pragma once

#include "../client.h"
#include "IExtractor.h"
#include <map>
#include <string>
#include <vector>
#include <regex>
#include "libXBMC_addon.h"

namespace vuplus
{

// (S4E37) (S04E37) (S2 Ep3/6) (S2 Ep7)
static const std::string MASTER_SEASON_EPISODE_PATTERN = "^.*\\(?([sS]\\.?[0-9]+ ?[eE][pP]?\\.?[0-9]+/?[0-9]*)\\)?[^]*$";
// (E130) (Ep10) (E7/9) (Ep7/10) (Ep.25)
static const std::string MASTER_EPISODE_PATTERN = "^.*\\(?([eE][pP]?\\.?[0-9]+/?[0-9]*)\\)?[^]*$";
// (2015E22) (2007E3) (2007E3/6)
static const std::string MASTER_YEAR_EPISODE_PATTERN = "^.*\\(?([12][0-9][0-9][0-9][eE][pP]?\\.?[0-9]+\\.?/?[0-9]*)\\)?[^]*$";
// Starts with 4/4 6/6, no prefix
static const std::string MASTER_EPISODE_NO_PREFIX_PATTERN = "^.*([0-9]+/[0-9]+)\\.? +[^]*$";

// Get from matster match, prefixed by S,s,E,e,Ep
static const std::string GET_SEASON_PATTERN = "^.*[sS]\\.?([0-9][0-9]*).*$";
static const std::string GET_EPISODE_PATTERN = "^.*[eE][pP]?\\.?([0-9][0-9]*).*$";
// Get from master match, no prefix
static const std::string GET_EPISODE_NO_PREFIX_PATTERN = "^([0-9]+)/[0-9]+";

// (2018)
static const std::string GET_YEAR_PATTERN = "^.*\\(([12][0-9][0-9][0-9])\\)[^]*$";
// (2018E25)
static const std::string GET_YEAR_EPISODE_PATTERN = "^.*\\(([12][0-9][0-9][0-9])[eE][pP]?\\.?[0-9]+/?[0-9]*\\)[^]*$";

struct EpisodeSeasonPattern
{
  std::regex masterRegex;
  std::regex seasonRegex;
  std::regex episodeRegex;
  bool hasSeasonRegex;

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
};

class ShowInfoExtractor
  : public IExtractor
{
public:
  ShowInfoExtractor(const vuplus::Settings &settings);
  ~ShowInfoExtractor(void);
  void ExtractFromEntry(VuBase &base);

private:
  std::vector<EpisodeSeasonPattern> episodeSeasonPatterns;
  std::vector<std::regex> yearPatterns;
};

} // end vuplus