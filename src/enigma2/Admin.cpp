#include "Admin.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/FileUtils.h"
#include "utilities/LocalizedString.h"
#include "utilities/json.hpp"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;
using json = nlohmann::json;

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
      const std::string strCmd  = StringUtils::Format("web/powerstate?newstate=5"); //Standby

      std::string strResult;
      WebUtils::SendSimpleCommand(strCmd, strResult, true);
    }    

    if (Settings::GetInstance().GetPowerstateModeOnAddonExit() == PowerstateMode::DEEP_STANDBY)
    {
      const std::string strCmd  = StringUtils::Format("web/powerstate?newstate=1"); //Deep Standby

      std::string strResult;
      WebUtils::SendSimpleCommand(strCmd, strResult, true); 
    }
  }
}

bool Admin::Initialise()
{
  Settings::GetInstance().SetAdmin(this);

  bool deviceInfoLoaded = LoadDeviceInfo();

  if (deviceInfoLoaded)
  {
    Settings::GetInstance().SetDeviceInfo(&m_deviceInfo);

    if (LoadDeviceSettings())
    {
      Settings::GetInstance().SetDeviceSettings(&m_deviceSettings);

      //If OpenWebVersion is new enough to allow the setting of AutoTimer setttings
      if (Settings::GetInstance().GetWebIfVersionAsNum() >= Settings::GetInstance().GenerateWebIfVersionAsNum(1, 3, 0))
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
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  std::string enigmaVersion;
  std::string imageVersion;
  std::string distroVersion;
  std::string webIfVersion;
  std::string serverName = "Enigma2";    
  unsigned int webIfVersionAsNum;

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return false;
  }

  std::string strTmp;

  Logger::Log(LEVEL_NOTICE, "%s - DeviceInfo", __FUNCTION__);

  // Get EnigmaVersion
  if (!XMLUtils::GetString(pElem, "e2enigmaversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2enigmaversion from result!", __FUNCTION__);
    return false;
  }
  enigmaVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, enigmaVersion.c_str());

  // Get ImageVersion
  if (!XMLUtils::GetString(pElem, "e2imageversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  imageVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, imageVersion.c_str());

  // Get DistroVersion
  if (!XMLUtils::GetString(pElem, "e2distroversion", strTmp)) 
  {
    Logger::Log(LEVEL_NOTICE, "%s Could not parse e2distroversion from result, continuing as not available in all images!", __FUNCTION__);
    strTmp = LocalizedString(60081); //unknown
  }
  else
  {
    distroVersion = strTmp.c_str();
  }
  Logger::Log(LEVEL_NOTICE, "%s - E2DistroVersion: %s", __FUNCTION__, distroVersion.c_str());

  // Get WebIfVersion
  if (!XMLUtils::GetString(pElem, "e2webifversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  webIfVersion = strTmp.c_str();
  webIfVersionAsNum = ParseWebIfVersion(webIfVersion);
  
  Logger::Log(LEVEL_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, webIfVersion.c_str());

  // Get DeviceName
  if (!XMLUtils::GetString(pElem, "e2devicename", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  serverName = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, serverName.c_str());

  m_deviceInfo = DeviceInfo(serverName, enigmaVersion, imageVersion, distroVersion, webIfVersion, webIfVersionAsNum);

  hRoot=TiXmlHandle(pElem);

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

        XMLUtils::GetString(tunerNode, "e2name", tunerName);
        XMLUtils::GetString(tunerNode, "e2model", tunerModel);

        m_tuners.emplace_back(Tuner(tunerNumber, tunerName, tunerModel));

        Logger::Log(LEVEL_DEBUG, "%s Tuner Info Loaded - Tuner Number: %d, Tuner Name:%s Tuner Model: %s", __FUNCTION__, tunerNumber, tunerName.c_str(), tunerModel.c_str());

        tunerNumber++;
      }
    }  
    else
    {
      Logger::Log(LEVEL_DEBUG, "Could not find <e2frontend> element");
    }
  }
  else
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2frontends> element");
  }

  return true;
}

unsigned int Admin::ParseWebIfVersion(const std::string &webIfVersion)
{
  unsigned int webIfVersionAsNum = 0;

  std::regex regex ("^.*[0-9]+\\.[0-9]+\\.[0-9].*$");
  if (regex_match(webIfVersion, regex))
  {
    int count = 0;
    unsigned int versionPart = 0;
    std::regex pattern("([0-9]+)");
    for (auto i = std::sregex_iterator(webIfVersion.begin(), webIfVersion.end(), pattern); i != std::sregex_iterator(); ++i) 
    {
        switch (count)
        {
          case 0:
            versionPart = atoi(i->str().c_str());
            webIfVersionAsNum = versionPart << 16;
            break;
          case 1:
              versionPart = atoi(i->str().c_str());
              webIfVersionAsNum |= versionPart << 8;
            break;     
          case 2:
              versionPart = atoi(i->str().c_str());
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
  //TODO: Include once addon starts to use new API
  //kodi::SetSettingString("webifversion", m_deviceInfo.GetWebIfVersion());

  if (!LoadAutoTimerSettings())
  {
    return false;
  }
  else
  {
    std::string autoTimerTagInTags = LocalizedString(30094); // N/A
    std::string autoTimerNameInTags = LocalizedString(30094); // N/A

    //If OpenWebVersion is new enough to allow the setting of AutoTimer setttings
    if (Settings::GetInstance().GetWebIfVersionAsNum() >= Settings::GetInstance().GenerateWebIfVersionAsNum(1, 3, 0))
    {
      if (m_deviceSettings.IsAddTagAutoTimerToTagsEnabled())
        autoTimerTagInTags = LocalizedString(30095); // True
      else
        autoTimerTagInTags = LocalizedString(30096); // False
      if (m_deviceSettings.IsAddAutoTimerNameToTagsEnabled())
        autoTimerNameInTags = LocalizedString(30095); // True
      else
        autoTimerNameInTags = LocalizedString(30096); //False
    }

    //TODO: Include once addon starts to use new API
    //kodi::SetSettingString("autotimertagintags", autoTimerTagInTags);
    //kodi::SetSettingString("autotimernameintags", autoTimerNameInTags);
  }

  if (!LoadRecordingMarginSettings())
  {
    return false;
  }
  else
  {
    //TODO: Include once addon starts to use new API
    //kodi::SetSettingInt("globalstartpaddingstb", m_deviceSettings.GetGlobalRecordingStartMargin());
    //kodi::SetSettingInt("globalendpaddingstb", m_deviceSettings.GetGlobalRecordingEndMargin());
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
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2settings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2settings> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2setting").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2setting> element");
    return false;
  }

  std::string settingName;
  std::string settingValue;
  bool setAutoTimerToTags = false;
  bool setAutoTimerNameToTags = false;
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2setting"))
  {
    if (!XMLUtils::GetString(pNode, "e2settingname", settingName)) 
      return false;

    if (!XMLUtils::GetString(pNode, "e2settingvalue", settingValue)) 
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

  Logger::Log(LEVEL_DEBUG, "%s Add Tag AutoTimer to Tags: %d, Add AutoTimer Name to tags: %d", __FUNCTION__,  m_deviceSettings.IsAddTagAutoTimerToTagsEnabled(),  m_deviceSettings.IsAddAutoTimerNameToTagsEnabled());

  return true;
}

bool Admin::LoadRecordingMarginSettings()
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/settings"); 

  const std::string strXML = WebUtils::GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2settings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2settings> element!", __FUNCTION__);
    return false;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2setting").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2setting> element");
    return false;
  }

  std::string settingName;
  std::string settingValue;
  bool readMarginBefore = false;
  bool readMarginAfter = false;
  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2setting"))
  {
    if (!XMLUtils::GetString(pNode, "e2settingname", settingName)) 
      continue;

    if (!XMLUtils::GetString(pNode, "e2settingvalue", settingValue)) 
      continue;          

    if (settingName == "config.recording.margin_before")
    {
      m_deviceSettings.SetGlobalRecordingStartMargin(atoi(settingValue.c_str()));
      readMarginBefore = true;
    }  
    else if (settingName == "config.recording.margin_after")
    {
       m_deviceSettings.SetGlobalRecordingEndMargin(atoi(settingValue.c_str())); 
       readMarginAfter = true;
    }

    if (readMarginBefore && readMarginAfter)
      break;
  }

  Logger::Log(LEVEL_DEBUG, "%s Margin Before: %d, Margin After: %d", __FUNCTION__,  m_deviceSettings.GetGlobalRecordingStartMargin(),  m_deviceSettings.GetGlobalRecordingEndMargin());

  return true;
}

bool Admin::SendAutoTimerSettings()
{
  if (!(m_deviceSettings.IsAddTagAutoTimerToTagsEnabled() && m_deviceSettings.IsAddAutoTimerNameToTagsEnabled()))
  {
    Logger::Log(LEVEL_DEBUG, "%s Setting AutoTimer Settings on Backend", __FUNCTION__);
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
    Logger::Log(LEVEL_NOTICE, "%s Setting Global Recording Start Margin Backend, from: %d, to: %d", __FUNCTION__, m_deviceSettings.GetGlobalRecordingStartMargin(), newValue);
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
    Logger::Log(LEVEL_NOTICE, "%s Setting Global Recording End Margin Backend, from: %d, to: %d", __FUNCTION__, m_deviceSettings.GetGlobalRecordingEndMargin(), newValue);
    const std::string url = StringUtils::Format("%s%d", "api/saveconfig?key=config.recording.margin_after&value=", newValue);
    std::string strResult;

    if (!WebUtils::SendSimpleJsonPostCommand(url, strResult)) 
      return false;
    else
      m_deviceSettings.SetGlobalRecordingEndMargin(newValue);
  }

  return true;
}

PVR_ERROR Admin::GetDriveSpace(long long *iTotal, long long *iUsed, std::vector<std::string> &locations)
{
  long long totalKb = 0;
  long long freeKb = 0;

  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/deviceinfo"); 

  const std::string strXML = WebUtils::GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_DEBUG, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2deviceinfo> element!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  hRoot=TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2hdds").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2hdds> element");
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlElement* hddNode = pNode->FirstChildElement("e2hdd");

  if (!hddNode)
  {
    Logger::Log(LEVEL_DEBUG, "Could not find <e2hdd> element");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (; hddNode != nullptr; hddNode = hddNode->NextSiblingElement("e2hdd"))
  {
    std::string capacity;
    std::string freeSpace;
    std::string mount;

    XMLUtils::GetString(hddNode, "e2capacity", capacity);
    XMLUtils::GetString(hddNode, "e2free", freeSpace);
    XMLUtils::GetString(hddNode, "e2mount", mount);

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

  *iTotal = totalKb;
  *iUsed = totalKb - freeKb;

  Logger::Log(LEVEL_INFO, "GetDriveSpace Total: %lld, Used %lld", *iTotal, *iUsed);

  return PVR_ERROR_NO_ERROR;
}

long long Admin::GetKbFromString(const std::string &stringInMbGbTb) const
{
  long long sizeInKb = 0;

  static const std::vector<std::string> sizes = {"MB", "GB", "TB"};
  long multiplier = 1024;
  std::string replaceWith = "";
  for (const std::string& size : sizes)
  {
    std::regex regexSize ("^.* " + size);
    std::regex regexReplaceSize (" " + size);

    if (regex_match(stringInMbGbTb, regexSize))
    {
      double sizeValue = atof(regex_replace(stringInMbGbTb, regexReplaceSize, replaceWith).c_str());

      sizeInKb += static_cast<long long>(sizeValue * multiplier);

      break;
    }

    multiplier = multiplier * 1024;
  }

  return sizeInKb;
}

PVR_ERROR Admin::GetTunerSignal(PVR_SIGNAL_STATUS &signalStatus)
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/tunersignal"); 

  const std::string strXML = WebUtils::GetHttpXML(url);
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to parse XML: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  std::string snrDb;
  std::string snrPercentage;
  std::string ber;
  std::string signalStrength;

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElem;
  TiXmlHandle hRoot(0);

  pElem = hDoc.FirstChildElement("e2frontendstatus").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2frontendstatus> element!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (!XMLUtils::GetString(pElem, "e2snrdb", snrDb)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2snrdb from result!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (!XMLUtils::GetString(pElem, "e2snr", snrPercentage)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2snr from result!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }  

  if (!XMLUtils::GetString(pElem, "e2ber", ber)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2ber from result!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }  

  if (!XMLUtils::GetString(pElem, "e2acg", signalStrength)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2acg from result!", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }  

  std::regex regexReplacePercent (" %");
  std::string regexReplace = "";

  signalStatus.iSNR = atoi(regex_replace(snrPercentage, regexReplacePercent, regexReplace).c_str());
  signalStatus.iBER = atol(ber.c_str());
  signalStatus.iSignal = atoi(regex_replace(signalStrength, regexReplacePercent, regexReplace).c_str());

  if (CanUseJsonApi())
  {
    GetTunerDetails(signalStatus);
  }

  return PVR_ERROR_NO_ERROR;
}  

bool Admin::CanUseJsonApi()
{
  return Settings::GetInstance().GetWebIfVersionAsNum() >= Settings::GetInstance().GenerateWebIfVersionAsNum(1, 3, 0) && StringUtils::StartsWith(Settings::GetInstance().GetWebIfVersion(), "OWIF");
}

void Admin::GetTunerDetails(PVR_SIGNAL_STATUS &signalStatus)
{  
  const std::string jsonUrl = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "api/tunersignal"); 

  const std::string strJson = WebUtils::GetHttpXML(jsonUrl);

  auto jsonDoc = json::parse(strJson);

  if (!jsonDoc)
    {
      Logger::Log(LEVEL_DEBUG, "%s Invalid JSON received, cannot load extra tunerdetails from OpenWebIf", __FUNCTION__);
          
      return;
    }

  for (json::iterator it = jsonDoc.begin(); it != jsonDoc.end(); ++it) 
  {
    if (it.key() == "tunernumber")
    { 
      Logger::Log(LEVEL_DEBUG, "%s Json API - %s : %d", __FUNCTION__, it.key().c_str(), it.value().get<int>());

      int tunerNumber = it.value().get<int>();

      if (m_tuners.size() > tunerNumber)
      {
        Tuner &tuner = m_tuners.at(tunerNumber);

        strncpy(signalStatus.strAdapterName, (tuner.m_tunerName + " - " + tuner.m_tunerModel).c_str(), sizeof(signalStatus.strAdapterName));
      }
    }
    else if (it.key() == "tunertype")
    {
      Logger::Log(LEVEL_DEBUG, "%s Json API - %s : %s", __FUNCTION__, it.key().c_str(), it.value().get<std::string>().c_str());
      strncpy(signalStatus.strAdapterStatus, it.value().get<std::string>().c_str(), sizeof(signalStatus.strAdapterStatus));
    }
  }
}