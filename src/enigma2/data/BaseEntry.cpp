/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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
