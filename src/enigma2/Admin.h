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
#include <vector>

#include "libXBMC_pvr.h"

namespace enigma2
{
  class Admin
  {
  public:
    void SendPowerstate();
    bool GetDeviceInfo();
    PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed, std::vector<std::string> &locations);
    const std::string& GetServerName() const;

  private:   
    unsigned int GetWebIfVersion(std::string versionString);
    long long GetKbFromString(const std::string &stringInMbGbTb) const;

    std::string m_strEnigmaVersion;
    std::string m_strImageVersion;
    std::string m_strWebIfVersion;
    std::string m_strServerName = "Enigma2";    
  };
} //namespace enigma2