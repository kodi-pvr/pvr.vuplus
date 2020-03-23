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