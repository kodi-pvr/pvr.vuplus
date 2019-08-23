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

#include "GenreIdMapper.h"

#include "../utilities/FileUtils.h"
#include "tinyxml.h"
#include "kodi/libXBMC_pvr.h"
#include "util/XMLUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

GenreIdMapper::GenreIdMapper() : IExtractor()
{
  LoadGenreIdMapFile();
}

GenreIdMapper::~GenreIdMapper(void) {}

void GenreIdMapper::ExtractFromEntry(BaseEntry& entry)
{
  if (entry.GetGenreType() != 0)
  {
    int combinedGenreType = entry.GetGenreType() | entry.GetGenreSubType();

    const int mappedGenreId = LookupGenreIdInMap(combinedGenreType);

    if (mappedGenreId != EPG_EVENT_CONTENTMASK_UNDEFINED)
    {
      entry.SetGenreType(GetGenreTypeFromCombined(mappedGenreId));
      entry.SetGenreSubType(GetGenreSubTypeFromCombined(mappedGenreId));
    }
  }
}

bool GenreIdMapper::IsEnabled()
{
  return Settings::GetInstance().GetMapGenreIds();
}

int GenreIdMapper::GetGenreTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0xF0;
}

int GenreIdMapper::GetGenreSubTypeFromCombined(int combinedGenreType)
{
  return combinedGenreType & 0x0F;
}

int GenreIdMapper::LookupGenreIdInMap(const int combinedGenreType)
{
  int genreType = EPG_EVENT_CONTENTMASK_UNDEFINED;

  auto genreMapSearch = m_genreIdToDvbIdMap.find(combinedGenreType);
  if (genreMapSearch != m_genreIdToDvbIdMap.end())
  {
    genreType = genreMapSearch->second;
  }

  return genreType;
}

void GenreIdMapper::LoadGenreIdMapFile()
{
  if (!LoadIdToIdGenreFile(Settings::GetInstance().GetMapGenreIdsFile(), m_genreIdToDvbIdMap))
    Logger::Log(LEVEL_ERROR, "%s Could not load genre id to dvb id file: %s", __FUNCTION__, Settings::GetInstance().GetMapGenreIdsFile().c_str());
}

bool GenreIdMapper::LoadIdToIdGenreFile(const std::string& xmlFile, std::map<int, int>& map)
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

  TiXmlElement* pElem = hDoc.FirstChildElement("genreIdMappings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <genreIdMappings> element!", __FUNCTION__);
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
    const std::string sourceIdString = pNode->Attribute("sourceId");
    const std::string targetIdString = pNode->GetText();

    int sourceId = strtol(sourceIdString.c_str(), nullptr, 16);
    int targetId = strtol(targetIdString.c_str(), nullptr, 16);

    map.insert({sourceId, targetId});

    Logger::Log(LEVEL_TRACE, "%s Read ID Mapping for: %s, sourceId=%#02X, targetId=%#02X", __FUNCTION__, mapperName.c_str(), sourceId, targetId);
  }

  return true;
}
