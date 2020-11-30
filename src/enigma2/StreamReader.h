/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IStreamReader.h"

#include <string>

#include <kodi/Filesystem.h>

namespace enigma2
{
  class ATTRIBUTE_HIDDEN StreamReader : public IStreamReader
  {
  public:
    StreamReader(const std::string& streamURL, const unsigned int m_readTimeout);
    ~StreamReader();

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
    kodi::vfs::CFile m_streamHandle;
    std::time_t m_start = std::time(nullptr);
  };
} // namespace enigma2
