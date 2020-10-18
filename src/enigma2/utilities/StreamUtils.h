/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <map>
#include <string>

#include <kodi/addon-instance/pvr/General.h>

namespace enigma2
{
  namespace utilities
  {
    static const std::string INPUTSTREAM_FFMPEGDIRECT = "inputstream.ffmpegdirect";

    enum class StreamType
      : int // same type as addon settings
    {
      HLS = 0,
      DASH,
      SMOOTH_STREAMING,
      TS,
      OTHER_TYPE
    };

    class StreamUtils
    {
    public:
      static const StreamType GetStreamType(const std::string& url);
      static const StreamType InspectStreamType(const std::string& url);
      static std::string GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType);
      static bool CheckInputstreamInstalledAndEnabled(const std::string& inputstreamName);
      static void SetFFmpegDirectManifestTypeStreamProperty(std::vector<kodi::addon::PVRStreamProperty>& properties, const std::string& streamURL, const StreamType& streamType);

    private:
      static const std::string GetManifestType(const StreamType& streamType);
      static const std::string GetMimeType(const StreamType& streamType);
      static bool HasMimeType(const StreamType& streamType);
      static std::string AddHeader(const std::string& headerTarget, const std::string& headerName, const std::string& headerValue, bool encodeHeaderValue);
      static std::string AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue);
      static std::string GetUrlEncodedProtocolOptions(const std::string& protocolOptions);
    };
  } // namespace utilities
} // namespace enigma2
