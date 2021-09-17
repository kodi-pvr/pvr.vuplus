/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../Settings.h"

#include <string>

#include <kodi/addon-instance/pvr/EPG.h>

namespace enigma2
{
  namespace data
  {
    class ATTRIBUTE_HIDDEN BaseEntry
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

      bool IsNew() const { return m_new; }
      void SetNew(int value) { m_new = value; }

      bool IsLive() const { return m_live; }
      void SetLive(int value) { m_live = value; }

      bool IsPremiere() const { return m_premiere; }
      void SetPremiere(int value) { m_premiere = value; }

      bool IsFinale() const { return m_finale; }
      void SetFinale(int value) { m_finale = value; }

      void ProcessPrependMode(PrependOutline prependOutlineMode);

    protected:
      std::string m_title;
      std::string m_plotOutline;
      std::string m_plot;
      int m_genreType = 0;
      int m_genreSubType = 0;
      std::string m_genreDescription;
      int m_episodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
      int m_episodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
      int m_seasonNumber = EPG_TAG_INVALID_SERIES_EPISODE;
      int m_year = 0;
      bool m_new = false;
      bool m_live = false;
      bool m_premiere = false;
      bool m_finale = false;
    };
  } //namespace data
} //namespace enigma2
