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
    static const std::string HTTP_PREFIX = "http://";
    static const std::string HTTPS_PREFIX = "https://";

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
      static const std::string UrlEncode(const std::string& value);
      static std::string ReadFileContentsStartOnly(const std::string& url, int* httpCode);
      static bool IsHttpUrl(const std::string& url);
      static std::string RedactUrl(const std::string& url);
    };
  } // namespace utilities
} // namespace enigma2
