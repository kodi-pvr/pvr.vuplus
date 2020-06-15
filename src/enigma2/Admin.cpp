/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Admin.h"

#include "../Enigma2.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/WebUtils.h"
#include "utilities/XMLUtils.h"

#include <cstdlib>
#include <regex>

#include <kodi/General.h>
#include <nlohmann/json.hpp>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using json = nlohmann::json;

Admin::Admin() : m_addonVersion(STR(VUPLUS_VERSION))
{
  m_serverName[0] = '\0';
  m_serverVersion[0] = '\0';
};

void Admin::SendPowerstate()
{
  if (Settings::GetInstance().GetPowerstateModeOnAddonExit() != PowerstateMode::DISABLED)
  {
    if (Settings::GetInstance().GetPowerstateModeOnAddonExit() == PowerstateMode::WAKEUP_THEN_STANDBY)
    {
      const std::string strCmd = StringUtils::Format("web/powerstate?newstate=4"); //Wakeup

      std::string strResult;
      WebUtils::SendSimpleCommand(strCmd, strResult, true);
    }

    if (Settings::GetInstance().GetPowerstateModeOnAddonExit() == PowerstateMode::STANDBY ||
      Settings::GetInstance().GetPowerstateModeOnAddonExit() == PowerstateMode::WAKEUP_THEN_STANDBY)
    {
      const std::string strCmd = StringUtils::Format("web/powerstate?newstate=5"); //Standby

      std::string strResult;
      WebUtils::SendSimpleCommand(strCmd, strResult, true);
    }

    if (Settings::GetInstance().GetPowerstateModeOnAddonExit() == PowerstateMode::DEEP_STANDBY)
    {
      const std::string strCmd = StringUtils::Format("web/powerstate?newstate=1"); //Deep Standby

      std::string strResult;
      WebUtils::SendSimpleCommand(strCmd, strResult, true);
    }
  }
}

bool Admin::Initialise()
{
  std::string unknown = kodi::GetLocalizedString(30081);
  SetCharString(m_serverName, unknown);
  SetCharString(m_serverVersion, unknown);

  Settings::GetInstance().SetAdmin(this);

  bool deviceInfoLoaded = LoadDeviceInfo();

  if (deviceInfoLoaded)
  {
    Settings::GetInstance().SetDeviceInfo(&m_deviceInfo);

    bool deviceSettingsLoaded = LoadDeviceSettings();
    Settings::GetInstance().SetDeviceSettings(&m_deviceSettings);

    if (deviceSettingsLoaded)
    {
      //If OpenWebVersion is new enough to allow the setting of AutoTimer setttings
      if (Settings::GetInstance().SupportsAutoTimers())
        SendAutoTimerSettings();
    }
  }

  return deviceInfoLoaded;
}

bool Admin::LoadDeviceInfo()
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/deviceinfo");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  std::string enigmaVersion;
  std::string imageVersion;
  std::string distroName;
  std::string webIfVersion;
  std::string deviceName = "Enigma2";
  unsigned int webIfVersionAsNum;

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __func__);
    return false;
  }

  std::string strTmp;

  Logger::Log(LEVEL_INFO, "%s - DeviceInfo", __func__);

  // Get EnigmaVersion
  if (!xml::GetString(pElem, "e2enigmaversion", strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2enigmaversion from result!", __func__);
    return false;
  }
  enigmaVersion = strTmp.c_str();
  Logger::Log(LEVEL_INFO, "%s - E2EnigmaVersion: %s", __func__, enigmaVersion.c_str());

  // Get ImageVersion
  if (!xml::GetString(pElem, "e2imageversion", strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2imageversion from result!", __func__);
    return false;
  }
  imageVersion = strTmp.c_str();
  Logger::Log(LEVEL_INFO, "%s - E2ImageVersion: %s", __func__, imageVersion.c_str());

  // Get distroName
  if (!xml::GetString(pElem, "e2distroversion", strTmp))
  {
    Logger::Log(LEVEL_INFO, "%s Could not parse e2distroversion from result, continuing as not available in all images!", __func__);
    strTmp = kodi::GetLocalizedString(30081); //unknown
  }
  else
  {
    distroName = strTmp.c_str();
  }
  Logger::Log(LEVEL_INFO, "%s - E2DistroName: %s", __func__, distroName.c_str());

  // Get WebIfVersion
  if (!xml::GetString(pElem, "e2webifversion", strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2webifversion from result!", __func__);
    return false;
  }
  webIfVersion = strTmp.c_str();
  webIfVersionAsNum = ParseWebIfVersion(webIfVersion);

  Logger::Log(LEVEL_INFO, "%s - E2WebIfVersion: %s", __func__, webIfVersion.c_str());

  // Get DeviceName
  if (!xml::GetString(pElem, "e2devicename", strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2devicename from result!", __func__);
    return false;
  }
  deviceName = strTmp.c_str();
  Logger::Log(LEVEL_INFO, "%s - E2DeviceName: %s", __func__, deviceName.c_str());

  m_deviceInfo = DeviceInfo(deviceName, enigmaVersion, imageVersion, distroName, webIfVersion, webIfVersionAsNum);

  std::string version = webIfVersion + " - " + distroName + " (" + imageVersion + "/" + enigmaVersion + ")";
  SetCharString(m_serverName, deviceName);
  SetCharString(m_serverVersion, version);

  Logger::Log(LEVEL_INFO, "%s - ServerVersion: %s", __func__, m_serverVersion);

  Logger::Log(LEVEL_INFO, "%s - AddonVersion: %s", __func__, m_addonVersion.c_str());

  hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2frontends").Element();

  if (pNode)
  {
    TiXmlElement* tunerNode = pNode->FirstChildElement("e2frontend");

    if (tunerNode)
    {
      int tunerNumber = 0;

      for (; tunerNode != nullptr; tunerNode = tunerNode->NextSiblingElement("e2frontend"))
      {
        std::string tunerName;
        std::string tunerModel;

        xml::GetString(tunerNode, "e2name", tunerName);
        xml::GetString(tunerNode, "e2model", tunerModel);

        m_tuners.emplace_back(Tuner(tunerNumber, tunerName, tunerModel));

        Logger::Log(LEVEL_DEBUG, "%s Tuner Info Loaded - Tuner Number: %d, Tuner Name:%s Tuner Model: %s", __func__, tunerNumber, tunerName.c_str(), tunerModel.c_str());

        tunerNumber++;
      }
    }
    else
    {
      Logger::Log(LEVEL_DEBUG, "%s Could not find <e2frontend> element", __func__);
    }
  }
  else
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2frontends> element", __func__);
  }

  return true;
}

unsigned int Admin::ParseWebIfVersion(const std::string& webIfVersion)
{
  unsigned int webIfVersionAsNum = 0;

  static const std::regex regex("^.*[0-9]+\\.[0-9]+\\.[0-9].*$");
  if (std::regex_match(webIfVersion, regex))
  {
    int count = 0;
    unsigned int versionPart = 0;
    static const std::regex pattern("([0-9]+)");
    for (auto i = std::sregex_iterator(webIfVersion.begin(), webIfVersion.end(), pattern); i != std::sregex_iterator(); ++i)
    {
        switch (count)
        {
          case 0:
            versionPart = std::atoi(i->str().c_str());
            webIfVersionAsNum = versionPart << 16;
            break;
          case 1:
              versionPart = std::atoi(i->str().c_str());
              webIfVersionAsNum |= versionPart << 8;
            break;
          case 2:
              versionPart = std::atoi(i->str().c_str());
              webIfVersionAsNum |= versionPart;
            break;
        }

        count++;
    }
  }

  return webIfVersionAsNum;
}

bool Admin::LoadDeviceSettings()
{
  kodi::SetSettingString("webifversion", m_deviceInfo.GetWebIfVersion());

  std::string autoTimerTagInTags = kodi::GetLocalizedString(30094); // N/A
  std::string autoTimerNameInTags = kodi::GetLocalizedString(30094); // N/A

  //If OpenWebVersion is new enough to allow the setting of AutoTimer setttings
  if (Settings::GetInstance().SupportsAutoTimers())
  {
    if (LoadAutoTimerSettings())
    {
      if (m_deviceSettings.IsAddTagAutoTimerToTagsEnabled())
        autoTimerTagInTags = kodi::GetLocalizedString(30095); // True
      else
        autoTimerTagInTags = kodi::GetLocalizedString(30096); // False
      if (m_deviceSettings.IsAddAutoTimerNameToTagsEnabled())
        autoTimerNameInTags = kodi::GetLocalizedString(30095); // True
      else
        autoTimerNameInTags = kodi::GetLocalizedString(30096); //False
    }
  }

  kodi::SetSettingString("autotimertagintags", autoTimerTagInTags);
  kodi::SetSettingString("autotimernameintags", autoTimerNameInTags);

  if (!LoadRecordingMarginSettings())
  {
    return false;
  }
  else
  {
    kodi::SetSettingInt("globalstartpaddingstb", m_deviceSettings.GetGlobalRecordingStartMargin());
    kodi::SetSettingInt("globalendpaddingstb", m_deviceSettings.GetGlobalRecordingEndMargin());
  }

  return true;
}

bool Admin::LoadAutoTimerSettings()
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "autotimer/get");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2settings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2settings> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2setting").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2setting> element", __func__);
    return false;
  }

  std::string settingName;
  std::string settingValue;
  bool setAutoTimerToTags = false;
  bool setAutoTimerNameToTags = false;
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2setting"))
  {
    if (!xml::GetString(pNode, "e2settingname", settingName))
      return false;

    if (!xml::GetString(pNode, "e2settingvalue", settingValue))
      return false;

    if (settingName == "config.plugins.autotimer.add_autotimer_to_tags")
    {
      m_deviceSettings.SetAddTagAutoTimerToTagsEnabled(settingValue == "True");
      setAutoTimerToTags = true;
    }
    else if (settingName == "config.plugins.autotimer.add_name_to_tags")
    {
       m_deviceSettings.SetAddAutoTimerNameToTagsEnabled(settingValue == "True");
       setAutoTimerNameToTags = true;
    }

    if (setAutoTimerNameToTags && setAutoTimerToTags)
      break;
  }

  Logger::Log(LEVEL_DEBUG, "%s Add Tag AutoTimer to Tags: %d, Add AutoTimer Name to tags: %d", __func__,  m_deviceSettings.IsAddTagAutoTimerToTagsEnabled(),  m_deviceSettings.IsAddAutoTimerNameToTagsEnabled());

  return true;
}

bool Admin::LoadRecordingMarginSettings()
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/settings");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2settings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2settings> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2setting").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2setting> element", __func__);
    return false;
  }

  std::string settingName;
  std::string settingValue;
  bool readMarginBefore = false;
  bool readMarginAfter = false;
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2setting"))
  {
    if (!xml::GetString(pNode, "e2settingname", settingName))
      continue;

    if (!xml::GetString(pNode, "e2settingvalue", settingValue))
      continue;

    if (settingName == "config.recording.margin_before")
    {
      m_deviceSettings.SetGlobalRecordingStartMargin(std::atoi(settingValue.c_str()));
      readMarginBefore = true;
    }
    else if (settingName == "config.recording.margin_after")
    {
       m_deviceSettings.SetGlobalRecordingEndMargin(std::atoi(settingValue.c_str()));
       readMarginAfter = true;
    }

    if (readMarginBefore && readMarginAfter)
      break;
  }

  Logger::Log(LEVEL_DEBUG, "%s Margin Before: %d, Margin After: %d", __func__,  m_deviceSettings.GetGlobalRecordingStartMargin(),  m_deviceSettings.GetGlobalRecordingEndMargin());

  return true;
}

bool Admin::SendAutoTimerSettings()
{
  if (!(m_deviceSettings.IsAddTagAutoTimerToTagsEnabled() && m_deviceSettings.IsAddAutoTimerNameToTagsEnabled()))
  {
    Logger::Log(LEVEL_DEBUG, "%s Setting AutoTimer Settings on Backend", __func__);
    const std::string url = StringUtils::Format("%s", "autotimer/set?add_name_to_tags=true&add_autotimer_to_tags=true");
    std::string strResult;

    if (!WebUtils::SendSimpleCommand(url, strResult))
      return false;
  }

  return true;
}

bool Admin::SendGlobalRecordingStartMarginSetting(int newValue)
{
  if (m_deviceSettings.GetGlobalRecordingStartMargin() != newValue)
  {
    Logger::Log(LEVEL_INFO, "%s Setting Global Recording Start Margin Backend, from: %d, to: %d", __func__, m_deviceSettings.GetGlobalRecordingStartMargin(), newValue);
    const std::string url = StringUtils::Format("%s%d", "api/saveconfig?key=config.recording.margin_before&value=", newValue);
    std::string strResult;

    if (!WebUtils::SendSimpleJsonPostCommand(url, strResult))
      return false;
    else
      m_deviceSettings.SetGlobalRecordingStartMargin(newValue);
  }

  return true;
}

bool Admin::SendGlobalRecordingEndMarginSetting(int newValue)
{
  if (m_deviceSettings.GetGlobalRecordingEndMargin() != newValue)
  {
    Logger::Log(LEVEL_INFO, "%s Setting Global Recording End Margin Backend, from: %d, to: %d", __func__, m_deviceSettings.GetGlobalRecordingEndMargin(), newValue);
    const std::string url = StringUtils::Format("%s%d", "api/saveconfig?key=config.recording.margin_after&value=", newValue);
    std::string strResult;

    if (!WebUtils::SendSimpleJsonPostCommand(url, strResult))
      return false;
    else
      m_deviceSettings.SetGlobalRecordingEndMargin(newValue);
  }

  return true;
}

PVR_ERROR Admin::GetDriveSpace(uint64_t& iTotal, uint64_t& iUsed, std::vector<std::string>& locations)
{
  uint64_t totalKb = 0;
  uint64_t freeKb = 0;

  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/deviceinfo");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __func__);
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2hdds").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2hdds> element", __func__);
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlElement* hddNode = pNode->FirstChildElement("e2hdd");

  if (!hddNode)
  {
    m_deviceHasHDD = false;
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2hdd> element", __func__);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (; hddNode != nullptr; hddNode = hddNode->NextSiblingElement("e2hdd"))
  {
    std::string capacity;
    std::string freeSpace;
    std::string mount;

    xml::GetString(hddNode, "e2capacity", capacity);
    xml::GetString(hddNode, "e2free", freeSpace);
    xml::GetString(hddNode, "e2mount", mount);

    if (!mount.empty())
    {
      auto it = std::find_if(locations.begin(), locations.end(),
        [&mount](std::string& location) { return location.find(mount) != std::string::npos; });

      if (it == locations.end())
        continue; // no valid mount point
    }

    totalKb += GetKbFromString(capacity);
    freeKb += GetKbFromString(freeSpace);
  }

  iTotal = totalKb;
  iUsed = totalKb - freeKb;

  Logger::Log(LEVEL_INFO, "%s Space Total: %lld, Used %lld", __func__, iTotal, iUsed);

  return PVR_ERROR_NO_ERROR;
}

uint64_t Admin::GetKbFromString(const std::string& stringInMbGbTb) const
{
  uint64_t sizeInKb = 0;

  static const std::vector<std::string> sizes = {"MB", "GB", "TB"};
  long multiplier = 1024;
  std::string replaceWith = "";
  for (const std::string& size : sizes)
  {
    const std::regex regexSize("^.* " + size);
    const std::regex regexReplaceSize(" " + size);

    if (std::regex_match(stringInMbGbTb, regexSize))
    {
      double sizeValue = std::atof(std::regex_replace(stringInMbGbTb, regexReplaceSize, replaceWith).c_str());

      sizeInKb += static_cast<uint64_t>(sizeValue * multiplier);

      break;
    }

    multiplier = multiplier * 1024;
  }

  return sizeInKb;
}

bool Admin::GetTunerSignal(SignalStatus& signalStatus, const std::shared_ptr<data::Channel>& channel)
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/signal");

  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  std::string snrDb;
  std::string snrPercentage;
  std::string ber;
  std::string signalStrength;

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2frontendstatus").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2frontendstatus> element!", __func__);
    return false;
  }

  if (!xml::GetString(pElem, "e2snrdb", snrDb))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2snrdb from result!", __func__);
    return false;
  }

  if (!xml::GetString(pElem, "e2snr", snrPercentage))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2snr from result!", __func__);
    return false;
  }

  if (!xml::GetString(pElem, "e2ber", ber))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2ber from result!", __func__);
    return false;
  }

  if (!xml::GetString(pElem, "e2acg", signalStrength))
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2acg from result!", __func__);
    return false;
  }

  static const std::regex regexReplacePercent(" %");
  std::string regexReplace = "";

  // For some reason the iSNR and iSignal values need to multiplied by 655!
  signalStatus.m_snrPercentage = std::atoi(std::regex_replace(snrPercentage, regexReplacePercent, regexReplace).c_str()) * 655;
  signalStatus.m_ber = std::atol(ber.c_str());
  signalStatus.m_signalStrength = std::atoi(std::regex_replace(signalStrength, regexReplacePercent, regexReplace).c_str()) * 655;

  if (Settings::GetInstance().SupportsTunerDetails())
  {
    //TODO: Cross reference against tuners once OpenWebIf API is updated.
    //StreamStatus streamStatus = GetStreamDetails(channel);
    GetTunerDetails(signalStatus, channel);
  }

  return true;
}

StreamStatus Admin::GetStreamDetails(const std::shared_ptr<data::Channel>& channel)
{
  StreamStatus streamStatus;

  const std::string jsonUrl = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "api/deviceinfo");

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    if (!jsonDoc["streams"].empty())
    {
      for (const auto& it : jsonDoc["streams"].items())
      {
        auto jsonStream = it.value();

        if (jsonStream["ref"].get<std::string>() == channel->GetGenericServiceReference() &&
            !jsonStream["ip"].get<std::string>().empty()) //TODO: Find out Kodi IP and compare
        {
          streamStatus.m_ipAddress = jsonStream["ip"].get<std::string>();
          streamStatus.m_serviceReference = channel->GetServiceReference();
          streamStatus.m_channelName = channel->GetChannelName(); //Use our channel name as from JSON is unreliable

          if (jsonStream["type"].get<std::string>() == "S")
            streamStatus.m_streamType = StreamType::DIRECTLY_STREAMED;
          else
            streamStatus.m_streamType = StreamType::TRANSCODED;

          break;
        }

        Logger::Log(LEVEL_DEBUG, "%s Active Stream IP: %s, ref: %s, name: %s", __func__, jsonStream["ip"].get<std::string>().c_str(), jsonStream["ref"].get<std::string>().c_str(), jsonStream["name"].get<std::string>().c_str());
      }
    }

    if (!streamStatus.m_channelName.empty())
    {
      if (!jsonDoc["tuners"].empty())
      {
        int tunerNumber = 0;

        for (const auto& it : jsonDoc["tuners"].items())
        {
          auto jsonTuner = it.value();

          if (jsonTuner["name"].get<std::string>() == streamStatus.m_channelName)
          {
            //TODO: Complete once API is available

            break;
          }

          tunerNumber++;
        }
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load extra stream details from OpenWebIf - JSON parse error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }

  return streamStatus;
}

void Admin::GetTunerDetails(SignalStatus& signalStatus, const std::shared_ptr<data::Channel>& channel)
{
  const std::string jsonUrl = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "api/tunersignal");

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  try
  {
    auto jsonDoc = json::parse(strJson);

    for (const auto& element : jsonDoc.items())
    {
      if (element.key() == "tunernumber")
      {
        Logger::Log(LEVEL_DEBUG, "%s Json API - %s : %d", __func__, element.key().c_str(), element.value().get<int>());

        int tunerNumber = element.value().get<int>();

        if (m_tuners.size() > tunerNumber)
        {
          Tuner& tuner = m_tuners.at(tunerNumber);

          signalStatus.m_adapterName = tuner.m_tunerName + " - " + tuner.m_tunerModel;
        }
      }
      else if (element.key() == "tunertype")
      {
        Logger::Log(LEVEL_DEBUG, "%s Json API - %s : %s", __func__, element.key().c_str(), element.value().get<std::string>().c_str());

        signalStatus.m_adapterStatus = element.value().get<std::string>();
      }
    }
  }
  catch (nlohmann::detail::parse_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s Invalid JSON received, cannot load extra tuner details from OpenWebIf - JSON parse error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }
  catch (nlohmann::detail::type_error& e)
  {
    Logger::Log(LEVEL_ERROR, "%s JSON type error - message: %s, exception id: %d", __func__, e.what(), e.id);
  }
}

void Admin::SetCharString(char* target, const std::string value)
{
  std::copy(value.begin(), value.end(), target);
  target[value.size()] = '\0';
}
