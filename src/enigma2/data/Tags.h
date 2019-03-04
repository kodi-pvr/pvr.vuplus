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

    class Tags
    {
    public:
      const std::string& GetTags() const { return m_tags; }
      void SetTags(const std::string& value ) { m_tags = value; }

      bool ContainsTag(const std::string &tag) const
      {
        std::regex regex("^.* ?" + tag + " ?.*$");

        return (regex_match(m_tags, regex));
      };

      std::string ReadTag(const std::string &tagName) const
      {
        std::string tag;

        size_t found = m_tags.find(tagName);
        if (found != std::string::npos)
        {
          tag = m_tags.substr(found, m_tags.size());

          found = tag.find(" ");
          if (found != std::string::npos)
            tag = tag.substr(0, found);

          tag = StringUtils::Trim(tag);
        }

        return tag;
      };

    protected:
      std::string m_tags;
    };
  } //namespace data
} //namespace enigma2