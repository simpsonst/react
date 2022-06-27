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

#include "features.h"

#ifdef __cplusplus
extern "C" {
#endif

#if react_ALLOW_TIMEVAL
  struct timeval;
  int react_prime_timeval(struct react_reg *, const struct timeval *);
#endif

#if react_ALLOW_TIMESPEC
  struct timespec;
  int react_prime_timespec(struct react_reg *, const struct timespec *);
#endif

#if react_ALLOW_WINFILETIME
#ifdef FillMemory
  int react_prime_winfiletime(struct react_reg *, const FILETIME *);
#endif
#endif

#if react_ALLOW_STDTIME
#ifdef CLOCKS_PER_SEC
  int react_prime_stdtime(struct react_reg *, const time_t *);
#endif
#endif

#ifdef __cplusplus
}
#endif
