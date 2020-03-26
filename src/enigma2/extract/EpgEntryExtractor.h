/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IExtractor.h"

#include <memory>
#include <string>
#include <vector>

namespace enigma2
{
  namespace extract
  {
    static const std::string GENRE_DIR = "/genres";
    static const std::string GENRE_ADDON_DATA_BASE_DIR = ADDON_DATA_BASE_DIR + GENRE_DIR;
    static const std::string SHOW_INFO_DIR = "/showInfo";
    static const std::string SHOW_INFO_ADDON_DATA_BASE_DIR = ADDON_DATA_BASE_DIR + SHOW_INFO_DIR;

    class EpgEntryExtractor : public IExtractor
    {
    public:
      EpgEntryExtractor();
      ~EpgEntryExtractor();

      void ExtractFromEntry(enigma2::data::BaseEntry& entry);
      bool IsEnabled();

    private:
      std::vector<std::unique_ptr<IExtractor>> m_extractors;
      bool m_anyExtractorEnabled;
    };
  } //namespace extract
} //namespace enigma2