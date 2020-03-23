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

#pragma once

#include "../../client.h"

#include <string>

namespace enigma2
{
  namespace utilities
  {
    class CurlFile
    {
    public:
      CurlFile() {};
      ~CurlFile() {};

      bool Get(const std::string& strURL, std::string& strResult);
      bool Post(const std::string& strURL, std::string& strResult);
      bool Check(const std::string& strURL);
    };
  } // namespace utilities
} // namespace enigma2
