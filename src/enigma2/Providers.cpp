/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Providers.h"

#include "../Enigma2.h"
#include "utilities/Logger.h"
#include "utilities/FileUtils.h"

#include <kodi/tools/StringUtils.h>
#include <tinyxml.h>

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;
using namespace kodi::tools;

Providers::Providers()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + PROVIDER_DIR, PROVIDER_ADDON_DATA_BASE_DIR, true);

  std::string providerMappingsFile = Settings::GetInstance().GetProviderNameMapFile();
  if (LoadProviderMappingFile(providerMappingsFile))
    Logger::Log(LEVEL_INFO, "%s - Loaded '%d' providers mappings", __func__, m_providerMappingsMap.size());
  else
    Logger::Log(LEVEL_ERROR, "%s - could not load provider mappings XML file: %s", __func__, providerMappingsFile.c_str());
}

void Providers::GetProviders(std::vector<kodi::addon::PVRProvider>& kodiProviders) const
{
  for (const auto& provider : m_providers)
  {
    kodi::addon::PVRProvider kodiProvider;

    provider->UpdateTo(kodiProvider);

    Logger::Log(LEVEL_DEBUG, "%s - Transfer provider '%s', unique id '%d'", __func__,
                provider->GetProviderName().c_str(), provider->GetUniqueId());

    kodiProviders.emplace_back(kodiProvider);
  }
}

int Providers::GetProviderUniqueId(const std::string& providerName)
{
  std::shared_ptr<Provider> provider = GetProvider(providerName);
  int uniqueId = PVR_PROVIDER_INVALID_UID;

  if (provider)
    uniqueId = provider->GetUniqueId();

  return uniqueId;
}

std::shared_ptr<Provider> Providers::GetProvider(int uniqueId)
{
  auto providerPair = m_providersUniqueIdMap.find(uniqueId);
  if (providerPair != m_providersUniqueIdMap.end())
    return providerPair->second;

  return {};
}

std::shared_ptr<Provider> Providers::GetProvider(const std::string& providerName)
{
  auto providerPair = m_providersNameMap.find(providerName);
  if (providerPair != m_providersNameMap.end())
    return providerPair->second;

  return {};
}

bool Providers::IsValid(int uniqueId)
{
  return GetProvider(uniqueId) != nullptr;
}

bool Providers::IsValid(const std::string &providerName)
{
  return GetProvider(providerName) != nullptr;
}

int Providers::GetNumProviders() const
{
  return m_providers.size();
}

void Providers::ClearProviders()
{
  m_providers.clear();
  m_providersUniqueIdMap.clear();
  m_providersNameMap.clear();
}

std::shared_ptr<Provider> Providers::AddProvider(const std::string& providerName)
{
  if (!providerName.empty())
  {
    Provider provider;
    std::string providerKey = providerName;
    StringUtils::ToLower(providerKey);

    auto providerPair = m_providerMappingsMap.find(providerKey);
    if (providerPair != m_providerMappingsMap.end())
    {
      provider = providerPair->second;
    }
    else
    {
      provider.SetProviderName(providerName);
    }

    return AddProvider(provider);
  }
  return {};
}

namespace
{

int GenerateProviderUniqueId(const std::string& providerName)
{
  std::string concat(providerName);

  const char* calcString = concat.c_str();
  int uniqueId = 0;
  int c;
  while ((c = *calcString++))
    uniqueId = ((uniqueId << 5) + uniqueId) + c; /* iId * 33 + c */

  return abs(uniqueId);
}

} // unnamed namespace

std::shared_ptr<Provider> Providers::AddProvider(Provider& newProvider)
{
  std::shared_ptr<Provider> foundProvider = GetProvider(newProvider.GetProviderName());

  if (!foundProvider)
  {
    newProvider.SetUniqueId(GenerateProviderUniqueId(newProvider.GetProviderName()));

    m_providers.emplace_back(new Provider(newProvider));

    std::shared_ptr<Provider> provider = m_providers.back();

    m_providersUniqueIdMap.insert({provider->GetUniqueId(), provider});
    m_providersNameMap.insert({provider->GetProviderName(), provider});

    return provider;
  }

  return foundProvider;
}

std::vector<std::shared_ptr<Provider>>& Providers::GetProvidersList()
{
  return m_providers;
}

bool Providers::LoadProviderMappingFile(const std::string& xmlFile)
{
  m_providerMappingsMap.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __func__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __func__, xmlFile.c_str());

  const std::string fileContents = FileUtils::ReadXmlFileToString(xmlFile);

  if (fileContents.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s No Content in XML file: %s", __func__, xmlFile.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(fileContents.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __func__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);

  TiXmlElement* pElem = hDoc.FirstChildElement("providerMappings").Element();

  if (!pElem)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <providerMappings> element!", __func__);
    return false;
  }

  TiXmlHandle hRoot = TiXmlHandle(pElem);

  TiXmlElement* pNode = hRoot.FirstChildElement("providerMapping").Element();

  if (!pNode)
  {
    Logger::Log(LEVEL_ERROR, "%s Could not find <providerMapping> element", __func__);
    return false;
  }

  for (; pNode != nullptr; pNode = pNode->NextSiblingElement("providerMapping"))
  {
    std::string mappedName = pNode->Attribute("mappedName") ? pNode->Attribute("mappedName") : "";

    if (!mappedName.empty())
    {
      StringUtils::ToLower(mappedName);
      Provider provider;

      TiXmlElement* childElem = pNode->FirstChildElement("name");
      if (childElem)
      {
        std::string name = childElem->GetText();
        if (name.empty())
        {
          Logger::Log(LEVEL_ERROR, "%s Could not read <name> element for provider mapping: %s", __func__, mappedName.c_str());
          return false;
        }

        provider.SetProviderName(childElem->GetText());
      }

      childElem = pNode->FirstChildElement("type");
      if (childElem)
      {
        std::string type = childElem->GetText();
        StringUtils::ToLower(type);
        if (type == "addon")
          provider.SetProviderType(PVR_PROVIDER_TYPE_ADDON);
        else if (type == "satellite")
          provider.SetProviderType(PVR_PROVIDER_TYPE_SATELLITE);
        else if (type == "cable")
          provider.SetProviderType(PVR_PROVIDER_TYPE_CABLE);
        else if (type == "aerial")
          provider.SetProviderType(PVR_PROVIDER_TYPE_AERIAL);
        else if (type == "iptv")
          provider.SetProviderType(PVR_PROVIDER_TYPE_IPTV);
        else
          provider.SetProviderType(PVR_PROVIDER_TYPE_UNKNOWN);
      }

      childElem = pNode->FirstChildElement("iconPath");
      if (childElem)
        provider.SetIconPath(childElem->GetText());

      childElem = pNode->FirstChildElement("countries");
      if (childElem)
      {
        std::vector<std::string> countries = StringUtils::Split(childElem->GetText(), PROVIDER_STRING_TOKEN_SEPARATOR);
        provider.SetCountries(countries);
      }

      childElem = pNode->FirstChildElement("languages");
      if (childElem)
      {
        std::vector<std::string> languages = StringUtils::Split(childElem->GetText(), PROVIDER_STRING_TOKEN_SEPARATOR);
        provider.SetLanguages(languages);
      }

      m_providerMappingsMap.insert({mappedName, provider});

      Logger::Log(LEVEL_TRACE, "%s Read Provider Mapping from: %s to %s", __func__, mappedName.c_str(), provider.GetProviderName().c_str());
    }
  }

  return true;
}