/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "addon.h"
#include "Enigma2.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

ADDON_STATUS CEnigma2Addon::Create()
{
  Logger::Log(LEVEL_DEBUG, "%s - Creating VU+ PVR-Client", __func__);

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([](LogLevel level, const char* message)
  {
    /* Don't log trace messages unless told so */
    if (level == LogLevel::LEVEL_TRACE && !Settings::GetInstance().GetTraceDebug())
      return;

    /* Convert the log level */
    AddonLog addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_FATAL:
        addonLevel = AddonLog::ADDON_LOG_FATAL;
        break;
      case LogLevel::LEVEL_ERROR:
        addonLevel = AddonLog::ADDON_LOG_ERROR;
        break;
      case LogLevel::LEVEL_WARNING:
        addonLevel = AddonLog::ADDON_LOG_WARNING;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = AddonLog::ADDON_LOG_INFO;
        break;
      default:
        addonLevel = AddonLog::ADDON_LOG_DEBUG;
    }

    if (addonLevel == AddonLog::ADDON_LOG_DEBUG && Settings::GetInstance().GetNoDebug())
      return;

    if (addonLevel == AddonLog::ADDON_LOG_DEBUG && Settings::GetInstance().GetDebugNormal())
      addonLevel = AddonLog::ADDON_LOG_INFO;

    kodi::Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.vuplus");

  Logger::Log(LogLevel::LEVEL_INFO, "%s starting PVR client...", __func__);

  m_settings.ReadFromAddon();

  return ADDON_STATUS_OK;
}

ADDON_STATUS CEnigma2Addon::SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue)
{
  return m_settings.SetValue(settingName, settingValue);
}

ADDON_STATUS CEnigma2Addon::CreateInstance(int instanceType, const std::string& instanceID, KODI_HANDLE instance, const std::string& version, KODI_HANDLE& addonInstance)
{
  if (instanceType == ADDON_INSTANCE_PVR)
  {
    Enigma2* usedInstance = new Enigma2(instance, version);
    if (!usedInstance->Start())
    {
      delete usedInstance;
      return ADDON_STATUS_PERMANENT_FAILURE;
    }
    addonInstance = usedInstance;

    // Store this instance also on this class, currently support Kodi only one
    // instance, for that it becomes stored here to interact e.g. about
    // settings. In future becomes another way added.
    m_usedInstances.emplace(std::make_pair(instanceID, usedInstance));
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

void CEnigma2Addon::DestroyInstance(int instanceType, const std::string& instanceID, KODI_HANDLE addonInstance)
{
  if (instanceType == ADDON_INSTANCE_PVR)
  {
    const auto& it = m_usedInstances.find(instanceID);
    if (it != m_usedInstances.end())
    {
      it->second->SendPowerstate();
      m_usedInstances.erase(it);
    }
  }
}

ADDONCREATOR(CEnigma2Addon)
