/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StreamReader.h"

#include "../client.h"
#include "utilities/Logger.h"

using namespace ADDON;
using namespace enigma2;
using namespace enigma2::utilities;

StreamReader::StreamReader(const std::string& streamURL, const unsigned int readTimeout)
{
  m_streamHandle = XBMC->CURLCreate(streamURL.c_str());
  if (readTimeout > 0)
    XBMC->CURLAddOption(m_streamHandle, XFILE::CURL_OPTION_PROTOCOL, "connection-timeout", std::to_string(readTimeout).c_str());

  Logger::Log(LEVEL_DEBUG, "%s StreamReader: Started; url=%s", __FUNCTION__, streamURL.c_str());
}

StreamReader::~StreamReader()
{
  if (m_streamHandle)
    XBMC->CloseFile(m_streamHandle);
  Logger::Log(LEVEL_DEBUG, "%s StreamReader: Stopped", __FUNCTION__);
}

bool StreamReader::Start()
{
  return XBMC->CURLOpen(m_streamHandle, XFILE::READ_NO_CACHE);
}

ssize_t StreamReader::ReadData(unsigned char* buffer, unsigned int size)
{
  return XBMC->ReadFile(m_streamHandle, buffer, size);
}

int64_t StreamReader::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_streamHandle, position, whence);
}

int64_t StreamReader::Position()
{
  return XBMC->GetFilePosition(m_streamHandle);
}

int64_t StreamReader::Length()
{
  return XBMC->GetFileLength(m_streamHandle);
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
