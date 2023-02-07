/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IStreamReader.h"
#include "Settings.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <kodi/Filesystem.h>

namespace enigma2
{
  class ATTR_DLL_LOCAL TimeshiftBuffer : public IStreamReader
  {
  public:
    TimeshiftBuffer(IStreamReader* strReader, std::shared_ptr<enigma2::Settings>& settings);
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
    bool HasTimeshiftCapacity() override;

  private:
    void DoReadWrite();

    static const int BUFFER_SIZE = 32 * 1024;
    static const int DEFAULT_READ_TIMEOUT = 10;
    static const int READ_WAITTIME = 50;

    std::string m_bufferPath;
    IStreamReader* m_streamReader;
    kodi::vfs::CFile m_filebufferReadHandle;
    kodi::vfs::CFile m_filebufferWriteHandle;
    int m_readTimeout;
    std::time_t m_start = 0;
    std::atomic<uint64_t> m_writePos = {0};
    uint64_t m_timeshiftBufferByteLimit = 0LL;

    std::atomic<bool> m_running = {false};
    std::thread m_inputThread;
    std::condition_variable m_condition;
    std::mutex m_mutex;
  };
} // namespace enigma2
