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

#ifndef react_event_CXXHDRINC
#define react_event_CXXHDRINC

#include <memory>

#include "event.h"

#include "prio.hh"
#include "handler.hh"
#include "condition.hh"

namespace react {
  class Core_base;

  class Event_base {
    friend class Core_base;

    react_t h;
    Handler *obj;

    Event_base(react_t);

    static void wrapper(void *);

  public:
    virtual ~Event_base();

    react_deprecated(void setprio(prio_t));
    react_deprecated(prio_t getprio());
    void setprios(prio_t p, subprio_t sp);
    void getprios(prio_t &p, subprio_t &sp) { react_getprios(h, &p, &sp); }
    void direct(Handler &);
    void prime(Condition &);
    void cancel();

    bool primed() { return react_isprimed(h); }
    bool triggered() { return react_istriggered(h); }
    bool active() { return react_isactive(h); }
    void trigger() { react_trigger(h); }
  };

  typedef std::shared_ptr<Event_base> Event;
}

#endif
