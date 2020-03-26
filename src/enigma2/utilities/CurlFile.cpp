/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "CurlFile.h"

#include "../Settings.h"
#include "Logger.h"

#include <cstdarg>

using namespace enigma2::utilities;

bool CurlFile::Get(const std::string& strURL, std::string& strResult)
{
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
      strResult.append(buffer);
    XBMC->CloseFile(fileHandle);
    return true;
  }
  return false;
}

bool CurlFile::Post(const std::string& strURL, std::string& strResult)
{
  void* fileHandle = XBMC->CURLCreate(strURL.c_str());

  if (!fileHandle)
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to create curl handle for %s", __FUNCTION__, strURL.c_str());
    return false;
  }

  XBMC->CURLAddOption(fileHandle, XFILE::CURL_OPTION_PROTOCOL, "postdata", "POST");

  if (!XBMC->CURLOpen(fileHandle, XFILE::READ_NO_CACHE))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to open url: %s", __FUNCTION__, strURL.c_str());
    XBMC->CloseFile(fileHandle);
    return false;
  }

  char buffer[1024];
  while (XBMC->ReadFileString(fileHandle, buffer, 1024))
    strResult.append(buffer);
  XBMC->CloseFile(fileHandle);

  if (!strResult.empty())
    return true;

  return false;
}

bool CurlFile::Check(const std::string& strURL)
{
  void* fileHandle = XBMC->CURLCreate(strURL.c_str());

  if (!fileHandle)
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to create curl handle for %s", __FUNCTION__, strURL.c_str());
    return false;
  }

  XBMC->CURLAddOption(fileHandle, XFILE::CURL_OPTION_PROTOCOL, "connection-timeout",
                      std::to_string(Settings::GetInstance().GetConnectioncCheckTimeoutSecs()).c_str());

  if (!XBMC->CURLOpen(fileHandle, XFILE::READ_NO_CACHE))
  {
    Logger::Log(LEVEL_TRACE, "%s Unable to open url: %s", __FUNCTION__, strURL.c_str());
    XBMC->CloseFile(fileHandle);
    return false;
  }

  XBMC->CloseFile(fileHandle);

  return true;
}