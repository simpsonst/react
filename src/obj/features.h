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

#ifndef features_HDRINCLUDED
#define features_HDRINCLUDED


/*

   This internal header file selects the kind of implementation to use
   based on predefined preprocessor macros.

   A POLLCALL_* macro will be defined indicating how the system polls
   for events, e.g. by poll(), select(), pselect() or Windows system
   calls.

   A TIMEFMT_* macro will be defined to indicate the two time types
   used by the polling system for absolute and relative times
   (e.g. struct timeval, struct timespec, etc), and typedefs
   moment_type and delay_type will be correspondingly defined.

   A KEEP_* macro will be defined to indicate the way in which
   information about interesting asynchronous events will be stored.

   A further duty of the header is to include platform-specific
   headers appropriately.

*/

#if defined __unix || defined __unix__ || \
  defined __riscos || defined __riscos__
#include <sys/ioctl.h>
#define ioctlsocket(a, b, c) ioctl(a, b, c)
#endif

#ifdef __WIN32__
/* We'll just use the range of Windows calls for polling, including
   MsgWaitForMultipleObjectsEx. */
#define POLLCALL_WINDOWS 1
#define TIMEFMT_WINDOWS 1

#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <time.h>

typedef FILETIME moment_type;
typedef DWORD delay_type;

#define ARRAY_LIMIT (MAXIMUM_WAIT_OBJECTS - 1)
typedef DWORD index_type;
typedef HANDLE elem_type;
typedef struct {
  struct react_reg *ev;
  BOOL fire;
} ref_type;
#define ARRAY_NULL (core->unused_handle)




#elif defined __riscos || defined __riscos__
#define POLLCALL_RISCOS 1
#define TIMEFMT_TIMEVAL 1

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <swis.h>

#include <riscos/swi/OS.h>
#include <riscos/swi/Socket.h>
#include <riscos/swi/Wimp.h>

typedef struct timeval moment_type, delay_type;




#elif !defined DISABLE_SIGMASK && !defined DISABLE_POLL && defined _GNU_SOURCE
/* We'll use the ppoll() interface provide by GNU.  It gives us
   nanosecond precision and signal-safety. */
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define POLLCALL_PPOLL 1
#define ENABLE_SIGMASK 1

#define TIMEFMT_TIMESPEC 1
typedef struct timespec moment_type, delay_type;



#elif !defined DISABLE_POLL && \
  (defined _GNU_SOURCE || defined _XOPEN_SOURCE || _POSIX_C_SOURCE >= 199309L)
/* We'll use the poll() interface.  It uses int as a delay in
   milliseconds, so we will use struct timeval for an absolute time
   (because its minor component is also in milliseconds. */
#define POLLCALL_POLL 1
#define TIMEFMT_TIMEVALPOLL 1

#include <time.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

typedef int delay_type;
typedef struct timeval moment_type;


#elif !defined DISABLE_SIGMASK && \
  (_POSIX_C_SOURCE >= 199309L || _XOPEN_SOURCE >= 600)
/* We'll use the nanosecond-precision struct timespec and signal-safe
   pselect() from POSIX. */
#define POLLCALL_PSELECT 1
#define ENABLE_SIGMASK 1
#define TIMEFMT_TIMESPEC 1

#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct timespec moment_type, delay_type;



#elif defined __unix || defined __unix__
/* A lot of systems provide a select() call, even if they don't
   provide pselect(). */
#define POLLCALL_SELECT 1
#define TIMEFMT_TIMEVAL 1

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>


typedef struct timeval moment_type, delay_type;




#else
#error "No recognized polling system"

#endif




#if TIMEFMT_TIMESPEC
#define DELAY_FMT "%lu.%09lu (%s)"
#define DELAY_ARG(T) ((T) ? (unsigned long) (T)->tv_sec : 0ul), \
    ((T) ? (unsigned long) (T)->tv_nsec : 0ul),                 \
    ((T) ? "okay" : "null")
#elif TIMEFMT_TIMEVAL
#define DELAY_FMT "%lu.%06lu (%s)"
#define DELAY_ARG(T) ((T) ? (unsigned long) (T)->tv_sec : 0ul), \
    ((T) ? (unsigned long) (T)->tv_usec : 0ul),                 \
    ((T) ? "okay" : "null")
#elif TIMEFMT_TIMEVALPOLL
#define DELAY_FMT "%.3g (%s)"
#define DELAY_ARG(T) ((T) ? *(T) / 1000.0 : 0.0), ((T) ? "okay" : "null")
#endif

#if POLLCALL_RISCOS || POLLCALL_SELECT || POLLCALL_PSELECT
#define KEEP_3FDSETS 1
#endif



#if POLLCALL_PPOLL || POLLCALL_POLL
#include <poll.h>

/* We keep an array of pollfd structures to pass directly to ppoll,
   and a parallel array of structures to reference the event that
   asked for it, and which of that event's own array it corresponds
   to.*/
#define ARRAY_LIMIT 65535u
/* TODO: Allow the limit to be dynamic, i.e., dependent on
   RLIMIT_NOFILE.  However, does this limitation of (p)poll imply that
   all FDs in the array passed to (p)poll must be unique? */
typedef nfds_t index_type;
typedef struct pollfd elem_type;
typedef struct {
  struct react_reg *ev;
  unsigned offset; // Offset into user's pollfd array
  index_type next; // Singly-linked list
  short origevs; // Original events
  int fd;
} ref_type;

#define ARRAY_NULL ((struct pollfd) { .fd = -1 })

#define KEEP_POLLREC 1

#endif


#endif
