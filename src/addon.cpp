/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
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
  /* Init settings */
  m_settings.reset(new Settings());

  Logger::Log(LEVEL_DEBUG, "%s - Creating VU+ PVR-Client", __func__);

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([this](LogLevel level, const char* message)
  {
    /* Don't log trace messages unless told so */
    if (level == LogLevel::LEVEL_TRACE && !m_settings->GetTraceDebug())
      return;

    /* Convert the log level */
    ADDON_LOG addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_FATAL:
        addonLevel = ADDON_LOG::ADDON_LOG_FATAL;
        break;
      case LogLevel::LEVEL_ERROR:
        addonLevel = ADDON_LOG::ADDON_LOG_ERROR;
        break;
      case LogLevel::LEVEL_WARNING:
        addonLevel = ADDON_LOG::ADDON_LOG_WARNING;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = ADDON_LOG::ADDON_LOG_INFO;
        break;
      default:
        addonLevel = ADDON_LOG::ADDON_LOG_DEBUG;
    }

    if (addonLevel == ADDON_LOG::ADDON_LOG_DEBUG && m_settings->GetNoDebug())
      return;

    if (addonLevel == ADDON_LOG::ADDON_LOG_DEBUG && m_settings->GetDebugNormal())
      addonLevel = ADDON_LOG::ADDON_LOG_INFO;

    kodi::Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.vuplus");

  Logger::Log(LogLevel::LEVEL_INFO, "%s starting PVR client...", __func__);

  return ADDON_STATUS_OK;
}

ADDON_STATUS CEnigma2Addon::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  return m_settings->SetValue(settingName, settingValue);
}

ADDON_STATUS CEnigma2Addon::CreateInstance(const kodi::addon::IInstanceInfo& instance, KODI_ADDON_INSTANCE_HDL& hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    Enigma2* usedInstance = new Enigma2(instance, m_settings);
    if (!usedInstance->Start())
    {
      delete usedInstance;
      return ADDON_STATUS_PERMANENT_FAILURE;
    }
    hdl = usedInstance;

    // Store this instance also on this class, currently support Kodi only one
    // instance, for that it becomes stored here to interact e.g. about
    // settings. In future becomes another way added.
    m_usedInstances.emplace(std::make_pair(instance.GetID(), usedInstance));
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

void CEnigma2Addon::DestroyInstance(const kodi::addon::IInstanceInfo& instance, const KODI_ADDON_INSTANCE_HDL hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    const auto& it = m_usedInstances.find(instance.GetID());
    if (it != m_usedInstances.end())
    {
      it->second->SendPowerstate();
      m_usedInstances.erase(it);
    }
  }
}

ADDONCREATOR(CEnigma2Addon)
