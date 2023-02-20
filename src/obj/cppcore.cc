// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
 *  React++, C++ veneer for C reactor
 *  Copyright (C) 2001,2004-6,2012,2014,2016-7  Lancaster University
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact Steven Simpson <https://github.com/simpsonst>
 */

#include <cstdio>
#include <cerrno>
#include <system_error>

#include "features.h"
#include "react/core.hh"
#include "react/event.hh"
#include "teo.hh"

using namespace std;
using namespace react;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

react::prio_t react::Core_base::minprio()
{
  return react_minprio(core);
}

react::prio_t react::Core_base::maxprio()
{
  return react_maxprio(core);
}

#pragma GCC diagnostic pop


#if react_HAVE_SIGSET
::sigset_t &react::signal_mask(Core core)
{
  return *react_sigmask(core->core);
}
#endif

void react::Core_base::debug(FILE *fp, unsigned lvl)
{
  react_debug(core, fp, lvl);
}

Event react::Core_base::open(prio_t p, subprio_t sp, Handler *h)
{
  shared_ptr<Event_base> r = open();
  r->setprios(p, sp);
  if (h) r->direct(*h);
  return r;
}

react::Core_base::Core_base(react_core_t c)
  : core(c), destroy(false) { }

react::Core_base::Core_base(size_t prios)
  : core(react_opencore(prios)), destroy(true)
{
  if (core == react_COREERROR)
    throw_errno();
}

react::Core_base::Core_base()
  : core(react_opencore(0)), destroy(true)
{
  if (core == react_COREERROR)
    throw_errno();
}

react::Core_base::~Core_base()
{
  if (destroy)
    react_closecore(core);
}

Event react::Core_base::open()
{
  react_t h = react_open(core);
  if (h == react_ERROR)
    throw_errno();
  return Event(new Event_base(h));
}

bool react::Core_base::yield()
{
  if (react_yield(core) < 0) {
    if (errno == EAGAIN
#ifndef __WIN32__
        || errno == EWOULDBLOCK
#endif
        )
      return false;
    throw_errno();
  }
  return true;
}


Core react::newCore(size_t prios)
{
  return Core(new Core_base(prios));
}

Core react::newCore()
{
  return Core(new Core_base());
}
