/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "WebUtils.h"

#include "../Settings.h"
#include "CurlFile.h"
#include "Logger.h"

#include <kodi/util/XMLUtils.h>
#include <p8-platform/util/StringUtils.h>
#include <tinyxml.h>

using namespace enigma2;
using namespace enigma2::utilities;

const char SAFE[256] =
{
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

std::string WebUtils::URLEncodeInline(const std::string& sSrc)
{
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const unsigned char* pSrc = (const unsigned char*)sSrc.c_str();
  const int SRC_LEN = sSrc.length();
  unsigned char* const pStart = new unsigned char[SRC_LEN * 3];
  unsigned char* pEnd = pStart;
  const unsigned char* const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (SAFE[*pSrc])
      *pEnd++ = *pSrc;
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char*)pStart, (char*)pEnd);
  delete[] pStart;
  return sResult;
}

bool WebUtils::CheckHttp(const std::string& url)
{
  Logger::Log(LEVEL_TRACE, "%s Check webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  CurlFile http;
  if (!http.Check(url))
  {
    Logger::Log(LEVEL_TRACE, "%s - Could not open webAPI.", __FUNCTION__);
    return false;
  }

  Logger::Log(LEVEL_TRACE, "%s WebAPI available", __FUNCTION__);

  return true;
}

std::string WebUtils::GetHttp(const std::string& url)
{
  Logger::Log(LEVEL_DEBUG, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  std::string strTmp;

  CurlFile http;
  if (!http.Get(url, strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s - Could not open webAPI.", __FUNCTION__);
    return "";
  }

  Logger::Log(LEVEL_DEBUG, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());

  return strTmp;
}

std::string WebUtils::GetHttpXML(const std::string& url)
{
  std::string strTmp = GetHttp(url);

  // If there is no newline add it as it not being there will cause a parse error
  // TODO: Remove once bug is fixed in Open WebIf
  if (strTmp.back() != '\n')
    strTmp += "\n";

  return strTmp;
}

std::string WebUtils::PostHttpJson(const std::string& url)
{
  Logger::Log(LEVEL_DEBUG, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  std::string strTmp;

  CurlFile http;
  if (!http.Post(url, strTmp))
  {
    Logger::Log(LEVEL_ERROR, "%s - Could not open webAPI.", __FUNCTION__);
    return "";
  }

  // If there is no newline add it as it not being there will cause a parse error
  // TODO: Remove once bug is fixed in Open WebIf
  if (strTmp.back() != '\n')
    strTmp += "\n";

  Logger::Log(LEVEL_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());

  return strTmp;
}

bool WebUtils::SendSimpleCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), strCommandURL.c_str());

  const std::string strXML = WebUtils::GetHttpXML(url);

  if (!bIgnoreResult)
  {

    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      Logger::Log(LEVEL_ERROR, "%s Unable to parse XML: %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);

    pElem = hDoc.FirstChildElement("e2simplexmlresult").Element();

    if (!pElem)
    {
      Logger::Log(LEVEL_ERROR, "%s Could not find <e2simplexmlresult> element!", __FUNCTION__);
      return false;
    }

    bool bTmp;

    if (!XMLUtils::GetBoolean(pElem, "e2state", bTmp))
    {
      Logger::Log(LEVEL_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      strResultText = StringUtils::Format("Could not parse e2state!");
      return false;
    }

    if (!XMLUtils::GetString(pElem, "e2statetext", strResultText))
    {
      Logger::Log(LEVEL_ERROR, "%s Could not parse e2state from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
      Logger::Log(LEVEL_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());

    return bTmp;
  }
  return true;
}

bool WebUtils::SendSimpleJsonCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), strCommandURL.c_str());

  const std::string strJson = WebUtils::GetHttp(url);

  if (!bIgnoreResult)
  {
    if (strJson.find("\"result\": true") != std::string::npos)
    {
      strResultText = "Success!";
    }
    else
    {
      strResultText = StringUtils::Format("Invalid Command");
      Logger::Log(LEVEL_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());
      return false;
    }
  }

  return true;
}

bool WebUtils::SendSimpleJsonPostCommand(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), strCommandURL.c_str());

  const std::string strJson = WebUtils::PostHttpJson(url);

  if (!bIgnoreResult)
  {
    if (strJson.find("\"result\": true") != std::string::npos)
    {
      strResultText = "Success!";
    }
    else
    {
      strResultText = StringUtils::Format("Invalid Command");
      Logger::Log(LEVEL_ERROR, "%s Error message from backend: '%s'", __FUNCTION__, strResultText.c_str());
      return false;
    }
  }

  return true;
}

std::string& WebUtils::Escape(std::string& s, const std::string from, const std::string to)
{
  std::string::size_type pos = -1;
  while ((pos = s.find(from, pos + 1)) != std::string::npos)
    s.erase(pos, from.length()).insert(pos, to);

  return s;
}