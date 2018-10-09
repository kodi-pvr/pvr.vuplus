#pragma once 
/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include <string>
#include <ctime>
#include <type_traits>

#include "Timer.h"

#include "libXBMC_pvr.h"

namespace enigma2
{
  namespace data
  {
    class AutoTimer
      : public Timer
    {
    public:
      enum DeDup
        : unsigned int  // same type as PVR_TIMER_TYPE.iPreventDuplicateEpisodes
      {
        DISABLED                         = 0,
        CHECK_TITLE                      = 1,
        CHECK_TITLE_AND_SHORT_DESC       = 2,
        CHECK_TITLE_AND_ALL_DESCS        = 3
      };

      AutoTimer() = default;

      const std::string& GetSearchPhrase() const { return m_searchPhrase; }
      void SetSearchPhrase(const std::string& value ) { m_searchPhrase = value; }

      const std::string& GetEncoding() const { return m_encoding; }
      void SetEncoding(const std::string& value ) { m_encoding = value; }

      const std::string& GetSearchCase() const { return m_searchCase; }
      void SetSearchCase(const std::string& value ) { m_searchCase = value; }

      const std::string& GetSearchType() const { return m_searchType; }
      void SetSearchType(const std::string& value ) { m_searchType = value; }

      unsigned int GetBackendId() const { return m_backendId; }
      void SetBackendId(unsigned int value) { m_backendId = value; }      
      
      bool GetSearchFulltext() const { return m_searchFulltext; }
      void SetSearchFulltext(bool value) { m_searchFulltext = value; }

      bool GetStartAnyTime() const { return m_startAnyTime; }
      void SetStartAnyTime(bool value) { m_startAnyTime = value; }

      bool GetEndAnyTime() const { return m_endAnyTime; }
      void SetEndAnyTime(bool value) { m_endAnyTime = value; }

      bool GetAnyChannel() const { return m_anyChannel; }
      void SetAnyChannel(bool value) { m_anyChannel = value; }

      std::underlying_type<DeDup>::type GetDeDup() const { return m_deDup; }
      void SetDeDup(const std::underlying_type<DeDup>::type value ) { m_deDup = value; }

      bool Like(const AutoTimer &right) const;
      bool operator==(const AutoTimer &right) const;
      void UpdateFrom(const AutoTimer &right);
      void UpdateTo(PVR_TIMER &right) const;

    private:
      std::string m_searchPhrase;
      std::string m_encoding;
      std::string m_searchCase;
      std::string m_searchType;
      unsigned int m_backendId;
      bool m_searchFulltext = false;
      bool m_startAnyTime = false;
      bool m_endAnyTime   = false;
      bool m_anyChannel = false;
      std::underlying_type<DeDup>::type m_deDup = DeDup::DISABLED;
    };
  } //namespace data
} //namespace enigma2