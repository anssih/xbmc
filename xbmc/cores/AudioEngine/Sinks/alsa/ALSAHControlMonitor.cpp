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

#include "ALSAHControlMonitor.h"

#include "AEFactory.h"
#include "linux/FDEventMonitor.h"
#include "utils/log.h"

CALSAHControlMonitor::CALSAHControlMonitor()
{
}

CALSAHControlMonitor::~CALSAHControlMonitor()
{
  Clear();
}

bool CALSAHControlMonitor::Add(const std::string& ctlHandleName,
                               snd_ctl_elem_iface_t interface,
                               unsigned int device,
                               const std::string& name)
{
  bool ok = false;
  snd_hctl_t *hctl;

  if (snd_hctl_open(&hctl, ctlHandleName.c_str(), 0) == 0)
  {
    if (snd_hctl_load(hctl) == 0)
    {
      snd_hctl_elem_t *elem;
      snd_ctl_elem_id_t *id;

      snd_ctl_elem_id_alloca(&id);

      snd_ctl_elem_id_set_interface(id, interface);
      snd_ctl_elem_id_set_name     (id, name.c_str());
      snd_ctl_elem_id_set_device   (id, device);

      elem = snd_hctl_find_elem(hctl, id);
      if (elem) {
        snd_hctl_elem_set_callback(elem, HCTLCallback);
        snd_hctl_nonblock(hctl, 1);
        m_ctlHandles.push_back(hctl);
        ok = true;
      }
    }

    /* in OK case we keep it open */
    if (!ok)
    {
      snd_hctl_close(hctl);
    }
  }

  return ok;
}

void CALSAHControlMonitor::Clear()
{
  Stop();

  for (unsigned int i = 0; i < m_ctlHandles.size(); ++i)
  {
    snd_hctl_close(m_ctlHandles[i]);
  }
  m_ctlHandles.clear();
}

void CALSAHControlMonitor::Start()
{
  assert(m_fdMonitorIds.size() == 0);

  std::vector<struct pollfd> pollfds;
  std::vector<CFDEventMonitor::MonitoredFD> monitoredFDs;
  
  for (unsigned int i = 0; i < m_ctlHandles.size(); ++i)
  {
    pollfds.resize(snd_hctl_poll_descriptors_count(m_ctlHandles[i]));
    int fdcount = snd_hctl_poll_descriptors(m_ctlHandles[i], &pollfds[0], pollfds.size());

    for (int j = 0; j < fdcount; ++j)
    {
      monitoredFDs.push_back(CFDEventMonitor::MonitoredFD(pollfds[j].fd,
                                                          pollfds[j].events,
                                                          FDEventCallback,
                                                          m_ctlHandles[i]));
    }
  }

  g_fdEventMonitor.AddFDs(monitoredFDs, m_fdMonitorIds);
}


void CALSAHControlMonitor::Stop()
{
  g_fdEventMonitor.RemoveFDs(m_fdMonitorIds);
  m_fdMonitorIds.clear();
}

int CALSAHControlMonitor::HCTLCallback(snd_hctl_elem_t *elem, unsigned int mask)
{
  /*
   * Currently we just re-enumerate on any change.
   * Custom callbacks for handling other control monitoring may be implemented when needed.
   */
  if (mask & SND_CTL_EVENT_MASK_VALUE)
  {
    CAEFactory::DeviceChange();
  }

  return 0;
}

void CALSAHControlMonitor::FDEventCallback(int id, int fd, short revents, void *data)
{
  /* Run ALSA event handling when the FD has events */
  snd_hctl_t *hctl = (snd_hctl_t *)data;
  snd_hctl_handle_events(hctl);
}

#endif
