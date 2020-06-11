/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FileUtils.h"

#include "Logger.h"

#include <kodi/Filesystem.h>

using namespace enigma2;
using namespace enigma2::utilities;

bool FileUtils::FileExists(const std::string& file)
{
  return kodi::vfs::FileExists(file, false);
}

bool FileUtils::CopyFile(const std::string& sourceFile, const std::string& targetFile)
{
  bool copySuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s Copying file: %s, to %s", __func__, sourceFile.c_str(), targetFile.c_str());

  kodi::vfs::CFile sourceFileHandle;

  if (sourceFileHandle.OpenFile(sourceFile, ADDON_READ_NO_CACHE))
  {
    const std::string fileContents = ReadFileContents(sourceFileHandle);

    sourceFileHandle.Close();

    kodi::vfs::CFile targetFileHandle;

    if (targetFileHandle.OpenFileForWrite(targetFile, true))
    {
      targetFileHandle.Write(fileContents.c_str(), fileContents.length());
      targetFileHandle.Close();
    }
    else
    {
      Logger::Log(LEVEL_ERROR, "%s Could not open target file to copy to: %s", __func__, targetFile.c_str());
      copySuccessful = false;
    }
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open source file to copy: %s", __func__, sourceFile.c_str());
    copySuccessful = false;
  }

  return copySuccessful;
}

bool FileUtils::WriteStringToFile(const std::string& fileContents, const std::string& targetFile)
{
  bool writeSuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s Writing strig to file: %s", __func__, targetFile.c_str());

  kodi::vfs::CFile targetFileHandle;

  if (targetFileHandle.OpenFileForWrite(targetFile, true))
  {
    targetFileHandle.Write(fileContents.c_str(), fileContents.length());
    targetFileHandle.Close();
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open target file to write to: %s", __func__, targetFile.c_str());
    writeSuccessful = false;
  }

  return writeSuccessful;
}

std::string FileUtils::ReadXmlFileToString(const std::string& sourceFile)
{
  return ReadFileToString(sourceFile) + "\n";
}

std::string FileUtils::ReadFileToString(const std::string& sourceFile)
{
  std::string fileContents;

  Logger::Log(LEVEL_DEBUG, "%s Reading file to string: %s", __func__, sourceFile.c_str());

  kodi::vfs::CFile sourceFileHandle;

  if (sourceFileHandle.OpenFile(sourceFile, ADDON_READ_NO_CACHE))
  {
    fileContents = ReadFileContents(sourceFileHandle);

    sourceFileHandle.Close();
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open source file to read: %s", __func__, sourceFile.c_str());
  }

  return fileContents;
}

std::string FileUtils::ReadFileContents(kodi::vfs::CFile& fileHandle)
{
  std::string fileContents;

  char buffer[1024];
  int bytesRead = 0;

  // Read until EOF or explicit error
  while ((bytesRead = fileHandle.Read(buffer, sizeof(buffer) - 1)) > 0)
    fileContents.append(buffer, bytesRead);

  return fileContents;
}

bool FileUtils::CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy)
{
  bool copySuccessful = true;

  kodi::vfs::CreateDirectory(targetDir);

  std::vector<kodi::vfs::CDirEntry> entries;

  if (kodi::vfs::GetDirectory(sourceDir, "", entries))
  {
    for (const auto& entry : entries)
    {
      if (entry.IsFolder() && recursiveCopy)
      {
        copySuccessful = CopyDirectory(sourceDir + "/" + entry.Label(), targetDir + "/" + entry.Label(), true);
      }
      else if (!entry.IsFolder())
      {
        copySuccessful = CopyFile(sourceDir + "/" + entry.Label(), targetDir + "/" + entry.Label());
      }
    }
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not copy directory: %s, to directory: %s", __func__, sourceDir.c_str(), targetDir.c_str());
    copySuccessful = false;
  }
  return copySuccessful;
}

std::vector<std::string> FileUtils::GetFilesInDirectory(const std::string& dir)
{
  std::vector<std::string> files;

  std::vector<kodi::vfs::CDirEntry> entries;

  if (kodi::vfs::GetDirectory(dir, "", entries))
  {
    for (const auto& entry : entries)
    {
      if (entry.IsFolder())
      {
        files.emplace_back(entry.Label());
      }
    }
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not get files in directory: %s", __func__, dir.c_str());
  }

  return files;
}

std::string FileUtils::GetResourceDataPath()
{
  return kodi::GetAddonPath("/resources/data");
}
