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

/* This header defines only macros based on features of target
   environment and the capabilities of the compiler.  No types will be
   defined.  No functions will be declared.  No other headers will be
   included.  The header is provided to allow the user to detect
   compile-time capabilities of the library with minimum name
   pollution. */

#ifndef react_features_HDRINCLUDED
#define react_features_HDRINCLUDED

#ifdef __GNUC__
#define react_deprecated(D) D __attribute__ ((deprecated))
#else
#define react_deprecated(D) D
#endif

#if __STDC_VERSION__ >= 201112L || _POSIX_C_SOURCE >= 199309L
#define react_ALLOW_TIMESPEC 1
#endif

#if _POSIX_C_SOURCE >= 199309L
#define react_ALLOW_STDTIME 1
#endif

#if defined __unix || defined __unix__ || \
  defined __riscos || defined __riscos__ || \
  defined __WIN32__
#define react_ALLOW_TIMEVAL 1
#define react_ALLOW_STDTIME 1
#endif

#if defined __unix || defined __unix__ || \
  defined __riscos || defined __riscos__ || \
  defined __WIN32__
#define react_ALLOW_SOCK 1
#endif

#if _GNU_SOURCE && !(defined __riscos || defined __riscos__)
#define react_ALLOW_ACCEPT4 1
#endif

#ifdef __WIN32__
#define react_ALLOW_WINFILETIME 1
#define react_ALLOW_WINHANDLE 1
#define react_ALLOW_WINSOCK 1
#define react_IMPL_SOCK_WIN 1
#define react_ALLOW_WINMSG 1
#define react_ALLOW_WINAPC 1
#define react_ALLOW_STDFILE 1
#define react_IMPL_STDFILE_WINHANDLE 1
#define react_ALLOW_STDIN 1
#define react_IMPL_STDIN_WINHANDLE 1
#endif

#ifndef __WIN32__
#define react_ALLOW_INTR 1
#endif

#if (_GNU_SOURCE || _POSIX_C_SOURCE >= 200112L) \
  && !defined __riscos && !defined __riscos__
#define react_ALLOW_POLL 1
#define react_ALLOW_POLLS 1
#endif

#if defined __unix || defined __unix__
#define react_ALLOW_FD 1
#define react_IMPL_SOCK_FD 1
#define react_ALLOW_FILE 1
#define react_IMPL_FILE_FD 1
#define react_ALLOW_PIPE 1
#define react_IMPL_PIPE_FD 1
#define react_ALLOW_STDFILE 1
#define react_IMPL_STDFILE_FD 1
#define react_ALLOW_STDIN 1
#define react_IMPL_STDIN_STDFILE 1
#endif

#if react_ALLOW_FD && _GNU_SOURCE
#define react_ALLOW_FDSPLICE 1
#endif

#define react_OCF_SCL_ENABLED 0u

#if defined __riscos || defined __riscos__
#define react_ALLOW_WIMPEV 1
#define react_ALLOW_WIMPMSG 1
#define react_ALLOW_WIMPPW 1
#define react_IMPL_SOCK_RISCOS 1
#define react_OCF_SCL 1u
#if defined __CC__NORCROFT || (defined __GNUC__ && defined __TARGET_SCL__)
#undef react_OCF_SCL_ENABLED
#define react_OCF_SCL_ENABLED react_OCF_SCL
#endif
#endif

#if (_POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE) \
  && !defined __riscos && !defined __riscos__
#define react_HAVE_SIGSET 1
#endif

#define react_HAVE_SYSTIMEHDR 1

#endif
