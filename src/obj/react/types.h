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

/* This header defines only arithmetic and private types.  It is
   guaranteed to include <react/features.h>.  To fully define all its
   types, headers like <stddef.h>, <winsock2.h> or <windows.h> should
   be included first.  It can be included again to complete the
   missing definitions. */

#include "features.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef react_types_HDRINCLUDED
#define react_types_HDRINCLUDED
  struct react_corestr;
  struct react_reg;

  /* Re-tasked error codes */
#define react_EINVAL EINVAL
#define react_EBADSTATE EBUSY
#define react_ENOMEM ENOMEM
#define react_ECOREBUSY EBUSY
#define react_EEVENTINUSE EADDRINUSE

  /* This is an opaque handle for a reactor.  react_COREERROR
     symbolizes a permanent invalid value. */
  typedef struct react_corestr *react_core_t;
  typedef react_deprecated(react_core_t react_core);
#define react_COREERROR ((struct react_corestr *) 0)

  /* Users of a reactor must express behaviour as a function with this
     prototype, plus a generic-pointer value to be given as its first
     argument. */
  typedef void react_proc_t(void *);
  typedef react_deprecated(react_proc_t react_proc);

#if react_ALLOW_WIMPEV
  typedef void react_wimpev_proc_t(void *, unsigned evtype, void *buf);
#endif

#if react_ALLOW_WIMPMSG
  typedef void react_wimpmsg_proc_t(void *, unsigned evtype,
                                    unsigned msgtype, void *buf);
#endif

  /* This is an opaque handle that binds an event to a behaviour and a
     priority.  react_ERROR symbolizes a permanent invalid value. */
  typedef struct react_reg *react_t;
#define react_ERROR ((struct react_reg *) 0)
  typedef react_deprecated(react_t react_event);

  /* Portable I/O modes */
  typedef enum {
    react_MIN,
    react_MOUT,
    react_MINPRI,
    react_MEXC = react_MINPRI,
    react_M_MAX
  } react_iomode_t;
  typedef react_deprecated(react_iomode_t react_iomode);

  /* Every event has a major priority (or just 'priority') and a minor
     priority (or 'subpriority').  Major priorities run from 0
     (highest) to react_MAXPRIOS - 1 (lowest), and minor priorities
     similarly run from 0 (highest) to react_MAXSUBPRIOS - 1 (lowest).

     When an event of a higher major priority is processed
     (i.e., the user's function is called), events of lower major
     priority are prevented from being processed until the next
     react_yield call.

     Minor priorities within a major priority merely impose an
     ordering when invoked by a single react_yield call. */

  /* This is a priority-queue subscript.  Lower numbers identify
     higher priorities. */
  typedef unsigned react_prio_t;
  typedef react_deprecated(react_prio_t react_prio);
#define react_MAXPRIOS 24

  /* This is a subpriority-queue subscript.  Lower numbers identify
     higher priorities.*/
  typedef unsigned react_subprio_t;
  typedef react_deprecated(react_subprio_t react_subprio);
#define react_MAXSUBPRIOS 24


#if react_ALLOW_FILE
#if react_IMPL_FILE_FD
  typedef int react_file_t;
#else
#error "Native file handle type unknown"
#endif
  typedef react_deprecated(react_file_t react_file);
#endif // native file handle allowed



#if react_ALLOW_PIPE
#if react_IMPL_PIPE_FD
  typedef int react_pipe_t;
#else
#error "Native pipe handle type unknown"
#endif
  typedef react_deprecated(react_pipe_t react_pipe);
#endif

#endif



#if react_ALLOW_SOCK

#ifdef __WIN32__
#ifdef INVALID_SOCKET
#ifndef react_sock_DEFINED
#define react_sock_DEFINED 1
  typedef SOCKET react_sock_t;
#define react_SOCKET_ERROR (SOCKET_ERROR)
#define react_INVALID_SOCKET (INVALID_SOCKET)
#endif
#endif

#ifdef APIENTRY
#ifndef react_ioctlflag_DEFINED
#define react_ioctlflag_DEFINED
  typedef u_long react_ioctlflag_t;
#endif
#endif

#ifndef react_socklen_DEFINED
#define react_socklen_DEFINED 1
  typedef int react_socklen_t;
#endif

#ifndef react_ssize_DEFINED
#define react_ssize_DEFINED 1
  typedef int react_ssize_t;
#endif

#ifndef react_buflen_DEFINED
#define react_buflen_DEFINED 1
  typedef int react_buflen_t;
#endif


#else

#include <sys/types.h>

#ifndef react_sock_DEFINED
#define react_sock_DEFINED 1
  typedef int react_sock_t;
#define react_SOCKET_ERROR (-1)
#define react_INVALID_SOCKET (-1)
#endif

#ifndef react_ioctlflag_DEFINED
#define react_ioctlflag_DEFINED
  typedef int react_ioctlflag_t;
#endif

#ifdef AF_UNSPEC
#ifndef react_socklen_DEFINED
#define react_socklen_DEFINED 1
  typedef socklen_t react_socklen_t;
#endif
#endif

#ifndef react_ssize_DEFINED
#define react_ssize_DEFINED 1
  typedef ssize_t react_ssize_t;
#endif

#ifndef react_buflen_DEFINED
#define react_buflen_DEFINED 1
  typedef size_t react_buflen_t;
#endif

#endif // non-Windows socket types

#if react_sock_DEFINED == 1
#undef react_sock_DEFINED
#define react_sock_DEFINED 2
  typedef react_deprecated(react_sock_t react_sock);
#endif

#endif // sockets allowed



#ifdef __cplusplus
}
#endif
