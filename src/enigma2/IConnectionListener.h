/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/addon-instance/PVR.h>

namespace enigma2
{
  class ATTRIBUTE_HIDDEN IConnectionListener : public kodi::addon::CInstancePVRClient
  {
  public:
    IConnectionListener(KODI_HANDLE instance, const std::string& version)
      : kodi::addon::CInstancePVRClient(instance, version) { }
    virtual ~IConnectionListener() = default;

    virtual void ConnectionLost() = 0;
    virtual void ConnectionEstablished() = 0;
  };
} // namespace enigma2
