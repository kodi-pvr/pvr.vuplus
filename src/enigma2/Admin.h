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

#include "data/Channel.h"
#include "kodi/libXBMC_pvr.h"
#include "utilities/DeviceInfo.h"
#include "utilities/DeviceSettings.h"
#include "utilities/SignalStatus.h"
#include "utilities/StreamStatus.h"
#include "utilities/Tuner.h"

#include <string>
#include <vector>

namespace enigma2
{
  class Admin
  {
  public:
    Admin();

    void SendPowerstate();
    bool Initialise();
    bool LoadDeviceSettings();
    bool SendAutoTimerSettings();
    bool SendGlobalRecordingStartMarginSetting(int newValue);
    bool SendGlobalRecordingEndMarginSetting(int newValue);
    const utilities::DeviceInfo& GetDeviceInfo() const { return m_deviceInfo; }
    PVR_ERROR GetDriveSpace(long long* iTotal, long long* iUsed, std::vector<std::string>& locations);
    const char* GetServerName() const { return m_serverName; }
    const char* GetServerVersion() const { return m_serverVersion; }
    const std::string& GetDeviceName() const { return m_deviceInfo.GetServerName(); }
    const std::string& GetDistroName() const { return m_deviceInfo.GetDistroName(); }
    const std::string& GetEnigmaVersion() const { return m_deviceInfo.GetEnigmaVersion(); }
    const std::string& GetImageVersion() const { return m_deviceInfo.GetImageVersion(); }
    const std::string& GetWebIfVersion() const { return m_deviceInfo.GetWebIfVersion(); }
    unsigned int GetWebIfVersionAsNum() const { return m_deviceInfo.GetWebIfVersionAsNum(); }
    const std::string& GetAddonVersion() const { return m_addonVersion; }
    bool GetTunerSignal(utilities::SignalStatus& signalStatus, const std::shared_ptr<data::Channel>& channel);

  private:
    static void SetCharString(char* target, const std::string value);
    bool LoadDeviceInfo();
    bool LoadAutoTimerSettings();
    bool LoadRecordingMarginSettings();
    unsigned int ParseWebIfVersion(const std::string& webIfVersion);
    long long GetKbFromString(const std::string& stringInMbGbTb) const;
    utilities::StreamStatus GetStreamDetails(const std::shared_ptr<data::Channel>& channel);
    void GetTunerDetails(utilities::SignalStatus& signalStatus, const std::shared_ptr<data::Channel>& channel);

    char m_serverName[256];
    char m_serverVersion[256];
    const std::string m_addonVersion;
    enigma2::utilities::DeviceInfo m_deviceInfo;
    enigma2::utilities::DeviceSettings m_deviceSettings;
    std::vector<enigma2::utilities::Tuner> m_tuners;
  };
} //namespace enigma2