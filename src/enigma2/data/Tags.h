#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <regex>

#include "p8-platform/util/StringUtils.h"

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
      void SetTags(const std::string& value ) { m_tags = value; }

      bool ContainsTag(const std::string &tag) const
      {
        std::regex regex("^.* ?" + tag + " ?.*$");

        return (regex_match(m_tags, regex));
      }

      void AddTag(const std::string &tagName, std::string tagValue = "", bool replaceUnderscores = false)
      {
        RemoveTag(tagName);

        if (!m_tags.empty())
          m_tags.append(" ");

        m_tags.append(tagName);

        if (!tagValue.empty())
        {
          if (replaceUnderscores)
            std::replace(tagValue.begin(), tagValue.end(), ' ', '_');
          m_tags.append(StringUtils::Format("=%s", tagValue.c_str()));
        }
      }

      std::string ReadTagValue(const std::string &tagName, bool replaceUnderscores = false) const
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

      void RemoveTag(const std::string &tagName)
      {
        std::regex regex(" *" + tagName + "=?[^\\s-]*");
        std::string replaceWith = "";

        m_tags = regex_replace(m_tags, regex, replaceWith);
      }

    protected:
      std::string m_tags;
    };
  } //namespace data
} //namespace enigma2