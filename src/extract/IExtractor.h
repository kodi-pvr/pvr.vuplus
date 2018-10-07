#pragma once

#include "../Settings.h"
#include "../VuBase.h"

#include <regex>

namespace VUPLUS
{

class IExtractor
{
public:
  IExtractor(const VUPLUS::Settings &settings) : m_settings(settings) {};
  virtual ~IExtractor(void) = default;
  virtual void ExtractFromEntry(VuBase &base) = 0;

protected:
  static std::string GetMatchTextFromString(const std::string &text, const std::regex &pattern)
  {
    std::string matchText = "";
    std::smatch match;

    if (regex_match(text, match, pattern))
    {
      if (match.size() == 2)
      {
        std::ssub_match base_sub_match = match[1];
        matchText = base_sub_match.str();  
      }
    }

    return matchText;
  };

  static std::string GetMatchedText(const std::string &firstText, const std::string &secondText, const std::regex &pattern)
  {
    std::string matchedText = GetMatchTextFromString(firstText, pattern);

    if (matchedText.empty())
      matchedText = GetMatchTextFromString(secondText, pattern);

    return matchedText;
  }

  VUPLUS::Settings m_settings;
};

} //namespace VUPLUS