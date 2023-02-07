/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "AddonSettings.h"

#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/SettingsMigration.h"

#include "kodi/General.h"

using namespace enigma2;
using namespace enigma2::utilities;

namespace
{

constexpr bool DEFAULT_TRACE_DEBUG = false;

bool ReadBoolSetting(const std::string& key, bool def)
{
  bool value;
  if (kodi::addon::CheckSettingBoolean(key, value))
    return value;

  return def;
}

ADDON_STATUS SetBoolSetting(bool oldValue, const kodi::addon::CSettingValue& newValue)
{
  if (oldValue == newValue.GetBoolean())
    return ADDON_STATUS_OK;

  return ADDON_STATUS_NEED_RESTART;
}

} // unnamed namespace

AddonSettings::AddonSettings()
{
  ReadSettings();
}

void AddonSettings::ReadSettings()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

  m_noDebug = kodi::addon::GetSettingBoolean("nodebug", false);
  m_debugNormal = kodi::addon::GetSettingBoolean("debugnormal", false);
  m_traceDebug = kodi::addon::GetSettingBoolean("tracedebug", false);
}

ADDON_STATUS AddonSettings::SetSetting(const std::string& settingName,
                                       const kodi::addon::CSettingValue& settingValue)
{
  if (settingName == "nodebug")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_noDebug, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "debugnormal")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_debugNormal, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tracedebug")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_traceDebug, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (SettingsMigration::IsMigrationSetting(settingName))
  {
    // ignore settings from pre-multi-instance setup
    return ADDON_STATUS_OK;
  }

  Logger::Log(LogLevel::LEVEL_ERROR, "AddonSettings::SetSetting - unknown setting '%s'",
              settingName.c_str());
  return ADDON_STATUS_UNKNOWN;
}