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

#include <regex>

namespace enigma2
{
  namespace utilities
  {
    class DeviceInfo
    {
    public:
      DeviceInfo() = default;
      DeviceInfo(const std::string& serverName, const std::string& enigmaVersion, const std::string& imageVersion, const std::string& distroName,
        const std::string& webIfVersion, unsigned int webIfVersionAsNum)
        : m_serverName(serverName), m_enigmaVersion(enigmaVersion), m_imageVersion(imageVersion), m_distroName(distroName),
          m_webIfVersion(webIfVersion), m_webIfVersionAsNum(webIfVersionAsNum) {};

      const std::string& GetServerName() const { return m_serverName; }
      const std::string& GetEnigmaVersion() const { return m_enigmaVersion; }
      const std::string& GetImageVersion() const { return m_imageVersion; }
      const std::string& GetDistroName() const { return m_distroName; }
      const std::string& GetWebIfVersion() const { return m_webIfVersion; }
      unsigned int GetWebIfVersionAsNum() const { return m_webIfVersionAsNum; }

    private:
      std::string m_serverName = "Enigma2";
      std::string m_enigmaVersion;
      std::string m_imageVersion;
      std::string m_distroName;
      std::string m_webIfVersion;
      unsigned int m_webIfVersionAsNum;
    };
  } //namespace utilities
} //namespace enigma2