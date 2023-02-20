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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "common.h"
#include "react/event.h"
#include "react/fd.h"
#include "react/socket.h"
#include "react/windows.h"
#include "react/idle.h"
#include "react/condtype.h"
#include "react/conds.h"
#include "react/time.h"

int react_prime(struct react_reg *r, react_cond_t cond, const void *d)
{
  /* Ensure that the event belongs to the unused list, and is
     untriggered. */
  react_cancel(r);

  switch (cond) {
#if react_ALLOW_WINFILETIME
  case react_CWINFILETIME:
    return react_prime_winfiletime(r, d);
#endif

#if react_ALLOW_WINSOCK
  case react_CWINSOCK: {
    const react_winsockcond_t *cd = d;
    return react_prime_winsock(r, cd->handle, cd->mode, NULL, NULL);
  }
#endif

#if react_ALLOW_WINHANDLE
  case react_CWINHANDLE: {
    const HANDLE *cd = d;
    return react_prime_winhandle(r, *cd, NULL);
  }
#endif

#if react_ALLOW_FD
#if react_ALLOW_PIPE && react_IMPL_PIPE_FD
  case react_CPIPE:
#endif
#if react_ALLOW_FILE && react_IMPL_FILE_FD
  case react_CFILE:
#endif
  case react_CFD: {
    const react_fdcond_t *data = d;
    return react_prime_fd(r, data->handle, data->mode);
  }
#endif


#if react_ALLOW_SOCK
  case react_CSOCK: {
    const react_sockcond_t *data = d;
    return react_prime_sock(r, data->handle, data->mode);
  }
#endif

#if react_ALLOW_TIMEVAL
  case react_CTIMEVAL:
    return react_prime_timeval(r, d);
#endif

#if react_ALLOW_TIMESPEC
  case react_CTIMESPEC:
    return react_prime_timespec(r, d);
#endif

  case react_CIDLE:
    return react_prime_idle(r);

  default:
    errno = react_EINVAL;
    return -1;
  }
}
