/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
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