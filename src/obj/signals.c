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
 *  Contact Steven Simpson <https://github.com/simpsonst>
 */

#include <errno.h>
#include <assert.h>
#include <signal.h>

#include "common.h"
#include "react/event.h"

#if react_HAVE_SIGSET
sigset_t *react_sigmask(struct react_corestr *core)
{
  /* We allow the user to manipulate this mask directly. */
  return &core->sigmask;
}
#endif

#if react_ALLOW_INTR
static void defuse_intr(struct react_reg *r)
{
  struct react_corestr *core = r->core;
  assert(core->sig_ev == r);
  core->sig_ev = NULL;
}

int react_prime_intr(struct react_reg *r)
{
  react_cancel(r);
  struct react_corestr *core = r->core;
  if (core->sig_ev) {
    errno = react_EEVENTINUSE;
    return -1;
  }

  core->sig_ev = r;
  r->act = &react_queue; /* Stay in the core, so further interrupts
                            will still be captured (albeit
                            indistinctly) by this handle. */
  r->defuse = &defuse_intr;
  return 0;
}
#endif
