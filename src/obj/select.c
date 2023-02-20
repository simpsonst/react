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

#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "react/fd.h"
#include "react/event.h"
#include "fdextract.h"

#if KEEP_3FDSETS
static bool detect_event_in_use(struct react_corestr *core,
                                int fd, react_iomode_t mode)
{
  if (fd >= core->fdmap.size) return false;
  return core->fdmap.base[fd].elem[mode] != NULL;
}

static void change_fdcount(struct react_corestr *core,
                           react_iomode_t mode, int amount)
{
  core->fdsum.count += amount;
#if POLLCALL_RISCOS
  switch (mode) {
  default:
    break;
  case react_MOUT:
  case react_MEXC:
    core->fdsum.special += amount;
    break;
  }
#endif
}

/* Make sure that we have an entry for mapping a file descriptor and
   mode to a specific event handle. */
static int ensure_fd_index(struct react_corestr *core, int fd)
{
  assert(fd < FD_SETSIZE);
  if (fd < core->fdmap.size) return 0;

  /* Work out whether we need to increase the buffer, and by how
     much. */
  const int req = fd + 1;
  int nsize = core->fdmap.size;
  while (nsize < req) {
    nsize += 2;
    nsize *= nsize / 2;
    if (nsize >= FD_SETSIZE)
      nsize = FD_SETSIZE;
  }

  if (nsize > core->fdmap.size) {
    /* We don't have the capacity, so re-allocate. */
    fdmap_elem_type *nbase = realloc(core->fdmap.base, sizeof *nbase * nsize);
    if (nbase == NULL) {
      errno = react_ENOMEM;
      return -1;
    }
    core->fdmap.base = nbase;

    /* Create and initialize the new entries. */
    while (core->fdmap.size < nsize) {
      fdmap_elem_type *p = &core->fdmap.base[core->fdmap.size++];
      static fdmap_elem_type null;
      *p = null;
    }
  }

  return 0;
}





static void defuse_fd(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  /* We have one less <FD, mode> event to look for. */
  assert(core->fdsum.count > 0);
  change_fdcount(core, r->data.fd.mode, -1);

  /* Prevent future calls from watching for the event. */
  assert(FD_ISSET(r->data.fd.fd, &core->fdsum.in_set[r->data.fd.mode]));
  FD_CLR(r->data.fd.fd, &core->fdsum.in_set[r->data.fd.mode]);

  /* Remove the reverse mapping.  This also records that the FD event
     is available again. */
  assert(core->fdmap.base[r->data.fd.fd].elem[r->data.fd.mode] == r);
  core->fdmap.base[r->data.fd.fd].elem[r->data.fd.mode] = NULL;
}

int react_prime_fd(struct react_reg *r, int fd, react_iomode_t mode)
{
  react_cancel(r);

  /* We can only support FDs upto what select() supports. */
  if (fd < 0 || fd >= FD_SETSIZE) {
    errno = react_EINVAL;
    return -1;
  }

  switch (mode) {
  default:
    errno = react_EINVAL;
    return -1;
  case react_MIN:
  case react_MOUT:
  case react_MEXC:
    break;
  }

  struct react_corestr *core = r->core;

  /* Fail if someone is already watching this FD. */
  if (detect_event_in_use(core, fd, mode)) {
    errno = react_EEVENTINUSE;
    return -1;
  }

  /* Create the link from FD back to this handle. */
  if (ensure_fd_index(core, fd) < 0)
    return -1;
  core->fdmap.base[fd].elem[mode] = r;

  /* Update the shared FD set. */
  assert(!FD_ISSET(fd, &core->fdsum.in_set[mode]));
  FD_SET(fd, &core->fdsum.in_set[mode]);
  change_fdcount(core, mode, +1);
  if (fd >= core->fdsum.nfds)
    core->fdsum.nfds = fd + 1;
  
  /* Record defining characteristics. */
  r->data.fd.fd = fd;
  r->data.fd.mode = mode;
  r->defuse = &defuse_fd;
  r->act = &react_trigger;
  return 0;
}







static void defuse_fds(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  for (react_iomode_t m = 0; m < react_M_MAX; m++) {
    /* Find all FDs in the set, and remove from the core. */
    for (int fd = -1;
         react_findfd(&r->data.fds.in[m], r->data.fds.lim, &fd); ) {
      /* Ensure that the reactor is not watching for this event.  It
         might already have stopped if the event has been queued but
         not defused. */
      FD_CLR(fd, &core->fdsum.in_set[m]);

      assert(core->fdsum.count > 0);
      change_fdcount(core, m, -1);

      /* Remove the reverse mapping.  This also records that the FD event
         is available again. */
      assert(core->fdmap.base[fd].elem[m] == r);
      core->fdmap.base[fd].elem[m] = NULL;
    }
  }
}

static void on_fds(struct react_reg *r)
{
  struct react_corestr *core = r->core;
  const int fd = core->fdsum.fd;
  const react_iomode_t mode = core->fdsum.mode;

  /* We have one less <FD, mode> event to look for. */
  assert(core->fdsum.count > 0);
  change_fdcount(core, mode, -1);

  /* Prevent future calls from watching for the event. */
  assert(FD_ISSET(fd, &core->fdsum.in_set[mode]));
  FD_CLR(fd, &core->fdsum.in_set[mode]);

  /* Ensure the handle is queued. */
  react_queue(r);
}

int react_prime_fds(struct react_reg *r, int nfds,
                    fd_set *in, fd_set *out, fd_set *exc)
{
  struct react_corestr *core = r->core;

  react_cancel(r);

  /* Validate every FD, and find the highest-numbered, so we can
     allocate resources for all of them in one go. */
  fd_set *p[react_M_MAX] = { in, out, exc };
  int highest = -1;
  for (react_iomode_t m = 0; m < react_M_MAX; m++) {
    if (p[m] == NULL) continue;
    for (int fd = -1; react_findfd(p[m], nfds, &fd); ) {
      if (detect_event_in_use(core, fd, m)) {
        errno = react_EEVENTINUSE;
        return -1;
      }
      if (fd > highest) highest = fd;
    }
  }
  if (highest < 0) {
    /* No FDs were given. */
    errno = react_EINVAL;
    return -1;
  }
  if (ensure_fd_index(core, highest) < 0)
    return -1;

  /* Everything's okay, so iterate over the supplied data again and
     merge it with what the core is watching. */
  for (react_iomode_t m = 0; m < react_M_MAX; m++) {
    if (p[m] == NULL) {
      /* The user has not provided a set for this mode. */
      FD_ZERO(&r->data.fds.in[m]);
      continue;
    }

    /* Copy the entire FD set so we know which events this handle is
       looking for.  This tells us what to release when defusing. */
    r->data.fds.in[m] = *p[m];

    /* Record where the user wants the results stored, and zero it. */
    r->data.fds.out[m] = p[m];
    FD_ZERO(r->data.fds.out[m]);

    /* Find all FDs in the provided set. */
    for (int fd = -1; react_findfd(p[m], nfds, &fd); ) {
      /* Make the core look for this event in future, and keep
         count. */
      FD_SET(fd, &core->fdsum.in_set[m]);
      change_fdcount(core, m, +1);

      /* Create the link from FD to event handle. */
      assert(core->fdmap.base[fd].elem[m] == NULL);
      core->fdmap.base[fd].elem[m] = r;
    }
  }

  /* Make sure we are watching enough FDs. */
  if (highest >= core->fdsum.nfds)
    core->fdsum.nfds = highest + 1;

  r->data.fds.lim = nfds;
  r->defuse = &defuse_fds;
  r->act = &on_fds;
  return 0;
}

#endif // 3 FD sets
