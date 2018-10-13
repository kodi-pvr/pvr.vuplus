#include "Admin.h"

#include <regex>

#include "../client.h"
#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include "util/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

void Admin::SendPowerstate()
{
  if (Settings::GetInstance().GetDeepStandbyOnAddonExit())
  {  
    std::string strTmp;
    strTmp = StringUtils::Format("web/powerstate?newstate=1");

    std::string strResult;
    WebUtils::SendSimpleCommand(strTmp, strResult, true); 
  }
}

bool Admin::GetDeviceInfo()
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/deviceinfo"); 

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
  m_strEnigmaVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2EnigmaVersion: %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  // Get ImageVersion
  if (!XMLUtils::GetString(pElem, "e2imageversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2imageversion from result!", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2ImageVersion: %s", __FUNCTION__, m_strImageVersion.c_str());

  // Get WebIfVersion
  if (!XMLUtils::GetString(pElem, "e2webifversion", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2webifversion from result!", __FUNCTION__);
    return false;
  }
  else
  {
    m_strWebIfVersion = strTmp.c_str();
    Logger::Log(LEVEL_NOTICE, "%s - E2WebIfVersion: %s", __FUNCTION__, m_strWebIfVersion.c_str());

    Settings::GetInstance().SetWebIfVersionAsNum(GetWebIfVersion(m_strWebIfVersion));
  }

  // Get DeviceName
  if (!XMLUtils::GetString(pElem, "e2devicename", strTmp)) 
  {
    Logger::Log(LEVEL_ERROR, "%s Could not parse e2devicename from result!", __FUNCTION__);
    return false;
  }
  m_strServerName = strTmp.c_str();
  Logger::Log(LEVEL_NOTICE, "%s - E2DeviceName: %s", __FUNCTION__, m_strServerName.c_str());

  return true;
}

unsigned int Admin::GetWebIfVersion(std::string versionString)
{
  unsigned int webIfVersion = 0;

  std::regex regex ("^.*[0-9]+\\.[0-9]+\\.[0-9].*$");
  if (regex_match(versionString, regex))
  {
    int count = 0;
    unsigned int versionPart = 0;
    std::regex pattern("([0-9]+)");
    for (auto i = std::sregex_iterator(versionString.begin(), versionString.end(), pattern); i != std::sregex_iterator(); ++i) 
    {
        switch (count)
        {
          case 0:
            versionPart = atoi(i->str().c_str());
            webIfVersion = versionPart << 16;
            break;
          case 1:
              versionPart = atoi(i->str().c_str());
              webIfVersion |= versionPart << 8;
            break;     
          case 2:
              versionPart = atoi(i->str().c_str());
              webIfVersion |= versionPart;
            break;      
        }      

        count++;
    }
  }

  return webIfVersion;
}

const std::string& Admin::GetServerName() const
{
  return m_strServerName;
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
  long multiplier = 1024 * 1024;
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