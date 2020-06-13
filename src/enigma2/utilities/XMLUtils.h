/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <tinyxml.h>
#include <vector>

namespace enigma2
{
namespace utilities
{
namespace xml
{

//==============================================================================
/// @brief Returns true if the encoding of the document is specified as as UTF-8
///
/// @param[in] strXML The XML file (embedded in a string) to check.
/// @return true if UTF8
///
inline bool HasUTF8Declaration(const std::string& strXML)
{
  std::string test = strXML;
  std::transform(test.begin(), test.end(), test.begin(), ::tolower);
  // test for the encoding="utf-8" string
  if (test.find("encoding=\"utf-8\"") != std::string::npos)
    return true;
  // TODO: test for plain UTF8 here?
  return false;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief Check given tag on XML has a child.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag to check
/// @return true if child present.
///
inline bool HasChild(const TiXmlNode* pRootNode, const std::string& strTag)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement)
    return false;
  const TiXmlNode* pNode = pElement->FirstChild();
  return (pNode != nullptr);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief Returns true if the encoding of the document is other then UTF-8.
///
/// @param[in] pDoc TinyXML related document field
/// @param[in] strEncoding Returns the encoding of the document. Empty if UTF-8
/// @return String of encoding if present and not UTF-8
///
inline bool GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding)
{
  const TiXmlNode* pNode = nullptr;
  while ((pNode = pDoc->IterateChildren(pNode)) && pNode->Type() != TiXmlNode::TINYXML_DECLARATION)
  {
  }
  if (!pNode)
    return false;
  const TiXmlDeclaration* pDecl = pNode->ToDeclaration();
  if (!pDecl)
    return false;
  strEncoding = pDecl->Encoding();
  std::transform(strEncoding.begin(), strEncoding.end(), strEncoding.begin(), ::toupper);

  if (strEncoding == "UTF-8" || strEncoding == "UTF8")
    strEncoding.clear();

  return !strEncoding.empty(); // Other encoding then UTF8?
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a text string value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetString(const TiXmlNode* pRootNode, const std::string& strTag, std::string& value)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement)
    return false;
  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != nullptr)
  {
    value = pNode->Value();
    return true;
  }
  value.clear();
  return false;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a HEX formated integer string inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetHex(const TiXmlNode* pRootNode, const std::string& strTag, uint32_t& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  value = strtoul(pNode->FirstChild()->Value(), nullptr, 16);
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a 32 bit unsigned integer value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetUInt(const TiXmlNode* pRootNode, const std::string& strTag, uint32_t& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  value = atol(pNode->FirstChild()->Value());
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a 32 bit integer value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetInt(const TiXmlNode* pRootNode, const std::string& strTag, int32_t& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  value = atoi(pNode->FirstChild()->Value());
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a 32 bit integer value stored inside XML.
///
/// This can define min and max values if content out of wanted range.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @param[in] min Minimal return value
/// @param[in] max Maximal return value
/// @return true if available and successfully done
///
inline bool GetInt(const TiXmlNode* pRootNode,
                   const std::string& strTag,
                   int32_t& value,
                   const int32_t min,
                   const int32_t max)
{
  if (GetInt(pRootNode, strTag, value))
  {
    if (value < min)
      value = min;
    if (value > max)
      value = max;
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a 64 bit integer value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetLong(const TiXmlNode* pRootNode, const std::string& strTag, int64_t& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  value = atoll(pNode->FirstChild()->Value());
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a floating point value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetFloat(const TiXmlNode* pRootNode, const std::string& strTag, float& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  value = atof(pNode->FirstChild()->Value());
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a floating point value stored inside XML.
///
/// This can define min and max values if content out of wanted range.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @param[in] min Minimal return value
/// @param[in] max Maximal return value
/// @return true if available and successfully done
///
inline bool GetFloat(const TiXmlNode* pRootNode,
                     const std::string& strTag,
                     float& value,
                     const float min,
                     const float max)
{
  if (GetFloat(pRootNode, strTag, value))
  { // check range
    if (value < min)
      value = min;
    if (value > max)
      value = max;
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a double floating point value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetDouble(const TiXmlNode* pRootNode, const std::string& strTag, double& value)
{
  const TiXmlNode* node = pRootNode->FirstChild(strTag);
  if (!node || !node->FirstChild())
    return false;
  value = atof(node->FirstChild()->Value());
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a boolean value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed value from XML
/// @return true if available and successfully done
///
inline bool GetBoolean(const TiXmlNode* pRootNode, const std::string& strTag, bool& value)
{
  const TiXmlNode* pNode = pRootNode->FirstChild(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  std::string strEnabled = pNode->FirstChild()->Value();
  std::transform(strEnabled.begin(), strEnabled.end(), strEnabled.begin(), ::tolower);
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" ||
      strEnabled == "false" || strEnabled == "0")
    value = false;
  else
  {
    value = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" &&
        strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To get a path value stored inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[out] value The readed path value from XML
/// @return true if available and successfully done
///
inline bool GetPath(const TiXmlNode* pRootNode, const std::string& strTag, std::string& value)
{
  const TiXmlElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement)
    return false;

  const TiXmlNode* pNode = pElement->FirstChild();
  if (pNode != nullptr)
  {
    value = pNode->Value();
    return true;
  }
  value.clear();
  return false;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a text string inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetString(TiXmlNode* pRootNode, const std::string& strTag, const std::string& value)
{
  TiXmlElement newElement(strTag);
  TiXmlNode* pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText xmlValue(value);
    pNewNode->InsertEndChild(xmlValue);
  }
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a list of strings inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] arrayValue List with string values to store
///
inline void SetStringArray(TiXmlNode* pRootNode,
                           const std::string& strTag,
                           const std::vector<std::string>& arrayValue)
{
  for (const auto& value : arrayValue)
    SetString(pRootNode, strTag, value);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a HEX formated integer string inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetHex(TiXmlNode* pRootNode, const std::string& strTag, uint32_t value)
{
  size_t size = snprintf(nullptr, 0, "%x", value) + 1;
  std::unique_ptr<char[]> strValue(new char[size]);
  snprintf(strValue.get(), size, "%x", value);
  SetString(pRootNode, strTag, std::string(strValue.get(), strValue.get() + size - 1));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a 32 bit unsigned integer value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetUInt(TiXmlNode* pRootNode, const std::string& strTag, uint32_t value)
{
  SetString(pRootNode, strTag, std::to_string(value));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a 32 bit integer value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetInt(TiXmlNode* pRootNode, const std::string& strTag, int32_t value)
{
  SetString(pRootNode, strTag, std::to_string(value));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a 64 bit integer value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetLong(TiXmlNode* pRootNode, const std::string& strTag, int64_t value)
{
  SetString(pRootNode, strTag, std::to_string(value));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a floating point value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetFloat(TiXmlNode* pRootNode, const std::string& strTag, float value)
{
  SetString(pRootNode, strTag, std::to_string(value));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a double floating point value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Value to store
///
inline void SetDouble(TiXmlNode* pRootNode, const std::string& strTag, double value)
{
  SetString(pRootNode, strTag, std::to_string(value));
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a boolean value inside XML.
///
/// By use of "true" or "false" as string in XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Boolean value to store
///
inline void SetBoolean(TiXmlNode* pRootNode, const std::string& strTag, bool value)
{
  SetString(pRootNode, strTag, value ? "true" : "false");
}
//------------------------------------------------------------------------------

//==============================================================================
/// @brief To store a path value inside XML.
///
/// @param[in] pRootNode TinyXML related note field
/// @param[in] strTag XML identification tag
/// @param[in] value Path to store
///
inline void SetPath(TiXmlNode* pRootNode, const std::string& strTag, const std::string& value)
{
  const int path_version = 1;
  TiXmlElement newElement(strTag);
  newElement.SetAttribute("pathversion", path_version);
  TiXmlNode* pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText xmlValue(value);
    pNewNode->InsertEndChild(xmlValue);
  }
}
//------------------------------------------------------------------------------

} /* namespace xml */
} /* namespace utilities */
} /* namespace enigma2 */
