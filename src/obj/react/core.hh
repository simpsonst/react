// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
 *  React - Event reactor for C
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
 *  Contact Steven Simpson ss at comp dot lancs dot ac dot uk
 */

#ifndef react_core_CXXHDRINC
#define react_core_CXXHDRINC

#include <csignal>
#include <memory>

#include "core.h"

#include "prio.hh"
#include "handler.hh"

namespace react {
  class Event_base;

  class Core_base;

  typedef std::shared_ptr<Core_base> Core;

#if react_HAVE_SIGSET
  ::sigset_t &signal_mask(Core);
#endif

  class Core_base {
    react_core_t core;
    bool destroy;

    std::shared_ptr<Event_base> open(prio_t p, subprio_t sp, Handler *h);

  public:
    Core_base(react_core_t);
    Core_base(size_t prios);
    Core_base();
    ~Core_base();

    react_core_t c_ref() { return core; }

#ifdef BUFSIZ
    void debug(FILE *, unsigned lvl);
#endif

    react_deprecated(prio_t minprio());
    react_deprecated(prio_t maxprio());

    std::shared_ptr<Event_base> open();

    std::shared_ptr<Event_base> open(Handler &h) { return open(0, 0, &h); }

    std::shared_ptr<Event_base> open(prio_t p, subprio_t sp, Handler &h) {
      return open(p, sp, &h);
    }

    std::shared_ptr<Event_base> open(Handler &h, prio_t p, subprio_t sp) {
      return open(p, sp, &h);
    }

    std::shared_ptr<Event_base> open(prio_t p, subprio_t sp) {
      return open(p, sp, NULL);
    }

    std::shared_ptr<Event_base> open(prio_t p, Handler &h) {
      return open(p, 0, &h);
    }

    std::shared_ptr<Event_base> open(prio_t p) {
      return open(p, 0, NULL);
    }

    std::shared_ptr<Event_base> open(Handler &h, prio_t p) {
      return open(p, 0, &h);
    }

    /* Yield functions only return false when they have nothing to do,
       i.e., they would otherwise block for ever. */
    bool yield();

#if react_HAVE_SIGSET
    friend ::sigset_t &signal_mask(Core);
#endif
  };

  Core newCore(size_t);
  Core newCore();
}

#endif
