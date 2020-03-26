/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <regex>
#include <string>

#include <p8-platform/util/StringUtils.h>

namespace enigma2
{
  namespace data
  {
    static const std::string TAG_FOR_GENRE_ID = "GenreId";
    static const std::string TAG_FOR_CHANNEL_REFERENCE = "ChannelRef";
    static const std::string TAG_FOR_CHANNEL_TYPE = "ChannelType";
    static const std::string TAG_FOR_ANY_CHANNEL = "AnyChannel";
    static const std::string VALUE_FOR_CHANNEL_TYPE_TV = "TV";
    static const std::string VALUE_FOR_CHANNEL_TYPE_RADIO = "Radio";

    class Tags
    {
    public:
      Tags() = default;
      Tags(const std::string& tags) : m_tags(tags) {};

      const std::string& GetTags() const { return m_tags; }
      void SetTags(const std::string& value) { m_tags = value; }

      bool ContainsTag(const std::string& tag) const
      {
        const std::regex regex("^.* ?" + tag + " ?.*$");

        return (std::regex_match(m_tags, regex));
      }

      void AddTag(const std::string& tagName, const std::string& tagValue = "", bool replaceUnderscores = false)
      {
        RemoveTag(tagName);

        if (!m_tags.empty())
          m_tags.append(" ");

        m_tags.append(tagName);

        if (!tagValue.empty())
        {
          std::string val = tagValue;
          if (replaceUnderscores)
            std::replace(val.begin(), val.end(), ' ', '_');
          m_tags.append(StringUtils::Format("=%s", val.c_str()));
        }
      }

      std::string ReadTagValue(const std::string& tagName, bool replaceUnderscores = false) const
      {
        std::string tagValue;

        size_t found = m_tags.find(tagName + "=");
        if (found != std::string::npos)
        {
          tagValue = m_tags.substr(found + tagName.size() + 1, m_tags.size());

          found = tagValue.find(" ");
          if (found != std::string::npos)
            tagValue = tagValue.substr(0, found);

          tagValue = StringUtils::Trim(tagValue);

          if (replaceUnderscores)
            std::replace(tagValue.begin(), tagValue.end(), '_', ' ');
        }

        return tagValue;
      }

      void RemoveTag(const std::string& tagName)
      {
        const std::regex regex(" *" + tagName + "=?[^\\s-]*");
        std::string replaceWith = "";

        m_tags = std::regex_replace(m_tags, regex, replaceWith);
      }

    protected:
      std::string m_tags;
    };
  } //namespace data
} //namespace enigma2