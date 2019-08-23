#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
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

#include "../Settings.h"

#include <string>

namespace enigma2
{
  namespace data
  {
    class BaseEntry
    {
    public:
      const std::string& GetTitle() const { return m_title; }
      void SetTitle(const std::string& value) { m_title = value; }

      const std::string& GetPlotOutline() const { return m_plotOutline; }
      void SetPlotOutline(const std::string& value) { m_plotOutline = value; }

      const std::string& GetPlot() const { return m_plot; }
      void SetPlot(const std::string& value) { m_plot = value; }

      int GetGenreType() const { return m_genreType; }
      void SetGenreType(int value) { m_genreType = value; }

      int GetGenreSubType() const { return m_genreSubType; }
      void SetGenreSubType(int value) { m_genreSubType = value; }

      const std::string& GetGenreDescription() const { return m_genreDescription; }
      void SetGenreDescription(const std::string& value) { m_genreDescription = value; }

      int GetEpisodeNumber() const { return m_episodeNumber; }
      void SetEpisodeNumber(int value) { m_episodeNumber = value; }

      int GetEpisodePartNumber() const { return m_episodePartNumber; }
      void SetEpisodePartNumber(int value) { m_episodePartNumber = value; }

      int GetSeasonNumber() const { return m_seasonNumber; }
      void SetSeasonNumber(int value) { m_seasonNumber = value; }

      int GetYear() const { return m_year; }
      void SetYear(int value) { m_year = value; }

      void ProcessPrependMode(PrependOutline prependOutlineMode);

    protected:
      std::string m_title;
      std::string m_plotOutline;
      std::string m_plot;
      int m_genreType = 0;
      int m_genreSubType = 0;
      std::string m_genreDescription;
      int m_episodeNumber = 0;
      int m_episodePartNumber = 0;
      int m_seasonNumber = 0;
      int m_year = 0;
    };
  } //namespace data
} //namespace enigma2