/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <ctime>
#include <string>

#include <kodi/Filesystem.h>

namespace enigma2
{
  class ATTR_DLL_LOCAL RecordingReader
  {
  public:
    RecordingReader(const std::string& streamURL, std::time_t start, std::time_t end, int duration);
    ~RecordingReader();

    bool Start();
    ssize_t ReadData(unsigned char* buffer, unsigned int size);
    int64_t Seek(long long position, int whence);
    int64_t Position();
    int64_t Length();
    int CurrentDuration();


  private:
    static const int REOPEN_INTERVAL = 30;
    static const int REOPEN_INTERVAL_FAST = 10;

    const std::string& m_streamURL;
    kodi::vfs::CFile m_readHandle;

    int m_duration;

    /*!< @brief start and end time of the recording set only in case this an ongoing recording */
    std::time_t m_start;
    std::time_t m_end;

    std::time_t m_nextReopen;
    uint64_t m_pos = {0};
    uint64_t m_len;
  };
} // namespace enigma2
