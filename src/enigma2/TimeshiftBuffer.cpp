/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeshiftBuffer.h"

#include "InstanceSettings.h"
#include "StreamReader.h"
#include "utilities/Logger.h"

using namespace enigma2;
using namespace enigma2::utilities;

TimeshiftBuffer::TimeshiftBuffer(IStreamReader* streamReader, std::shared_ptr<InstanceSettings>& settings) : m_streamReader(streamReader)
{
  m_bufferPath = settings->GetTimeshiftBufferPath() + "/tsbuffer.ts";
  unsigned int readTimeout = settings->GetReadTimeoutSecs();
  m_readTimeout = (readTimeout) ? readTimeout : DEFAULT_READ_TIMEOUT;
  if (settings->EnableTimeshiftDiskLimit())
    m_timeshiftBufferByteLimit = settings->GetTimeshiftDiskLimitBytes();

  m_filebufferWriteHandle.OpenFileForWrite(m_bufferPath, true);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_filebufferReadHandle.OpenFile(m_bufferPath, ADDON_READ_NO_CACHE);
}

TimeshiftBuffer::~TimeshiftBuffer()
{
  m_running = false;
  if (m_inputThread.joinable())
    m_inputThread.join();

  if (m_filebufferWriteHandle.IsOpen())
  {
    // XBMC->TruncateFile doesn't work for unknown reasons
    m_filebufferWriteHandle.Close();
    kodi::vfs::CFile tmp;
    if (tmp.OpenFileForWrite(m_bufferPath, true))
      tmp.Close();
  }
  if (m_filebufferReadHandle.IsOpen())
    m_filebufferReadHandle.Close();

  if (!kodi::vfs::DeleteFile(m_bufferPath))
    Logger::Log(LEVEL_ERROR, "%s Unable to delete file when timeshift buffer is deleted: %s", __func__, m_bufferPath.c_str());

  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Stopped", __func__);
}

bool TimeshiftBuffer::Start()
{
  if (m_streamReader == nullptr || !m_filebufferWriteHandle.IsOpen() || !m_filebufferReadHandle.IsOpen())
    return false;
  if (m_running)
    return true;

  Logger::Log(LEVEL_INFO, "%s Timeshift: Started", __func__);
  m_start = std::time(nullptr);
  m_running = true;
  m_inputThread = std::thread([&] { DoReadWrite(); });

  return true;
}

void TimeshiftBuffer::DoReadWrite()
{
  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Thread started", __func__);
  uint8_t buffer[BUFFER_SIZE];

  m_streamReader->Start();
  while (m_running)
  {
    ssize_t read = m_streamReader->ReadData(buffer, sizeof(buffer));

    // don't handle any errors here, assume write fully succeeds
    ssize_t write = m_filebufferWriteHandle.Write(buffer, read);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_writePos += write;

    m_condition.notify_one();
  }
  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Thread stopped", __func__);
  return;
}

int64_t TimeshiftBuffer::Seek(long long position, int whence)
{
  return m_filebufferReadHandle.Seek(position, whence);
}

int64_t TimeshiftBuffer::Position()
{
  return m_filebufferReadHandle.GetPosition();
}

int64_t TimeshiftBuffer::Length()
{
  return m_writePos;
}

ssize_t TimeshiftBuffer::ReadData(unsigned char* buffer, unsigned int size)
{
  int64_t requiredLength = Position() + size;

  /* make sure we never read above the current write position */
  std::unique_lock<std::mutex> lock(m_mutex);
  bool available = m_condition.wait_for(lock, std::chrono::seconds(m_readTimeout), [&] { return Length() >= requiredLength; });

  if (!available)
  {
    Logger::Log(LEVEL_DEBUG, "%s Timeshift: Read timed out; waited %d", __func__, m_readTimeout);
    return -1;
  }

  return m_filebufferReadHandle.Read(buffer, size);
}

std::time_t TimeshiftBuffer::TimeStart()
{
  return m_start;
}

std::time_t TimeshiftBuffer::TimeEnd()
{
  return std::time(nullptr);
}

bool TimeshiftBuffer::IsRealTime()
{
  // other PVRs use 10 seconds here, but we aren't doing any demuxing
  // we'll therefore just asume 1 secs needs about 1mb
  return Length() - Position() <= 10 * 1048576;
}

bool TimeshiftBuffer::IsTimeshifting()
{
  return true;
}

bool TimeshiftBuffer::HasTimeshiftCapacity()
{
  return m_timeshiftBufferByteLimit == 0 || m_timeshiftBufferByteLimit > m_writePos;
}
