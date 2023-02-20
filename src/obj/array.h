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

#ifndef array_HDRINCLUDED
#define array_HDRINCLUDED

#include <stddef.h>

#include "features.h"

#if ARRAY_LIMIT > 0

struct react_corestr;

/* Allocate one element and return its index, or return (index_type)
   -1 on failure. */
index_type react_arralloc(struct react_corestr *);

/* Release element i. */
void react_arrfree(struct react_corestr *, index_type i);

typedef void arrproc_t(struct react_corestr *,
                       elem_type *, ref_type *);

/* Ensure that the next n calls to alloc will succeed.  Return 0 on
   success and -1 on failure.  init is invoked to initialize each
   newly allocated element. */
int react_arrensure(struct react_corestr *, size_t n,
                    arrproc_t *init);

#if 0
/* Allocate [n] new elements at the end of the array, possibly
   resizing the array.  The index of the first new element is
   returned, or ((index_type) -1) on failure. */
index_type react_arralloc(struct react_corestr *, index_type n);

/* Free [n] adjecent elements in the array, starting at [s].  Upto [n]
   elements will be moved to replace them.  Return the number that are
   moved. */
index_type react_arrfree(struct react_corestr *, index_type s,
                         index_type n);
#endif

#define react_arr2refs(C,A) (&(C)->sysbuf.base[(A)-(C)->sysbuf.ref_base])
#define react_refs2arr(C,R) (&(C)->sysbuf.ref_base[(R)-(C)->sysbuf.base])

#endif


#endif
