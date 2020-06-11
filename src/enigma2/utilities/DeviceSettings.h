/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

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
