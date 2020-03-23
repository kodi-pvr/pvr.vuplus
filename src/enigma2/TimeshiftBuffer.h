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

#include "IStreamReader.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace enigma2
{
  class TimeshiftBuffer : public IStreamReader
  {
  public:
    TimeshiftBuffer(IStreamReader* strReader, const std::string& m_timeshiftBufferPath, const unsigned int m_readTimeoutX);
    ~TimeshiftBuffer();

    bool Start() override;
    ssize_t ReadData(unsigned char* buffer, unsigned int size) override;
    int64_t Seek(long long position, int whence) override;
    int64_t Position() override;
    int64_t Length() override;
    std::time_t TimeStart() override;
    std::time_t TimeEnd() override;
    bool IsRealTime() override;
    bool IsTimeshifting() override;

  private:
    void DoReadWrite();

    static const int BUFFER_SIZE = 32 * 1024;
    static const int DEFAULT_READ_TIMEOUT = 10;
    static const int READ_WAITTIME = 50;

    std::string m_bufferPath;
    IStreamReader* m_streamReader;
    void* m_filebufferReadHandle;
    void* m_filebufferWriteHandle;
    int m_readTimeout;
    std::time_t m_start = 0;
    std::atomic<uint64_t> m_writePos = {0};

    std::atomic<bool> m_running = {false};
    std::thread m_inputThread;
    std::condition_variable m_condition;
    std::mutex m_mutex;
  };
} // namespace enigma2