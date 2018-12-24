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

#include <regex>
#include <string>
#include <vector>

#include "utilities/DeviceInfo.h"
#include "utilities/DeviceSettings.h"
#include "utilities/Tuner.h"

#include "libXBMC_pvr.h"

namespace enigma2
{

  class Admin
  {
  public:
    void SendPowerstate();
    bool Initialise();
    bool LoadDeviceSettings();
    bool SendAutoTimerSettings();
    bool SendGlobalRecordingStartMarginSetting(int newValue);
    bool SendGlobalRecordingEndMarginSetting(int newValue);
    const utilities::DeviceInfo& GetDeviceInfo() const { return m_deviceInfo; }
    PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed, std::vector<std::string> &locations);
    const std::string& GetServerName() const { return m_deviceInfo.GetServerName(); }
    const std::string& GetEnigmaVersion() const { return m_deviceInfo.GetEnigmaVersion(); }
    const std::string& GetImageVersion() const { return m_deviceInfo.GetImageVersion(); }
    const std::string& GetWebIfVersion() const { return m_deviceInfo.GetWebIfVersion(); }
    unsigned int GetWebIfVersionAsNum() const { return m_deviceInfo.GetWebIfVersionAsNum(); }
    PVR_ERROR GetTunerSignal(PVR_SIGNAL_STATUS &signalStatus);

  private:   
    bool LoadDeviceInfo();
    bool LoadAutoTimerSettings();
    bool LoadRecordingMarginSettings();
    unsigned int ParseWebIfVersion(const std::string &webIfVersion);
    long long GetKbFromString(const std::string &stringInMbGbTb) const;
    void GetTunerDetails(PVR_SIGNAL_STATUS &signalStatus);

    static std::string GetMatchTextFromString(const std::string &text, const std::regex &pattern)
    {
      std::string matchText = "";
      std::smatch match;

      if (regex_match(text, match, pattern))
      {
        if (match.size() == 2)
        {
          std::ssub_match base_sub_match = match[1];
          matchText = base_sub_match.str();  
        }
      }

      return matchText;
    };    

    enigma2::utilities::DeviceInfo m_deviceInfo;
    enigma2::utilities::DeviceSettings m_deviceSettings;
    std::vector<enigma2::utilities::Tuner> m_tuners;
  };
} //namespace enigma2