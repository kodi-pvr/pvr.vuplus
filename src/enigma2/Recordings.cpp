#include "Recordings.h"

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

void Recordings::GetRecordings(std::vector<PVR_RECORDING> &kodiRecordings)
{
  for (auto& recording : m_recordings)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer recording '%s', Recording Id '%s'", __FUNCTION__, recording.GetTitle().c_str(), recording.GetRecordingId().c_str());
    PVR_RECORDING kodiRecording;
    memset(&kodiRecording, 0, sizeof(PVR_RECORDING));

    recording.UpdateTo(kodiRecording, m_channels, IsInRecordingFolder(recording.GetTitle()));

    kodiRecordings.emplace_back(kodiRecording);
  }
}

int Recordings::GetNumRecordings() const
{
  return m_recordings.size();
}

void Recordings::ClearRecordings()
{
  m_recordings.clear();
}

bool Recordings::IsInRecordingFolder(const std::string &recordingFolder) const
{
  int iMatches = 0;
  for (const auto& recording : m_recordings)
  {
    if (recordingFolder == recording.GetTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found Recording title '%s' in recordings vector!", __FUNCTION__, recordingFolder.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found Recording title twice '%s' in recordings vector!", __FUNCTION__, recordingFolder.c_str());
        return true;    
      }
    }
  }

  return false;
}

PVR_ERROR Recordings::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  const std::string strTmp = StringUtils::Format("web/moviedelete?sRef=%s", WebUtils::URLEncodeInline(recinfo.strRecordingId).c_str());

  std::string strResult;
  if(!WebUtils::SendSimpleCommand(strTmp, strResult)) 
    return PVR_ERROR_FAILED;

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

const std::string Recordings::GetRecordingURL(const PVR_RECORDING &recinfo)
{
  for (const auto& recording : m_recordings)
  {
    if (recinfo.strRecordingId == recording.GetRecordingId())
      return recording.GetStreamURL();
  }
  return "";
}

std::vector<std::string>& Recordings::GetLocations()
{
  return m_locations;
}

void Recordings::ClearLocations()
{
  m_locations.clear();
}

bool Recordings::LoadLocations()
{
  std::string url;
  if (Settings::GetInstance().GetRecordingsFromCurrentLocationOnly())
    url = StringUtils::Format("%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/getcurrlocation"); 
  else 
    url = StringUtils::Format("%s%s",  Settings::GetInstance().GetConnectionURL().c_str(), "web/getlocations"); 
 
  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2locations").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2locations> element", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2location> element", __FUNCTION__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2location"))
  {
    const std::string strTmp = pNode->GetText();

    m_locations.emplace_back(strTmp);

    Logger::Log(LEVEL_DEBUG, "%s Added '%s' as a recording location", __FUNCTION__, strTmp.c_str());
  }

  Logger::Log(LEVEL_INFO, "%s Loaded '%d' recording locations", __FUNCTION__, m_locations.size());

  return true;
}

void Recordings::LoadRecordings()
{
  ClearRecordings();

  for (const auto& location : m_locations)
  {
    if (!GetRecordingsFromLocation(location))
    {
      Logger::Log(LEVEL_ERROR, "%s Error fetching lists for folder: '%s'", __FUNCTION__, location.c_str());
    }
  }
}

bool Recordings::GetRecordingsFromLocation(std::string recordingLocation)
{
  std::string url;
  std::string directory;

  if (recordingLocation == "default")
  {
    url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/movielist"); 
    directory = StringUtils::Format("/");
  }
  else 
  {
    url = StringUtils::Format("%s%s?dirname=%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/movielist", 
                                          WebUtils::URLEncodeInline(recordingLocation).c_str()); 
    directory = recordingLocation;
  }
 
  const std::string strXML = WebUtils::GetHttpXML(url);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <e2movielist> element!", __FUNCTION__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  int iNumRecordings = 0; 

  if (!pNode)
  {
    Logger::Log(LEVEL_DEBUG, "%s Could not find <e2movie> element, no movies at location: %s", directory.c_str(), __FUNCTION__);
  }  
  else
  {  
    for (; pNode != nullptr; pNode = pNode->NextSiblingElement("e2movie"))
    {

      RecordingEntry recordingEntry;

      if (!recordingEntry.UpdateFrom(pNode, directory, m_channels))
        continue;

      if (m_entryExtractor.IsEnabled())
        m_entryExtractor.ExtractFromEntry(recordingEntry);

      iNumRecordings++;

      m_recordings.emplace_back(recordingEntry);

      Logger::Log(LEVEL_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, recordingEntry.GetTitle().c_str(), recordingEntry.GetStartTime(), recordingEntry.GetDuration());
    }

    Logger::Log(LEVEL_INFO, "%s Loaded %u Recording Entries from folder '%s'", __FUNCTION__, iNumRecordings, recordingLocation.c_str());
  }
  return true;
}