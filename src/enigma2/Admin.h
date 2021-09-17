/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"
#include "utilities/DeviceInfo.h"
#include "utilities/DeviceSettings.h"
#include "utilities/SignalStatus.h"
#include "utilities/StreamStatus.h"
#include "utilities/Tuner.h"

#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/General.h>

namespace enigma2
{
  class ATTRIBUTE_HIDDEN Admin
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
    PVR_ERROR GetDriveSpace(uint64_t& iTotal, uint64_t& iUsed, std::vector<std::string>& locations);
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
    bool GetDeviceHasHDD() const { return m_deviceHasHDD; };

  private:
    static void SetCharString(char* target, const std::string value);
    bool LoadDeviceInfo();
    bool LoadAutoTimerSettings();
    bool LoadRecordingMarginSettings();
    unsigned int ParseWebIfVersion(const std::string& webIfVersion);
    uint64_t GetKbFromString(const std::string& stringInMbGbTb) const;
    utilities::StreamStatus GetStreamDetails(const std::shared_ptr<data::Channel>& channel);
    void GetTunerDetails(utilities::SignalStatus& signalStatus, const std::shared_ptr<data::Channel>& channel);

    char m_serverName[256];
    char m_serverVersion[256];
    bool m_deviceHasHDD = true;
    const std::string m_addonVersion;
    enigma2::utilities::DeviceInfo m_deviceInfo;
    enigma2::utilities::DeviceSettings m_deviceSettings;
    std::vector<enigma2::utilities::Tuner> m_tuners;
  };
} //namespace enigma2
