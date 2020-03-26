/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
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
    class WebUtils
    {
    public:
      static std::string URLEncodeInline(const std::string& sStr);
      static bool CheckHttp(const std::string& url);
      static std::string GetHttp(const std::string& url);
      static std::string GetHttpXML(const std::string& url);
      static std::string PostHttpJson(const std::string& url);
      static bool SendSimpleCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult = false);
      static bool SendSimpleJsonCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult = false);
      static bool SendSimpleJsonPostCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult = false);
      static std::string& Escape(std::string& s, const std::string from, const std::string to);
    };
  } // namespace utilities
} // namespace enigma2
