/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

#include <memory>
#include <unordered_map>

#include "enigma2/AddonSettings.h"

class Enigma2;

class ATTR_DLL_LOCAL CEnigma2Addon : public kodi::addon::CAddonBase
{
public:
  CEnigma2Addon() = default;

  ADDON_STATUS Create() override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue) override;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl) override;
  void DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl) override;

private:
  std::unordered_map<std::string, Enigma2*> m_usedInstances;
  std::shared_ptr<enigma2::AddonSettings> m_settings;
};
