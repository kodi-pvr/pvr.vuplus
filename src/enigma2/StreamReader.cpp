/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamReader.h"

#include "utilities/Logger.h"

using namespace enigma2;
using namespace enigma2::utilities;

StreamReader::StreamReader(const std::string& streamURL, const unsigned int readTimeout)
{
  m_streamHandle.CURLCreate(streamURL);
  if (readTimeout > 0)
    m_streamHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "connection-timeout", std::to_string(readTimeout).c_str());

  Logger::Log(LEVEL_DEBUG, "%s StreamReader: Started; url=%s", __func__, streamURL.c_str());
}

StreamReader::~StreamReader()
{
  Logger::Log(LEVEL_DEBUG, "%s StreamReader: Stopped", __func__);
}

bool StreamReader::Start()
{
  return m_streamHandle.CURLOpen(ADDON_READ_NO_CACHE);
}

ssize_t StreamReader::ReadData(unsigned char* buffer, unsigned int size)
{
  return m_streamHandle.Read(buffer, size);
}

int64_t StreamReader::Seek(long long position, int whence)
{
  return m_streamHandle.Seek(position, whence);
}

int64_t StreamReader::Position()
{
  return m_streamHandle.GetPosition();
}

int64_t StreamReader::Length()
{
  return m_streamHandle.GetLength();
}

std::time_t StreamReader::TimeStart()
{
  return m_start;
}

std::time_t StreamReader::TimeEnd()
{
  return std::time(nullptr);
}

bool StreamReader::IsRealTime()
{
  return true;
}

bool StreamReader::IsTimeshifting()
{
  return false;
}

bool StreamReader::HasTimeshiftCapacity()
{
  return false;
}
