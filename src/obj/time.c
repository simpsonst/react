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

#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include "mytime.h"

#if defined TIMEFMT_WINDOWS
/* We're using FILETIME as our moment_type, and DWORD as our
   delay_type. */
const char *react_systime_fmt(const moment_type *t)
{
  ULARGE_INTEGER *ai = (void *) t;
  unsigned long long ll = ai->QuadPart;
  static char buf[100];
  snprintf(buf, sizeof buf, "%llu", ll);
  return buf;
}

const char *react_systime_fmtdelay(const delay_type *d)
{
  static char buf[100];
  snprintf(buf, sizeof buf, "%lums", (unsigned long) *d);
  return buf;
}

int react_systime_cmp(const moment_type *a, const moment_type *b)
{
  ULARGE_INTEGER *ai = (void *) a;
  ULARGE_INTEGER *bi = (void *) b;

  return bi->QuadPart < ai->QuadPart ? +1 :
    bi->QuadPart > ai->QuadPart ? -1 : 0;
}

void react_systime_zero(delay_type *a)
{
  *a = 0;
}

int react_systime_diff(delay_type *r,
                       const moment_type *a,
                       const moment_type *b)
{
  ULARGE_INTEGER *ai = (void *) a;
  ULARGE_INTEGER *bi = (void *) b;

  if (ai->QuadPart < bi->QuadPart) {
    *r = 0;
    return 1;
  }
  ULONGLONG d = ai->QuadPart - bi->QuadPart;
#if 0
  printf("The event time is %llu hns\n", (unsigned long long) ai->QuadPart);
  printf("The current time is %llu hns\n", (unsigned long long) bi->QuadPart
  printf("The delay is %llu(%llx) hns\n", (unsigned long long) d);
#endif
  d /= 10000;
  if (d > ~(DWORD) 0)
    *r = ~(DWORD) 0;
  else
    *r = d;
  return 0;
}

int react_systime_now(moment_type *spec)
{
  GetSystemTimeAsFileTime(spec);
#if 0
  printf("The current time is %llu(%llx) hns\n",
         *(unsigned long long *) spec,
         *(unsigned long long *) spec);
#endif
  return 0;
}




#elif defined TIMEFMT_TIMESPEC
/* We're using struct timespec as our moment_type and delay_type. */

const char *react_systime_fmt(const moment_type *t)
{
  static char buf[100];
  snprintf(buf, sizeof buf, "%ld.%09lds", t->tv_sec, t->tv_nsec);
  return buf;
}

const char *react_systime_fmtdelay(const delay_type *d)
{
  return react_systime_fmt(d);
}

int react_systime_cmp(const moment_type *a, const moment_type *b)
{
  long d = a->tv_sec - b->tv_sec;
  if (d) return d < 0 ? -1 : +1;
  d = a->tv_nsec - b->tv_nsec;
  return d ? d < 0 ? -1 : +1 : 0;
}

void react_systime_zero(delay_type *a)
{
  a->tv_sec = a->tv_nsec = 0;
}

int react_systime_diff(delay_type *r,
                       const moment_type *a,
                       const moment_type *b)
{
  r->tv_sec = a->tv_sec - b->tv_sec;
  r->tv_nsec = a->tv_nsec - b->tv_nsec;
  if (r->tv_nsec < 0) {
    r->tv_sec--;
    r->tv_nsec += 1000000000;
  }
  if (r->tv_sec < 0) {
    r->tv_sec = r->tv_nsec = 0;
    return 1;
  }
  return 0;
}

#if __STDC_VERSION__ >= 201112L
/* struct timespec is now a standard type. */
int react_systime_now(moment_type *spec)
{
  return timespec_get(spec, TIME_UTC);
}
#elif _POSIX_C_SOURCE >= 199309L
/* POSIX defines a function to directly obtain a struct timespec for
   the current real time. */
#include <time.h>
int react_systime_now(moment_type *spec)
{
  return clock_gettime(CLOCK_REALTIME, spec);
}
#else
/* We don't know a way to get the real time directly as a struct
   timespec, so we get a struct timeval instead, and scale the
   microseconds to nanoseconds. */
#include <sys/time.h>

int react_systime_now(moment_type *spec)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0)
    return -1;

  spec->tv_sec = tv.tv_sec;
  spec->tv_nsec = tv.tv_usec * 1000;
  return 0;
}
#endif




#elif defined TIMEFMT_TIMEVAL || defined TIMEFMT_TIMEVALPOLL
/* We're using struct timeval as our moment_type, and either the same
   again or int as the delay_type in milliseconds. */
const char *react_systime_fmt(const moment_type *t)
{
  static char buf[100];
  snprintf(buf, sizeof buf, "%ld.%06lds", (long) t->tv_sec, t->tv_usec);
  return buf;
}

int react_systime_cmp(const moment_type *a, const moment_type *b)
{
  long d = a->tv_sec - b->tv_sec;
  if (d) return d < 0 ? -1 : +1;
  d = a->tv_usec - b->tv_usec;
  return d ? d < 0 ? -1 : +1 : 0;
}

int react_systime_now(moment_type *spec)
{
  return gettimeofday(spec, NULL);
}

#ifdef TIMEFMT_TIMEVAL
const char *react_systime_fmtdelay(const delay_type *d)
{
  return react_systime_fmt(d);
}

void react_systime_zero(delay_type *a)
{
  a->tv_sec = a->tv_usec = 0;
}

int react_systime_diff(delay_type *r,
                       const moment_type *a, const moment_type *b)
{
  r->tv_sec = a->tv_sec - b->tv_sec;
  r->tv_usec = a->tv_usec - b->tv_usec;
  if (r->tv_usec < 0) {
    r->tv_sec--;
    r->tv_usec += 1000000;
  }
  if (r->tv_sec < 0) {
    r->tv_sec = r->tv_usec = 0;
    return 1;
  }
  return 0;
}
#elif defined TIMEFMT_TIMEVALPOLL
const char *react_systime_fmtdelay(const delay_type *d)
{
  static char buf[100];
  snprintf(buf, sizeof buf, "%d.%06ds", *d / 1000000, *d % 1000000);
  return buf;
}

void react_systime_zero(delay_type *a)
{
  *a = 0;
}

#include <limits.h>
#include <stdint.h>

int react_systime_diff(delay_type *r,
                       const moment_type *a, const moment_type *b)
{
  intmax_t delay = a->tv_sec - b->tv_sec;
  delay *= 1000000;
  delay += a->tv_usec - b->tv_usec;
  if (delay < 0) {
    *r = 0;
    return 1;
  }

  if (delay > INT_MAX)
    delay = INT_MAX;
  *r = delay;
  return 0;
}

#else
#error "Delay format for TIMEVAL unknown"
#endif

#endif
