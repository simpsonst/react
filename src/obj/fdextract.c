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

#include <assert.h>
#include <string.h>

#include "features.h"

#if POLLCALL_PSELECT || POLLCALL_SELECT || POLLCALL_RISCOS
#include "fdextract.h"

int react_findfd(const fd_set *fds, int nfds, int *pos)
{
  assert(nfds <= FD_SETSIZE);
#if defined NFDBITS && defined __USE_XOPEN && \
  defined __USE_GNU && defined __USE_MISC
  /* This is a Linux-aware optimisation of this function, using probably
     non-future-proof extracts from the system header files. */
  const unsigned lim = (nfds + NFDBITS - 1) / NFDBITS;
  assert(lim <= sizeof fds->fds_bits / sizeof fds->fds_bits[0]);
  for (unsigned i = (++*pos + NFDBITS - 1) / NFDBITS; i < lim; i++) {
    fd_mask mask = -1;
    mask <<= *pos % NFDBITS;
    int r = ffsl(fds->fds_bits[i] & mask);
    if (r <= 0)
      continue;
    r = r - 1 + i * NFDBITS;
    if (r >= nfds)
      return 0;
    assert(FD_ISSET(r, fds));
    *pos = r;
    return 1;
  }
  return 0;
#else
  /* This is the entirely portable way to search for a set bit in an
     fd_set.  It's also rather linear. */
  while (++*pos < nfds)
    if (FD_ISSET(*pos, fds))
      return 1;
  return 0;
#endif
}

#endif
