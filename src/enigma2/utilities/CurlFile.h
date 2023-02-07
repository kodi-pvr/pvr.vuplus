/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

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
      bool Check(const std::string& strURL, int connectionTimeoutSecs);
    };
  } // namespace utilities
} // namespace enigma2
