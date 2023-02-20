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
 *  Contact Steven Simpson ss at comp dot lancs dot ac dot uk
 */


#include <cerrno>
#include <system_error>

#include "react/event.hh"
#include "teo.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void react::Event_base::setprio(prio_t p)
{
  if (react_setprio(h, p) < 0)
    throw_errno();
}

react_prio_t react::Event_base::getprio() { return react_getprio(h); }

#pragma GCC diagnostic pop

void react::Event_base::setprios(prio_t p, subprio_t sp)
{
  if (react_setprios(h, p, sp) < 0)
    throw_errno();
}

react::Event_base::Event_base(react_t h) : h(h), obj(0) { }

react::Event_base::~Event_base() { react_close(h); }

void react::Event_base::wrapper(void *vd)
{
  Event_base *d = static_cast<Event_base *>(vd);
  d->obj->react();
}

void react::Event_base::direct(Handler &obj)
{
  this->obj = &obj;
  react_direct(h, &wrapper, static_cast<void *>(this));
}

void react::Event_base::prime(Condition &cond)
{
  if (cond.prime(h) < 0)
    throw_errno();
}

void react::Event_base::cancel() { react_cancel(h); }
