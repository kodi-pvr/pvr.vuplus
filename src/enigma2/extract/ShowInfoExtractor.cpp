#include "ShowInfoExtractor.h"

#include "../utilities/FileUtils.h"

#include "tinyxml.h"
#include "util/XMLUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

ShowInfoExtractor::ShowInfoExtractor() 
  : IExtractor()
{
  if (!LoadShowInfoPatternsFile(Settings::GetInstance().GetExtractShowInfoFile(), m_episodeSeasonPatterns, m_yearPatterns))
    Logger::Log(LEVEL_ERROR, "%s Could not load show info patterns file: %s", __FUNCTION__, Settings::GetInstance().GetExtractShowInfoFile().c_str());
}

ShowInfoExtractor::~ShowInfoExtractor(void)
{
}

void ShowInfoExtractor::ExtractFromEntry(BaseEntry &entry)
{
  for (const auto& patternSet : m_episodeSeasonPatterns)
  {
    const std::string masterText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), patternSet.masterRegex);

    if (!masterText.empty())
    {
      if (patternSet.hasSeasonRegex && entry.GetSeasonNumber() == 0)
      {
        const std::string seasonText = GetMatchTextFromString(masterText, patternSet.seasonRegex);
        if (!seasonText.empty())
        {
          entry.SetSeasonNumber(atoi(seasonText.c_str()));
        }
      }

      if (entry.GetEpisodeNumber() == 0)
      {
        const std::string episodeText = GetMatchTextFromString(masterText, patternSet.episodeRegex);      
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

  for (const auto& pattern : m_yearPatterns)
  {
    const std::string yearText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), pattern);

    if (!yearText.empty() && entry.GetYear() == 0)
    {
      entry.SetYear(atoi(yearText.c_str()));
    }

    if (entry.GetYear() != 0)
      break;
  }
}

bool ShowInfoExtractor::IsEnabled()
{
  return Settings::GetInstance().GetExtractShowInfo();
}

bool ShowInfoExtractor::LoadShowInfoPatternsFile(const std::string &xmlFile, std::vector<EpisodeSeasonPattern> &episodeSeasonPatterns, std::vector<std::regex> yearPatterns)
{
  episodeSeasonPatterns.clear();
  yearPatterns.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __FUNCTION__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __FUNCTION__, xmlFile.c_str());

  const std::string fileContents = FileUtils::ReadXmlFileToString(xmlFile);

  if (fileContents.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s No Content in XML file: %s", __FUNCTION__, xmlFile.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(fileContents.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("showInfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <showInfo> element!", __FUNCTION__);
    return false;
  }

  std::string name;

  if (!XMLUtils::GetString(pElem, "name", name)) 
    return false;

  hRoot=TiXmlHandle(pElem);

  //First we do the seasonEpisodes
  TiXmlElement* pNode = hRoot.FirstChildElement("seasonEpisodes").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <seasonEpisodes> element", __FUNCTION__);
    return false;
  }    

  pNode = pNode->FirstChildElement("seasonEpisode");

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <seasonEpisode> element", __FUNCTION__);
    return false;
  }    

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("seasonEpisode"))
  {
    TiXmlElement* childNode = pNode->FirstChildElement("master");

    if (childNode)
    {
      const std::string masterPattern = childNode->Attribute("pattern");

      childNode = pNode->FirstChildElement("episode");

      if (childNode)
      {
        const std::string episodePattern = childNode->Attribute("pattern");

        childNode = pNode->FirstChildElement("season");
        if (childNode != nullptr)
        {
          const std::string seasonPattern = childNode->Attribute("pattern");

          if (!masterPattern.empty() && !seasonPattern.empty() && !episodePattern.empty())
          {
            episodeSeasonPatterns.emplace_back(EpisodeSeasonPattern(masterPattern, seasonPattern, episodePattern));

            Logger::Log(LEVEL_DEBUG, "%s Adding seasonEpisode pattern: %s, master: %s, season: %s, episode: %s", __FUNCTION__, name.c_str(), masterPattern.c_str(), seasonPattern.c_str(), episodePattern.c_str());
          }
        }
        else
        {
          if (!masterPattern.empty() && !episodePattern.empty())
          {
            episodeSeasonPatterns.emplace_back(EpisodeSeasonPattern(masterPattern, episodePattern));

            Logger::Log(LEVEL_DEBUG, "%s Adding episode pattern from: %s, master: %s, episode: %s", __FUNCTION__, name.c_str(), masterPattern.c_str(), episodePattern.c_str());
          }
        }
      }
      else
      {
        Logger::Log(LEVEL_ERROR, "%s Could find <episode> element, skipping pattern from: %s", __FUNCTION__, name.c_str());
      }
    }
    else
    {
      Logger::Log(LEVEL_ERROR, "%s Could find <master> element, skipping pattern from: %s", __FUNCTION__, name.c_str());
    }
  }

  //Now we do the years
  pNode = hRoot.FirstChildElement("years").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <years> element", __FUNCTION__);
    return false;
  }    

  pNode = pNode->FirstChildElement("year");

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <year> element", __FUNCTION__);
    return false;
  }    

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("year"))
  {
    const std::string yearPattern = pNode->Attribute("pattern");

    yearPatterns.emplace_back(std::regex(yearPattern));

    Logger::Log(LEVEL_DEBUG, "%s Adding year pattern from: %s, pattern: %s", __FUNCTION__, name.c_str(), yearPattern.c_str());
  }

  return true;    
}
