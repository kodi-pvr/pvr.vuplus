#include "TimeshiftBuffer.h"

#include "../client.h"
#include "StreamReader.h"
#include "utilities/Logger.h"

#include "p8-platform/util/util.h"

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::utilities;

TimeshiftBuffer::TimeshiftBuffer(IStreamReader *m_streamReader,
    const std::string &m_timeshiftBufferPath, const unsigned int m_readTimeoutX)
  : m_streamReader(m_streamReader)
{
  m_bufferPath = m_timeshiftBufferPath + "/tsbuffer.ts";
  m_readTimeout = (m_readTimeoutX) ? m_readTimeout
      : DEFAULT_READ_TIMEOUT;

  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath.c_str(), true);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath.c_str(), XFILE::READ_NO_CACHE);
}

TimeshiftBuffer::~TimeshiftBuffer(void)
{
  m_running = false;
  if (m_inputThread.joinable())
    m_inputThread.join();

  if (m_filebufferWriteHandle)
  {
    // XBMC->TruncateFile doesn't work for unknown reasons
    XBMC->CloseFile(m_filebufferWriteHandle);
    void *tmp;
    if ((tmp = XBMC->OpenFileForWrite(m_bufferPath.c_str(), true)) != nullptr)
      XBMC->CloseFile(tmp);
  }
  if (m_filebufferReadHandle)
    XBMC->CloseFile(m_filebufferReadHandle);
  
  if (!XBMC->DeleteFile(m_bufferPath.c_str()))
    Logger::Log(LEVEL_ERROR, "%s Unable to delete file when timeshift buffer is deleted: %s", __FUNCTION__, m_bufferPath.c_str());
  
  SAFE_DELETE(m_streamReader);
  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Stopped", __FUNCTION__);
}

bool TimeshiftBuffer::Start()
{
  if (m_streamReader == nullptr
      || m_filebufferWriteHandle == nullptr
      || m_filebufferReadHandle == nullptr)
    return false;
  if (m_running)
    return true;

  Logger::Log(LEVEL_INFO, "%s Timeshift: Started", __FUNCTION__);
  m_start = time(nullptr);
  m_running = true;
  m_inputThread = std::thread([&] { DoReadWrite(); });

  return true;
}

void TimeshiftBuffer::DoReadWrite()
{
  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Thread started", __FUNCTION__);
  uint8_t buffer[BUFFER_SIZE];

  m_streamReader->Start();
  while (m_running)
  {
    ssize_t read = m_streamReader->ReadData(buffer, sizeof(buffer));

    // don't handle any errors here, assume write fully succeeds
    ssize_t write = XBMC->WriteFile(m_filebufferWriteHandle, buffer, read);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_writePos += write;

    m_condition.notify_one();
  }
  Logger::Log(LEVEL_DEBUG, "%s Timeshift: Thread stopped", __FUNCTION__);
  return;
}

int64_t TimeshiftBuffer::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_filebufferReadHandle, position, whence);
}

int64_t TimeshiftBuffer::Position()
{
  return XBMC->GetFilePosition(m_filebufferReadHandle);
}

int64_t TimeshiftBuffer::Length()
{
  return m_writePos;
}

ssize_t TimeshiftBuffer::ReadData(unsigned char *buffer, unsigned int size)
{
  int64_t requiredLength = Position() + size;

  /* make sure we never read above the current write position */
  std::unique_lock<std::mutex> lock(m_mutex);
  bool available = m_condition.wait_for(lock, std::chrono::seconds(m_readTimeout),
    [&] { return Length() >= requiredLength; });

  if (!available)
  {
    Logger::Log(LEVEL_DEBUG, "%s Timeshift: Read timed out; waited %d", __FUNCTION__, m_readTimeout);
    return -1;
  }

  return XBMC->ReadFile(m_filebufferReadHandle, buffer, size);
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
