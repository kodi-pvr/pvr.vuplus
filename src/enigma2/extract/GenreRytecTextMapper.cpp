/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GenreRytecTextMapper.h"

#include "../utilities/FileUtils.h"

#include "tinyxml.h"
#include "kodi/libXBMC_pvr.h"
#include "util/XMLUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

GenreRytecTextMapper::GenreRytecTextMapper() : IExtractor()
{
  LoadGenreTextMappingFiles();

  for (const auto& genreMapEntry : m_kodiGenreTextToDvbIdMap)
  {
    m_kodiDvbIdToGenreTextMap.insert({genreMapEntry.second, genreMapEntry.first});
  }

  m_genrePattern = std::regex(GENRE_PATTERN);
  m_genreMajorPattern = std::regex(GENRE_MAJOR_PATTERN);
}

GenreRytecTextMapper::~GenreRytecTextMapper(void) {}

void GenreRytecTextMapper::ExtractFromEntry(BaseEntry& entry)
{
  if (entry.GetGenreType() == 0)
  {
    const std::string genreText = GetMatchedText(entry.GetPlotOutline(), entry.GetPlot(), m_genrePattern);

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

bool GenreRytecTextMapper::IsEnabled()
{
  return Settings::GetInstance().GetMapRytecTextGenres();
}

int GenreRytecTextMapper::GetGenreTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0xF0;
}

int GenreRytecTextMapper::GetGenreSubTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0x0F;
}

int GenreRytecTextMapper::GetGenreTypeFromText(const std::string& genreText, const std::string& showName)
{
  int genreType = LookupGenreValueInMaps(genreText);

  if (genreType == EPG_EVENT_CONTENTMASK_UNDEFINED)
  {
    if (m_settings.GetLogMissingGenreMappings())
      Logger::Log(LEVEL_NOTICE, "%s: Tried to find genre text but no value: '%s', show - '%s'", __FUNCTION__, genreText.c_str(), showName.c_str());

    std::string genreMajorText = GetMatchTextFromString(genreText, m_genreMajorPattern);

    if (!genreMajorText.empty())
    {
      genreType = LookupGenreValueInMaps(genreMajorText);

      if (genreType == EPG_EVENT_CONTENTMASK_UNDEFINED && m_settings.GetLogMissingGenreMappings())
        Logger::Log(LEVEL_NOTICE, "%s: Tried to find major genre text but no value: '%s', show - '%s'", __FUNCTION__, genreMajorText.c_str(), showName.c_str());
    }
  }

  return genreType;
}

int GenreRytecTextMapper::LookupGenreValueInMaps(const std::string& genreText)
{
  int genreType = EPG_EVENT_CONTENTMASK_UNDEFINED;

  auto genreMapSearch = m_genreMap.find(genreText);
  if (genreMapSearch != m_genreMap.end())
  {
    genreType = genreMapSearch->second;
  }
  else
  {
    auto kodiGenreMapSearch = m_kodiGenreTextToDvbIdMap.find(genreText);
    if (kodiGenreMapSearch != m_kodiGenreTextToDvbIdMap.end())
    {
      genreType = kodiGenreMapSearch->second;
    }
  }

  return genreType;
}

void GenreRytecTextMapper::LoadGenreTextMappingFiles()
{
  if (!LoadTextToIdGenreFile(GENRE_KODI_DVB_FILEPATH, m_kodiGenreTextToDvbIdMap))
    Logger::Log(LEVEL_ERROR, "%s Could not load text to genre id file: %s", __FUNCTION__, GENRE_KODI_DVB_FILEPATH.c_str());

  if (!LoadTextToIdGenreFile(Settings::GetInstance().GetMapRytecTextGenresFile(), m_genreMap))
    Logger::Log(LEVEL_ERROR, "%s Could not load genre id to dvb id file: %s", __FUNCTION__, Settings::GetInstance().GetMapRytecTextGenresFile().c_str());
}

bool GenreRytecTextMapper::LoadTextToIdGenreFile(const std::string& xmlFile, std::map<std::string, int>& map)
{
  map.clear();

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
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("genreTextMappings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <genreTextMappings> element!", __FUNCTION__);
    return false;
  }

  std::string mapperName;

  if (!XMLUtils::GetString(pElem, "mapperName", mapperName))
    return false;

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("mappings").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <mappings> element", __FUNCTION__);
    return false;
  }

  pNode = pNode->FirstChildElement("mapping");

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <mapping> element", __FUNCTION__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("mapping"))
  {
    const std::string targetIdString = pNode->Attribute("targetId");
    const std::string textMapping = pNode->GetText();

    int targetId = strtol(targetIdString.c_str(), nullptr, 16);

    map.insert({textMapping, targetId});

    Logger::Log(LEVEL_TRACE, "%s Read Text Mapping for: %s, text=%s, targetId=%#02X", __FUNCTION__, mapperName.c_str(), textMapping.c_str(), targetId);
  }

  return true;
}
