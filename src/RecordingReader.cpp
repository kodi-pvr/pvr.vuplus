#include "RecordingReader.h"

#include <algorithm>

#include "client.h"

#include "p8-platform/threads/mutex.h"

using namespace ADDON;

RecordingReader::RecordingReader(const std::string &streamURL, std::time_t end)
  : m_streamURL(streamURL), m_end(end)
{
  m_readHandle = XBMC->CURLCreate(m_streamURL.c_str());
  (void)XBMC->CURLOpen(m_readHandle, XFILE::READ_NO_CACHE);
  m_len = XBMC->GetFileLength(m_readHandle);
  m_nextReopen = time(nullptr) + REOPEN_INTERVAL;
  XBMC->Log(LOG_DEBUG, "RecordingReader: Started; url=%s, end=%u",
      m_streamURL.c_str(), m_end);
}

RecordingReader::~RecordingReader(void)
{
  if (m_readHandle)
    XBMC->CloseFile(m_readHandle);
  XBMC->Log(LOG_DEBUG, "RecordingReader: Stopped");
}

bool RecordingReader::Start()
{
  return (m_readHandle != nullptr);
}

ssize_t RecordingReader::ReadData(unsigned char *buffer, unsigned int size)
{
  /* check for playback of ongoing recording */
  if (m_end)
  {
    std::time_t now = std::time(nullptr);
    if (m_pos == m_len || now > m_nextReopen)
    {
      /* reopen stream */
      XBMC->Log(LOG_DEBUG, "RecordingReader: Reopening stream...");
      (void)XBMC->CURLOpen(m_readHandle, XFILE::READ_REOPEN | XFILE::READ_NO_CACHE);
      m_len = XBMC->GetFileLength(m_readHandle);
      XBMC->SeekFile(m_readHandle, m_pos, SEEK_SET);

      // random value (10 MiB) we choose to switch to fast reopen interval
      bool nearEnd = m_len - m_pos <= 10 * 1024 * 1024;
      m_nextReopen = now + (nearEnd ? REOPEN_INTERVAL_FAST : REOPEN_INTERVAL);

      /* recording has finished */
      if (now > m_end)
        m_end = 0;
    }
  }

  ssize_t read = XBMC->ReadFile(m_readHandle, buffer, size);
  m_pos += read;
  return read;
}

int64_t RecordingReader::Seek(long long position, int whence)
{
  int64_t ret = XBMC->SeekFile(m_readHandle, position, whence);
  // for unknown reason seek sometimes doesn't return the correct position
  // so let's sync with the underlaying implementation
  m_pos = XBMC->GetFilePosition(m_readHandle);
  m_len = XBMC->GetFileLength(m_readHandle);
  return ret;
}

int64_t RecordingReader::Position()
{
  return m_pos;
}

int64_t RecordingReader::Length()
{
  return m_len;
}
