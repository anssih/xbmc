/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#ifdef HAS_ALSA

#include <poll.h>
#include <sys/eventfd.h>

#include "utils/log.h"

#include "FDEventMonitor.h"

CFDEventMonitor::CFDEventMonitor() :
  CThread("FDEventMonitor"),
  m_nextID(0),
  m_wakeupfd(-1)
{
}

CFDEventMonitor::~CFDEventMonitor()
{
  if (m_wakeupfd >= 0)
  {
    /* sets m_bStop */
    StopThread(false);

    /* wake up the poll() call */
    eventfd_write(m_wakeupfd, 1);
    
    /* Wait for the thread to stop */
    StopThread(true);

    close(m_wakeupfd);
  }
}

void CFDEventMonitor::AddFD(const MonitoredFD& monitoredFD, int& id)
{
  CSingleLock lock(m_mutex);
  InterruptPoll();

  AddFDLocked(monitoredFD, id);

  StartMonitoring();
}

void CFDEventMonitor::AddFDs(const std::vector<MonitoredFD>& monitoredFDs,
                             std::vector<int>& ids)
{
  CSingleLock lock(m_mutex);
  InterruptPoll();

  for (unsigned int i = 0; i < monitoredFDs.size(); ++i)
  {
    int id;
    AddFDLocked(monitoredFDs[i], id);
    ids.push_back(id);
  }
  
  StartMonitoring();
}

void CFDEventMonitor::RemoveFD(int id)
{
  CSingleLock lock(m_mutex);
  InterruptPoll();

  if (m_monitoredFDs.erase(id) != 1)
  {
    // LOG ERROR
  }
  
  UpdatePollDescs();
}

void CFDEventMonitor::RemoveFDs(const std::vector<int>& ids)
{
  CSingleLock lock(m_mutex);
  InterruptPoll();

  for (unsigned int i = 0; i < ids.size(); ++i)
  {
    if (m_monitoredFDs.erase(ids[i]) != 1)
    {
      // LOG ERROR
    }
  }
  
  UpdatePollDescs();
}

void CFDEventMonitor::Process()
{
  eventfd_t dummy;

  while (!m_bStop)
  {
    CSingleLock lock(m_mutex);

    /* flush wakeup fd */
    eventfd_read(m_wakeupfd, &dummy);

    CSingleLock pollLock(m_pollMutex);

    /*
     * Leave the main mutex to allow another thread to
     * 1. Lock main mutex (to pause this loop).
     * 2. Signal poll handler to exit via m_wakeupfd.
     * 3. Lock poll mutex (to make sure poll handling has stopped).
     */
    lock.Leave();

    int err = poll(&m_pollDescs[0], m_pollDescs.size(), -1);
    if (err < 0)
    {
      // ERROR
    }
    // Something woke us up - either there is data available or we are shutting down

    for (unsigned int i = 0; i < m_pollDescs.size(); ++i)
    {
      struct pollfd& pollDesc = m_pollDescs[i];
      int id = m_monitoredFDbyPollDescs[i];
      const MonitoredFD& monitoredFD = m_monitoredFDs[id];

      if (pollDesc.revents)
      {
        if (monitoredFD.callback)
        {
          monitoredFD.callback(id, pollDesc.fd, pollDesc.revents,
                               monitoredFD.callbackData);
        }
        
        if (pollDesc.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          // ERROR LOG and REMOVE
        }

        pollDesc.revents = 0;
      }
    }
  }
}

void CFDEventMonitor::AddFDLocked(const MonitoredFD& monitoredFD, int& id)
{
  id = m_nextID;
  
  while (m_monitoredFDs.count(id))
  {
    ++id;
  }
  m_nextID = id + 1;

  m_monitoredFDs[id] = monitoredFD;
  
  AddPollDesc(id, monitoredFD.fd, monitoredFD.events);
}

void CFDEventMonitor::AddPollDesc(int id, int fd, short events)
{
  struct pollfd newPollFD;
  newPollFD.fd = fd;
  newPollFD.events = events;
  newPollFD.revents = 0;
  
  m_pollDescs.push_back(newPollFD);
  m_monitoredFDbyPollDescs.push_back(id);
}

void CFDEventMonitor::UpdatePollDescs()
{
  m_monitoredFDbyPollDescs.clear();
  m_pollDescs.clear();

  for (std::map<int, MonitoredFD>::iterator it = m_monitoredFDs.begin();
       it != m_monitoredFDs.end(); ++it)
  {
    AddPollDesc(it->first, it->second.fd, it->second.events);
  }
}

void CFDEventMonitor::StartMonitoring()
{
  if (!IsRunning())
  {
    /* Start the monitoring thread */

    m_wakeupfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (m_wakeupfd < 0)
    {
      // ERROR LOG and FAIL
      return;
    }

    /* Add wakeup fd to the fd list */
    int id;
    AddFDLocked(MonitoredFD(m_wakeupfd, POLLIN, NULL, NULL), id);

    Create(false);
  }
}

void CFDEventMonitor::InterruptPoll()
{
  if (m_wakeupfd >= 0)
  {
    eventfd_write(m_wakeupfd, 1);
    /* wait for the poll() result handling (if any) to end */
    CSingleLock pollLock(m_pollMutex);
  }
}

#endif
