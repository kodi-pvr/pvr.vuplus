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

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <kodi/libXBMC_pvr.h>

namespace enigma2
{
  static const int FAST_RECONNECT_ATTEMPTS = 5;
  static const int SLEEP_INTERVAL_STEP_MS = 500;

  class IConnectionListener;

  class ConnectionManager
  {
  public:
    ConnectionManager(IConnectionListener& connectionListener);
    ~ConnectionManager();

    void Start();
    void Stop();
    void Disconnect();
    void Reconnect();

    void OnSleep();
    void OnWake();

  private:
    void Process();
    void SetState(PVR_CONNECTION_STATE state);
    void SteppedSleep(int intervalMs);

    IConnectionListener& m_connectionListener;
    std::atomic<bool> m_running = {false};
    std::thread m_thread;
    mutable std::mutex m_mutex;
    bool m_suspended;
    PVR_CONNECTION_STATE m_state;
  };
} // namespace enigma2
