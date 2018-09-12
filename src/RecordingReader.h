#pragma once

#include "libXBMC_addon.h"

#include <ctime>

class RecordingReader
{
public:
  RecordingReader(const std::string &streamURL, std::time_t end);
  ~RecordingReader(void);
  bool Start();
  ssize_t ReadData(unsigned char *buffer, unsigned int size);
  int64_t Seek(long long position, int whence);
  int64_t Position();
  int64_t Length();

private:
  std::string m_streamURL;
  void *m_readHandle;

  /*!< @brief end time of the recording in case this an ongoing recording */
  std::time_t m_end;
  std::time_t m_nextReopen;
  uint64_t m_pos = { 0 };
  uint64_t m_len;
};
