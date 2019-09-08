#pragma once
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

#include "client.h"

#include <string>

namespace enigma2
{
  class LocalizedString
  {
  public:
    explicit LocalizedString(int id)
    {
      Load(id);
    }

    bool Load(int id)
    {
      char* str;
      if ((str = XBMC->GetLocalizedString(id)))
      {
        m_localizedString = str;
        XBMC->FreeString(str);
        return true;
      }

      m_localizedString = "";
      return false;
    }

    std::string Get()
    {
      return m_localizedString;
    }

    operator std::string()
    {
      return Get();
    }

    const char* c_str()
    {
      return m_localizedString.c_str();
    }

  private:
    LocalizedString() = delete;
    LocalizedString(const LocalizedString&) = delete;
    LocalizedString& operator=(const LocalizedString&) = delete;

    std::string m_localizedString;
  };
} //namespace enigma2
