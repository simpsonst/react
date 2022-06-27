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

#ifndef react_condtype_HDRINCLUDED
#define react_condtype_HDRINCLUDED

#include "features.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* The reactor supports several types of conditions.  Some are
     internal, and should not be used directly; some are portable
     (e.g. IDLE); some are semi-portable (e.g. TIMEVAL, the accuracy
     may not be fully portable); the remaining are
     platform-specific. */
  typedef enum {
    /* These types are internal, and should not be used by clients. */
    react_CNONE, /* Used to indicate that a binding has no associated
                    event. */
    react_CSYSTIME, /* Used to indicate a binding to a time expressed
                       in the polling routine's native type. */

    /* The remaining types are available at compile time, and may be
       platform-dependent. */

    react_CIDLE, /* The event should fire as long as there are no
                    other events of higher priorities.  No extra
                    arguments are required. */

    react_CSTDTIME, /* The event triggers at a specified time.  The
                       argument is a time_t const*. */

    react_CTIMEVAL, /* The event triggers at a specified time.  The
                       argument is a struct timeval const*. */

    react_CTIMESPEC, /* The event triggers at a specified time.  The
                        argument is a struct timespec const*. */

    react_CSTDFILE, /* The event triggers when a file can be accessed
                       without blocking.  The argument is a
                       react_stdfilecond_t const*. */

    react_CFILE, /* The event triggers when a file can be accessed
                    without blocking.  The argument is a
                    react_filecond_t const*. */

    react_CPIPE, /* The event triggers when a pipe can be accessed
                    without blocking.  The argument is a
                    react_pipecond_t const*. */

    react_CSOCK, /* The event triggers when a socket can be accessed
                    without blocking.  The argument is a
                    react_sockcond_t const*. */

    react_CFD, /* The event triggers when a descriptor can be accessed
                  without blocking.  The argument is a react_fdcond_t
                  const*. */

    react_CPOLL, /* The event triggers when a descriptor can be
                    accessed without blocking.  The argument is a
                    react_pollcond_t const*. */

    react_CWINSOCK, /* The event triggers when a socket can be
                       accessed without blocking.  The argument is a
                       react_winsockcond_t const*. */

    react_CWINFILETIME, /* The event triggers at the specified time.
                           The argument is a FILETIME const*. */

    react_CWINHANDLE, /* The event triggers when a semaphore
                         somethings.  The argument is a HANDLE
                         const*. */

    react_CWINHANDLEINT /* The event triggers when a semaphore
                           somethings.  The argument is a
                           react_whdlint_t const*. */

  } react_cond_t;
  typedef react_deprecated(react_cond_t react_condtype);

#ifdef __cplusplus
}
#endif

#endif
