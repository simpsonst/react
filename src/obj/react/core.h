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
 *  Contact Steven Simpson <http://www.lancaster.ac.uk/~simpsons/>
 */

#ifndef react_core_HDRINCLUDED
#define react_core_HDRINCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "features.h"
#include "types.h"


  /* Create a new reactor.  The argument is now ignored, as cores no
     longer have fixed priorities. */
#define react_opencore(P) react_opencoref((P), 0)

  /* This is an internal function to obtain details of the
     application's environment by calling it from one of the macros
     above.  The application should just use react_opencore(p). */
  react_core_t react_opencoref(size_t prios, unsigned flags);

  /* Destroy a reactor. */
  void react_closecore(react_core_t);

  /* Get the priority range.

     Deprecated: Cores no longer have predefined minimum and maximum
     priorities.  Instead, the maximum priority is 0, and higher
     values represent lower priorities.  Hard limits are defined as
     macros, but the core will dynamically allocate more queues on
     demand, within those limits. */
  react_deprecated(react_prio react_minprio(react_core_t));
  react_deprecated(react_prio react_maxprio(react_core_t));

  /* Yield control to a reactor.  This is the portable version which
     denies access to any special platform-specific scheduling, such
     as Windows messages, WIMP events, etc.  It returns negative on
     error, with EGAIN/EWOULDBLOCK if there are no primed event
     handles, EINTR if a signal occurred, or possibly other values. */
  int react_yield(react_core_t);

#ifdef BUFSIZ
  void react_debug(react_core_t, FILE *, unsigned lvl);
#endif

#if react_HAVE_SIGSET
#ifdef SIG_DFL
  /* Get the signal mask.  These are the signals that will be blocked
     while waiting for events.  The set is empty by default, and
     probably should stay that way. */
  sigset_t *react_sigmask(react_core_t);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
