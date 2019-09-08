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

#include "EpgEntryExtractor.h"

#include "../utilities/FileUtils.h"
#include "GenreIdMapper.h"
#include "GenreRytecTextMapper.h"
#include "ShowInfoExtractor.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

EpgEntryExtractor::EpgEntryExtractor()
  : IExtractor()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + GENRE_DIR, GENRE_ADDON_DATA_BASE_DIR, true);
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + SHOW_INFO_DIR, SHOW_INFO_ADDON_DATA_BASE_DIR, true);

  if (Settings::GetInstance().GetMapGenreIds())
    m_extractors.emplace_back(new GenreIdMapper());
  if (Settings::GetInstance().GetMapRytecTextGenres())
    m_extractors.emplace_back(new GenreRytecTextMapper());
  if (Settings::GetInstance().GetExtractShowInfo())
    m_extractors.emplace_back(new ShowInfoExtractor());

  m_anyExtractorEnabled = false;
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      m_anyExtractorEnabled = true;
  }
}

EpgEntryExtractor::~EpgEntryExtractor(void) {}

void EpgEntryExtractor::ExtractFromEntry(BaseEntry& entry)
{
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      extractor->ExtractFromEntry(entry);
  }
}

bool EpgEntryExtractor::IsEnabled() { return m_anyExtractorEnabled; }