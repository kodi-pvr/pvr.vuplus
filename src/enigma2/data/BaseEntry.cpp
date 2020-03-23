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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "BaseEntry.h"

#include <cinttypes>

#include <p8-platform/util/StringUtils.h>

using namespace enigma2;
using namespace enigma2::data;

void BaseEntry::ProcessPrependMode(PrependOutline prependOutlineMode)
{
  // Some providers only use PlotOutline (e.g. freesat) and Kodi does not display it, if this is the case swap them
  if (m_plot.empty())
  {
    m_plot = m_plotOutline;
    m_plotOutline.clear();
  }
  else if ((Settings::GetInstance().GetPrependOutline() == prependOutlineMode ||
            Settings::GetInstance().GetPrependOutline() == PrependOutline::ALWAYS) &&
           !m_plotOutline.empty() && m_plotOutline != "N/A")
  {
    m_plot.insert(0, m_plotOutline + "\n");
    m_plotOutline.clear();
  }
}
