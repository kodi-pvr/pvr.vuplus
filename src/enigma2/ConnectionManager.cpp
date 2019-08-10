/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ConnectionManager.h"

#include "../client.h"
#include "IConnectionListener.h"
#include "Settings.h"
#include "p8-platform/os.h"
#include "p8-platform/util/StringUtils.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

using namespace P8PLATFORM;
using namespace enigma2;
using namespace enigma2::utilities;

/*
 * Enigma2 Connection handler
 */

ConnectionManager::ConnectionManager(IConnectionListener& connectionListener)
  : m_connectionListener(connectionListener), m_suspended(false), m_state(PVR_CONNECTION_STATE_UNKNOWN)
{
}

ConnectionManager::~ConnectionManager()
{
  StopThread(-1);
  Disconnect();
  StopThread(0);
}

void ConnectionManager::Start()
{
  // Note: "connecting" must only be set one time, before the very first connection attempt, not on every reconnect.
  SetState(PVR_CONNECTION_STATE_CONNECTING);
  CreateThread();
}

void ConnectionManager::Stop()
{
  StopThread(-1);
  Disconnect();
}

void ConnectionManager::OnSleep()
{
  CLockObject lock(m_mutex);

  Logger::Log(LogLevel::LEVEL_DEBUG, "%s going to sleep", __FUNCTION__);

  m_suspended = true;
}

void ConnectionManager::OnWake()
{
  CLockObject lock(m_mutex);

  Logger::Log(LogLevel::LEVEL_DEBUG, "%s Waking up", __FUNCTION__);

  m_suspended = false;
}

void ConnectionManager::SetState(PVR_CONNECTION_STATE state)
{
  PVR_CONNECTION_STATE prevState(PVR_CONNECTION_STATE_UNKNOWN);
  PVR_CONNECTION_STATE newState(PVR_CONNECTION_STATE_UNKNOWN);

  {
    CLockObject lock(m_mutex);

    /* No notification if no state change or while suspended. */
    if (m_state != state && !m_suspended)
    {
      prevState = m_state;
      newState = state;
      m_state = newState;

      Logger::Log(LogLevel::LEVEL_DEBUG, "connection state change (%d -> %d)", prevState, newState);
    }
  }

  if (prevState != newState)
  {
    static std::string serverString;

    if (newState == PVR_CONNECTION_STATE_SERVER_UNREACHABLE)
    {
      m_connectionListener.ConnectionLost();
    }
    else if (newState == PVR_CONNECTION_STATE_CONNECTED)
    {
      m_connectionListener.ConnectionEstablished();
    }

    /* Notify connection state change (callback!) */
    PVR->ConnectionStateChange(Settings::GetInstance().GetConnectionURL().c_str(), newState, NULL);
  }
}

void ConnectionManager::Disconnect()
{
  CLockObject lock(m_mutex);

  m_connectionListener.ConnectionLost();
}

void ConnectionManager::Reconnect()
{
  // Setting this state will cause Enigma2 to receive a connetionLost event
  // The connection manager will then connect again causeing a reload of all state
  SetState(PVR_CONNECTION_STATE_SERVER_UNREACHABLE);
}

void* ConnectionManager::Process()
{
  static bool log = false;
  static unsigned int retryAttempt = 0;
  int fastReconnectIntervalMs = (Settings::GetInstance().GetConnectioncCheckIntervalSecs() * 1000) / 2;
  int intervalMs = Settings::GetInstance().GetConnectioncCheckIntervalSecs() * 1000;

  while (!IsStopped())
  {
    while (m_suspended)
    {
      Logger::Log(LogLevel::LEVEL_DEBUG, "%s - suspended, waiting for wakeup...", __FUNCTION__);

      /* Wait for wakeup */
      SteppedSleep(intervalMs);
    }

    const std::string url = StringUtils::Format("%s%s", Settings::GetInstance().GetConnectionURL().c_str(), "web/currenttime");

    /* Connect */
    if (!WebUtils::CheckHttp(url))
    {
      /* Unable to connect */
      if (retryAttempt == 0)
        Logger::Log(LogLevel::LEVEL_ERROR, "%s - unable to connect to: %s", __FUNCTION__, url.c_str());
      SetState(PVR_CONNECTION_STATE_SERVER_UNREACHABLE);

      // Retry a few times with a short interval, after that with the default timeout
      if (++retryAttempt <= FAST_RECONNECT_ATTEMPTS)
        SteppedSleep(fastReconnectIntervalMs);
      else
        SteppedSleep(intervalMs);

      continue;
    }

    SetState(PVR_CONNECTION_STATE_CONNECTED);
    retryAttempt = 0;

    SteppedSleep(intervalMs);
  }

  return nullptr;
}

void ConnectionManager::SteppedSleep(int intervalMs)
{
  int sleepCountMs = 0;

  while (sleepCountMs <= intervalMs)
  {
    if (!IsStopped())
      Sleep(SLEEP_INTERVAL_STEP_MS);

    sleepCountMs += SLEEP_INTERVAL_STEP_MS;
  }
}