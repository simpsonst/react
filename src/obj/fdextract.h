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

#ifdef __cplusplus
#error "Do not compile as C++"
#endif

#ifndef fdextract_HDRINCLUDED
#define fdextract_HDRINCLUDED

/* Find the next FD in range (*pos, nfds) in *fds.  Return non-zero if
   found, or zero if not found. */
int react_findfd(const fd_set *fds, int nfds, int *pos);

#endif
