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

#include <kodi/Filesystem.h>

using namespace enigma2::utilities;

bool CurlFile::Get(const std::string& strURL, std::string& strResult)
{
  kodi::vfs::CFile fileHandle;
  if (fileHandle.OpenFile(strURL))
  {
    std::string buffer;
    while (fileHandle.ReadLine(buffer))
      strResult.append(buffer);
    return true;
  }
  return false;
}

bool CurlFile::Post(const std::string& strURL, std::string& strResult)
{
  kodi::vfs::CFile fileHandle;
  if (!fileHandle.CURLCreate(strURL))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to create curl handle for %s", __func__, strURL.c_str());
    return false;
  }

  fileHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "postdata", "POST");

  if (!fileHandle.CURLOpen(ADDON_READ_NO_CACHE))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to open url: %s", __func__, strURL.c_str());
    return false;
  }

  std::string buffer;
  while (fileHandle.ReadLine(buffer))
    strResult.append(buffer);

  if (!strResult.empty())
    return true;

  return false;
}

bool CurlFile::Check(const std::string& strURL)
{
  kodi::vfs::CFile fileHandle;
  if (!fileHandle.CURLCreate(strURL))
  {
    Logger::Log(LEVEL_ERROR, "%s Unable to create curl handle for %s", __func__, strURL.c_str());
    return false;
  }

  fileHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "connection-timeout",
                      std::to_string(Settings::GetInstance().GetConnectioncCheckTimeoutSecs()));

  if (!fileHandle.CURLOpen(ADDON_READ_NO_CACHE))
  {
    Logger::Log(LEVEL_TRACE, "%s Unable to open url: %s", __func__, strURL.c_str());
    return false;
  }

  return true;
}
