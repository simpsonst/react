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

#ifndef mytime_HDRINCLUDED
#define mytime_HDRINCLUDED

#include "common.h"

/* Now we declare common functions that operate on times.  See time.c
   for implementations. */

/* Return a value whose sign is the same as that of *a-*b. */
int react_systime_cmp(const moment_type *a, const moment_type *b);

/* Get the current time.  Return negative on failure; zero on
   success. */
int react_systime_now(moment_type *spec);

/* Express a zero-time delay. */
void react_systime_zero(delay_type *a);

/* Compute *a-*b in *r.  If this is negative, return non-zero, and
   possibly set *r to zero. */
int react_systime_diff(delay_type *r,
                       const moment_type *a, const moment_type *b);

const char *react_systime_fmt(const moment_type *);
const char *react_systime_fmtdelay(const delay_type *);

#endif
