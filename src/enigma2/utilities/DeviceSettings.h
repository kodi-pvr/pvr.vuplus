#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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

#include <regex>

namespace enigma2
{
  namespace utilities
  {
    class DeviceSettings
    {
    public:
      DeviceSettings() = default;

      bool IsAddTagAutoTimerToTagsEnabled() const { return m_addTagAutoTimerToTagsEnabled; }
      void SetAddTagAutoTimerToTagsEnabled(bool value) { m_addTagAutoTimerToTagsEnabled = value; }

      bool IsAddAutoTimerNameToTagsEnabled() const { return m_addAutoTimerNameToTagsEnabled; }
      void SetAddAutoTimerNameToTagsEnabled(bool value) { m_addAutoTimerNameToTagsEnabled = value; }

      int GetGlobalRecordingStartMargin() const { return m_globalRecrordingStartMargin; }
      void SetGlobalRecordingStartMargin(int value) { m_globalRecrordingStartMargin = value; }

      int GetGlobalRecordingEndMargin() const { return m_globalRecrordingEndMargin; }
      void SetGlobalRecordingEndMargin(int value) { m_globalRecrordingEndMargin = value; }

    private:
      bool m_addTagAutoTimerToTagsEnabled = false;
      bool m_addAutoTimerNameToTagsEnabled = false;
      int m_globalRecrordingStartMargin = 0;
      int m_globalRecrordingEndMargin = 0;
    };
  } //namespace utilities
} //namespace enigma2