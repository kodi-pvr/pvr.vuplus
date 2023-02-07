/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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

EpgEntryExtractor::EpgEntryExtractor(const std::shared_ptr<enigma2::Settings>& settings)
  : IExtractor(settings)
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + GENRE_DIR, GENRE_ADDON_DATA_BASE_DIR, true);
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + SHOW_INFO_DIR, SHOW_INFO_ADDON_DATA_BASE_DIR, true);

  if (m_settings->GetMapGenreIds())
    m_extractors.emplace_back(new GenreIdMapper(m_settings));
  if (m_settings->GetMapRytecTextGenres())
    m_extractors.emplace_back(new GenreRytecTextMapper(m_settings));
  if (m_settings->GetExtractShowInfo())
    m_extractors.emplace_back(new ShowInfoExtractor(m_settings));

  m_anyExtractorEnabled = false;
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      m_anyExtractorEnabled = true;
  }
}

EpgEntryExtractor::~EpgEntryExtractor() {}

void EpgEntryExtractor::ExtractFromEntry(BaseEntry& entry)
{
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      extractor->ExtractFromEntry(entry);
  }
}

bool EpgEntryExtractor::IsEnabled() { return m_anyExtractorEnabled; }
