/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/Filesystem.h>
#include <string>
#include <vector>

namespace enigma2
{
  namespace utilities
  {
    class FileUtils
    {
    public:
      static bool FileExists(const std::string& file);
      static bool CopyFile(const std::string& sourceFile, const std::string& targetFile);
      static bool WriteStringToFile(const std::string& fileContents, const std::string& targetFile);
      static std::string ReadFileToString(const std::string& sourceFile);
      static std::string ReadXmlFileToString(const std::string& sourceFile);
      static bool CopyDirectory(const std::string& sourceDir, const std::string& targetDir, bool recursiveCopy);
      static std::vector<std::string> GetFilesInDirectory(const std::string& dir);
      static std::string GetResourceDataPath();

    private:
      static std::string ReadFileContents(kodi::vfs::CFile& fileHandle);
    };
  } // namespace utilities
} // namespace enigma2
