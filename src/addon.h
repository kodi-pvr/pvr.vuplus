/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>
#include <unordered_map>

#include "enigma2/Settings.h"

class Enigma2;

class ATTRIBUTE_HIDDEN CEnigma2Addon : public kodi::addon::CAddonBase
{
public:
  CEnigma2Addon() = default;

  ADDON_STATUS Create() override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue) override;
  ADDON_STATUS CreateInstance(int instanceType, const std::string& instanceID, KODI_HANDLE instance, const std::string& version, KODI_HANDLE& addonInstance) override;
  void DestroyInstance(int instanceType, const std::string& instanceID, KODI_HANDLE addonInstance) override;

private:
  std::unordered_map<std::string, Enigma2*> m_usedInstances;
  enigma2::Settings& m_settings = enigma2::Settings::GetInstance();
};
