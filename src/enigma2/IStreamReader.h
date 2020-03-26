/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <ctime>

#include <kodi/libXBMC_addon.h>

namespace enigma2
{
  class IStreamReader
  {
  public:
    virtual ~IStreamReader() = default;
    virtual bool Start() = 0;
    virtual ssize_t ReadData(unsigned char* buffer, unsigned int size) = 0;
    virtual int64_t Seek(long long position, int whence) = 0;
    virtual int64_t Position() = 0;
    virtual int64_t Length() = 0;
    virtual std::time_t TimeStart() = 0;
    virtual std::time_t TimeEnd() = 0;
    virtual bool IsRealTime() = 0;
    virtual bool IsTimeshifting() = 0;
  };
} // namespace enigma2