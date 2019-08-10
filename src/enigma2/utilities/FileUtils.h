#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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
      static std::string ReadFileContents(void* fileHandle);
    };
  } // namespace utilities
} // namespace enigma2
