/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "FileUtils.h"

#include "../../client.h"
#include "Logger.h"

#include <kodi/kodi_vfs_types.h>

using namespace enigma2;
using namespace enigma2::utilities;

bool FileUtils::FileExists(const std::string& file)
{
  return XBMC->FileExists(file.c_str(), false);
}

bool FileUtils::CopyFile(const std::string& sourceFile, const std::string& targetFile)
{
  bool copySuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s Copying file: %s, to %s", __FUNCTION__, sourceFile.c_str(), targetFile.c_str());

  void* sourceFileHandle = XBMC->OpenFile(sourceFile.c_str(), 0x08); //READ_NO_CACHE

  if (sourceFileHandle)
  {
    const std::string fileContents = ReadFileContents(sourceFileHandle);

    XBMC->CloseFile(sourceFileHandle);

    void* targetFileHandle = XBMC->OpenFileForWrite(targetFile.c_str(), true);

    if (targetFileHandle)
    {
      XBMC->WriteFile(targetFileHandle, fileContents.c_str(), fileContents.length());
      XBMC->CloseFile(targetFileHandle);
    }
    else
    {
      Logger::Log(LEVEL_ERROR, "%s Could not open target file to copy to: %s", __FUNCTION__, targetFile.c_str());
      copySuccessful = false;
    }
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open source file to copy: %s", __FUNCTION__, sourceFile.c_str());
    copySuccessful = false;
  }

  return copySuccessful;
}

bool FileUtils::WriteStringToFile(const std::string& fileContents, const std::string& targetFile)
{
  bool writeSuccessful = true;

  Logger::Log(LEVEL_DEBUG, "%s Writing strig to file: %s", __FUNCTION__, targetFile.c_str());

  void* targetFileHandle = XBMC->OpenFileForWrite(targetFile.c_str(), true);

  if (targetFileHandle)
  {
    XBMC->WriteFile(targetFileHandle, fileContents.c_str(), fileContents.length());
    XBMC->CloseFile(targetFileHandle);
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open target file to write to: %s", __FUNCTION__, targetFile.c_str());
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

  Logger::Log(LEVEL_DEBUG, "%s Reading file to string: %s", __FUNCTION__, sourceFile.c_str());

  void* sourceFileHandle = XBMC->OpenFile(sourceFile.c_str(), 0x08); //READ_NO_CACHE

  if (sourceFileHandle)
  {
    fileContents = ReadFileContents(sourceFileHandle);

    XBMC->CloseFile(sourceFileHandle);
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not open source file to read: %s", __FUNCTION__, sourceFile.c_str());
  }

  return fileContents;
}

std::string FileUtils::ReadFileContents(void* fileHandle)
{
  std::string fileContents;

  char buffer[1024];
  int bytesRead = 0;

  // Read until EOF or explicit error
  while ((bytesRead = XBMC->ReadFile(fileHandle, buffer, sizeof(buffer) - 1)) > 0)
    fileContents.append(buffer, bytesRead);

  return fileContents;
}

bool FileUtils::CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy)
{
  bool copySuccessful = true;

  XBMC->CreateDirectory(targetDir.c_str());

  VFSDirEntry* entries;
  unsigned int numEntries;

  if (XBMC->GetDirectory(sourceDir.c_str(), "", &entries, &numEntries))
  {
    for (int i = 0; i < numEntries; i++)
    {
      if (entries[i].folder && recursiveCopy)
      {
        copySuccessful = CopyDirectory(sourceDir + "/" + entries[i].label, targetDir + "/" + entries[i].label, true);
      }
      else if (!entries[i].folder)
      {
        copySuccessful = CopyFile(sourceDir + "/" + entries[i].label, targetDir + "/" + entries[i].label);
      }
    }

    XBMC->FreeDirectory(entries, numEntries);
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not copy directory: %s, to directory: %s", __FUNCTION__, sourceDir.c_str(), targetDir.c_str());
    copySuccessful = false;
  }
  return copySuccessful;
}

std::vector<std::string> FileUtils::GetFilesInDirectory(const std::string& dir)
{
  std::vector<std::string> files;

  VFSDirEntry* entries;
  unsigned int numEntries;

  if (XBMC->GetDirectory(dir.c_str(), "", &entries, &numEntries))
  {
    for (int i = 0; i < numEntries; i++)
    {
      if (!entries[i].folder)
      {
        files.emplace_back(entries[i].label);
      }
    }

    XBMC->FreeDirectory(entries, numEntries);
  }
  else
  {
    Logger::Log(LEVEL_ERROR, "%s Could not get files in directory: %s", __FUNCTION__, dir.c_str());
  }

  return files;
}

std::string FileUtils::GetResourceDataPath()
{
  char path[1024];
  XBMC->GetSetting("__addonpath__", path);
  std::string resourcesDataPath = path;
  resourcesDataPath += "/resources/data";

  return resourcesDataPath;
}
