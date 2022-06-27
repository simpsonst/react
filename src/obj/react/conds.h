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

/* Everything in here will eventually be deprecated.  It's the old way
   of passing arbitrary parameters to a single priming function, now
   replaced by multiple (and platform-specific) priming functions that
   take the right arguments. */

#ifndef react_conds_HDRINCLUDED
#define react_conds_HDRINCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "features.h"
#include "types.h"

#if react_ALLOW_FD
  typedef struct {
    int handle;  /* the file-descriptor */
    react_iomode_t mode;
  } react_fdcond_t;

  typedef react_deprecated(react_fdcond_t react_fdcond);
#endif

#if react_ALLOW_SOCK
#if react_IMPL_SOCK_FD
  typedef react_fdcond_t react_sockcond_t;
#define react_sockcond_DEFINED 1
#elif react_IMPL_SOCK_RISCOS
  typedef struct {
    int handle;
    react_iomode_t mode;
  } react_sockcond_t;
#define react_sockcond_DEFINED 1
#elif react_IMPL_SOCK_WIN
#ifdef INVALID_SOCKET
  typedef struct {
    SOCKET handle;
    react_iomode_t mode;
  } react_sockcond_t;
#define react_sockcond_DEFINED 1
#endif
#else
#error "Native socket handle unknown"
#endif
#endif

#if react_sockcond_DEFINED
  typedef react_deprecated(react_sockcond_t react_sockcond);
#endif

#if react_ALLOW_FILE
#if react_IMPL_FILE_FD
  typedef react_fdcond_t react_filecond_t;
#else
#error "Native file handle unknown"
#endif
  typedef react_deprecated(react_filecond_t react_filecond);
#endif

#if react_ALLOW_PIPE
#if react_IMPL_PIPE_FD
  typedef react_fdcond_t react_pipecond_t;
#else
#error "Native pipe handle unknown"
#endif
  typedef react_deprecated(react_pipecond_t react_pipecond);
#endif

#if react_ALLOW_POLL
  typedef struct {
    int handle;
    short in_mode;
    short *out_mode;
  } react_pollcond_t;
  typedef react_deprecated(react_pollcond_t react_pollcond);
#endif

#if react_ALLOW_WINSOCK
#ifdef INVALID_SOCKET
  typedef struct {
    SOCKET handle;
    long mode; /* use modes from WSAEventSelect */
  } react_winsockcond_t;
  typedef react_deprecated(react_winsockcond_t react_winsockcond);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
