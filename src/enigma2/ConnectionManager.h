#pragma once

/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>

#include "libXBMC_pvr.h"

#include "p8-platform/threads/mutex.h"
#include "p8-platform/threads/threads.h"

namespace enigma2
{
  static const int FAST_RECONNECT_ATTEMPTS = 5;
  static const int SLEEP_INTERVAL_STEP_MS = 500;

  class IConnectionListener;

  class ConnectionManager : public P8PLATFORM::CThread
  {
  public:
    ConnectionManager(IConnectionListener& connectionListener);
    ~ConnectionManager() override;

    void Start();
    void Stop();
    void Disconnect();
    void Reconnect();

    void OnSleep();
    void OnWake();

  private:
    void* Process() override;
    void SetState(PVR_CONNECTION_STATE state);
    void SteppedSleep(int intervalMs);

    IConnectionListener& m_connectionListener;
    mutable P8PLATFORM::CMutex m_mutex;
    bool m_suspended;
    PVR_CONNECTION_STATE m_state;
  };
} // namespace enigma2
