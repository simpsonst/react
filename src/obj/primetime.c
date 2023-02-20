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

#include "common.h"
#include "react/event.h"
#include "react/time.h"

static void defuse_time(struct react_reg *r)
{
  struct react_corestr *core = r->core;
  bheap_remove(&core->timed, r);
}

static int prime_systime(struct react_reg *r, const moment_type *arg)
{
  react_cancel(r);

  struct react_corestr *core = r->core;
  r->data.systime.when = *arg;
  bheap_insert(&core->timed, r);
  r->act = &react_trigger;
  r->defuse = &defuse_time;
  return 0;
}

#if react_ALLOW_STDTIME
int react_prime_stdtime(struct react_reg *r, const time_t *arg)
{
#if TIMEFMT_WINDOWS
  /* From <https://msdn.microsoft.com/en-us/library/windows/desktop/ms724228%28v=vs.85%29.aspx> */
  FILETIME alt;
  LONGLONG ll = Int32x32To64(*arg, 10000000) + 116444736000000000;
  alt.dwLowDateTime = (DWORD) ll;
  alt.dwHighDateTime = ll >> 32;
  return prime_systime(r, &alt);
#elif TIMEFMT_TIMEVAL || TIMEFMT_TIMEVALPOLL
  struct timeval alt = {
    .tv_sec = *arg,
    .tv_usec = 0
  };
  return prime_systime(r, &alt);
#elif TIMEFMT_TIMESPEC
  struct timespec alt = {
    .tv_sec = *arg,
    .tv_nsec = 0
  };
  return prime_systime(r, &alt);
#else
#error "No implementation"
  return -1;
#endif
}
#endif

#if react_ALLOW_TIMEVAL
int react_prime_timeval(struct react_reg *r, const struct timeval *arg)
{
#if TIMEFMT_WINDOWS
  union {
    FILETIME ft;
    ULARGE_INTEGER uli;
  } u;

  u.uli.QuadPart = 11644473600LL;
  u.uli.QuadPart += arg->tv_sec;
  u.uli.QuadPart *= 10000000LL;
  u.uli.QuadPart += arg->tv_usec * 10;

#if 0
  fprintf(stderr, "Primed on WINFILETIME: %p (%llu)\n", (void *) r,
          (unsigned long long) u.uli.QuadPart);
#endif
  return react_prime_winfiletime(r, &u.ft);
#elif TIMEFMT_TIMESPEC
  /* Convert the timeval into a timespec. */
  struct timespec x;
  x.tv_sec = arg->tv_sec;
  x.tv_nsec = arg->tv_usec * 1000;
  return react_prime_timespec(r, &x);
#elif TIMEFMT_TIMEVAL || TIMEFMT_TIMEVALPOLL
  return prime_systime(r, arg);
#else
#error "No implementation"
  return -1;
#endif
}
#endif

#if react_ALLOW_TIMESPEC
int react_prime_timespec(struct react_reg *r, const struct timespec *arg)
{
#if TIMEFMT_WINDOWS
  union {
    FILETIME ft;
    ULARGE_INTEGER uli;
  } u;

  u.uli.QuadPart = 11644473600LL;
  u.uli.QuadPart += arg->tv_sec;
  u.uli.QuadPart *= 10000000LL;
  u.uli.QuadPart += arg->tv_nsec / 100;

  return react_prime_winfiletime(r, &u.ft);
#elif TIMEFMT_TIMEVAL || TIMEFMT_TIMEVALPOLL
  /* Convert the timeval into a timespec. */
  struct timeval x;
  x.tv_sec = arg->tv_sec;
  x.tv_usec = arg->tv_nsec / 1000;
  return react_prime_timeval(r, &x);
#elif TIMEFMT_TIMESPEC
  return prime_systime(r, arg);
#else
#error "No implementation"
  return -1;
#endif
}
#endif

#if react_ALLOW_WINFILETIME
int react_prime_winfiletime(struct react_reg *r, const FILETIME *arg)
{
#if TIMEFMT_WINDOWS
  return prime_systime(r, arg);
#else
#error "No implementation"
  return -1;
#endif
}
#endif
