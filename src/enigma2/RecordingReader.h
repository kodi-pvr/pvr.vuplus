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

#pragma once

#include "kodi/libXBMC_addon.h"

#include <ctime>
#include <string>

namespace enigma2
{
  class RecordingReader
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
    void* m_readHandle;

    int m_duration;

    /*!< @brief start and end time of the recording set only in case this an ongoing recording */
    std::time_t m_start;
    std::time_t m_end;

    std::time_t m_nextReopen;
    uint64_t m_pos = {0};
    uint64_t m_len;
  };
} // namespace enigma2