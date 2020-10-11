/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamUtils.h"

#include "../Settings.h"
#include "FileUtils.h"
#include "Logger.h"
#include "WebUtils.h"

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::utilities;

bool StreamUtils::CheckInputstreamInstalledAndEnabled(const std::string& inputstreamName)
{
  std::string version;
  bool enabled;

  if (kodi::IsAddonAvailable(inputstreamName, version, enabled))
  {
    if (!enabled)
    {
      std::string message = StringUtils::Format(kodi::GetLocalizedString(30502).c_str(), inputstreamName.c_str());
      kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::GetLocalizedString(30500), message);
    }
  }
  else // Not installed
  {
    std::string message = StringUtils::Format(kodi::GetLocalizedString(30501).c_str(), inputstreamName.c_str());
    kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::GetLocalizedString(30500), message);
  }

  return true;
}

void StreamUtils::SetFFmpegDirectManifestTypeStreamProperty(std::vector<kodi::addon::PVRStreamProperty>& properties, const std::string& streamURL, const StreamType& streamType)
{
  std::string manifestType = StreamUtils::GetManifestType(streamType);
  if (!manifestType.empty())
    properties.emplace_back("inputstream.ffmpegdirect.manifest_type", manifestType);
}

const StreamType StreamUtils::GetStreamType(const std::string& url)
{
  if (url.find(".m3u8") != std::string::npos)
    return StreamType::HLS;

  if (url.find(".mpd") != std::string::npos)
    return StreamType::DASH;

  if (url.find(".ism") != std::string::npos &&
      !(url.find(".ismv") != std::string::npos || url.find(".isma") != std::string::npos))
    return StreamType::SMOOTH_STREAMING;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url)
{
  if (!FileUtils::FileExists(url))
    return StreamType::OTHER_TYPE;

  int httpCode = 0;
  const std::string source = WebUtils::ReadFileContentsStartOnly(url, &httpCode);

  if (httpCode == 200)
  {
    if (StringUtils::StartsWith(source, "#EXTM3U") && (source.find("#EXT-X-STREAM-INF") != std::string::npos || source.find("#EXT-X-VERSION") != std::string::npos))
      return StreamType::HLS;

    if (source.find("<MPD") != std::string::npos)
      return StreamType::DASH;

    if (source.find("<SmoothStreamingMedia") != std::string::npos)
      return StreamType::SMOOTH_STREAMING;
  }

  // There is no other way to select this stream type other than this setting
  if (Settings::GetInstance().UseMpegtsForUnknownStreams())
    return StreamType::TS;

  return StreamType::OTHER_TYPE;
}

const std::string StreamUtils::GetManifestType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "hls";
    case StreamType::DASH:
      return "mpd";
    case StreamType::SMOOTH_STREAMING:
      return "ism";
    default:
      return "";
  }
}

const std::string StreamUtils::GetMimeType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "application/x-mpegURL";
    case StreamType::DASH:
      return "application/xml+dash";
    case StreamType::TS:
      return "video/mp2t";
    default:
      return "";
  }
}

bool StreamUtils::HasMimeType(const StreamType& streamType)
{
  return streamType != StreamType::OTHER_TYPE && streamType != StreamType::SMOOTH_STREAMING;
}

std::string StreamUtils::GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType)
{
  std::string newStreamUrl = streamUrl;

  if (WebUtils::IsHttpUrl(streamUrl) && Settings::GetInstance().UseFFmpegReconnect())
  {
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect", "1");
    if (streamType != StreamType::HLS)
      newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_at_eof", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_streamed", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_delay_max", "4294");

    Logger::Log(LogLevel::LEVEL_DEBUG, "%s - FFmpeg Reconnect Stream URL: %s", __FUNCTION__, newStreamUrl.c_str());
  }

  return newStreamUrl;
}

std::string StreamUtils::AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue)
{
  return StreamUtils::AddHeader(streamUrl, headerName, headerValue, false);
}

std::string StreamUtils::AddHeader(const std::string& headerTarget, const std::string& headerName, const std::string& headerValue, bool encodeHeaderValue)
{
  std::string newHeaderTarget = headerTarget;

  bool hasProtocolOptions = false;
  bool addHeader = true;
  size_t found = newHeaderTarget.find("|");

  if (found != std::string::npos)
  {
    hasProtocolOptions = true;
    addHeader = newHeaderTarget.find(headerName + "=", found + 1) == std::string::npos;
  }

  if (addHeader)
  {
    if (!hasProtocolOptions)
      newHeaderTarget += "|";
    else
      newHeaderTarget += "&";

    newHeaderTarget += headerName + "=" + (encodeHeaderValue ? WebUtils::UrlEncode(headerValue) : headerValue);
  }

  return newHeaderTarget;
}

std::string StreamUtils::GetUrlEncodedProtocolOptions(const std::string& protocolOptions)
{
  std::string encodedProtocolOptions = "";

  std::vector<std::string> headers = StringUtils::Split(protocolOptions, "&");
  for (std::string header : headers)
  {
    std::string::size_type pos(header.find('='));
    if(pos == std::string::npos)
      continue;

    encodedProtocolOptions = StreamUtils::AddHeader(encodedProtocolOptions, header.substr(0, pos), header.substr(pos + 1), true);
  }

  // We'll return the protocol options without the leading '|'
  if (!encodedProtocolOptions.empty() && encodedProtocolOptions[0] == '|')
    encodedProtocolOptions.erase(0, 1);

  return encodedProtocolOptions;
}
