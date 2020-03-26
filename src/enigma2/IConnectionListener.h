/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

namespace enigma2
{
  class IConnectionListener
  {
  public:
    virtual ~IConnectionListener() = default;

    virtual void ConnectionLost() = 0;
    virtual void ConnectionEstablished() = 0;
  };
} // namespace enigma2
