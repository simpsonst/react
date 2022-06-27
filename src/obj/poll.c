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

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "array.h"
#include "react/event.h"
#include "react/fd.h"

#if POLLCALL_POLL || POLLCALL_PPOLL

static const short ALL_EVENTS = 0
  | POLLIN
  | POLLPRI
  | POLLOUT
#if _GNU_SOURCE
  | POLLRDHUP
#endif
  | POLLERR
  | POLLHUP
  | POLLNVAL
#if _XOPEN_SOURCE
  | POLLRDBAND
  | POLLWRBAND
#endif
  ;

static const short FATAL_EVENTS = POLLERR | POLLHUP | POLLNVAL;

static const short UNREQUESTED_EVENTS = POLLERR | POLLHUP | POLLNVAL;

static const short IN_EVENTS = POLLIN
#if _GNU_SOURCE
  | POLLRDHUP
#endif
  ;

static const short OUT_EVENTS = POLLOUT | POLLERR | POLLHUP | POLLNVAL;

static const short EXC_EVENTS = POLLPRI
#if _GNU_SOURCE
  | POLLRDBAND
  | POLLWRBAND
#endif
  ;

#if 0
static const char *evtext(short evs)
{
  static char buf[200];
  buf[0] = '\0';

  if (evs & POLLIN) strcat(buf, " IN");
  if (evs & POLLOUT) strcat(buf, " OUT");
  if (evs & POLLPRI) strcat(buf, " PRI");
  if (evs & POLLERR) strcat(buf, " ERR");
  if (evs & POLLHUP) strcat(buf, " HUP");
  if (evs & POLLNVAL) strcat(buf, " NVAL");

#if _GNU_SOURCE
  if (evs & POLLRDHUP) strcat(buf, " RDHUP");
#endif

#if _XOPEN_SOURCE
  if (evs & POLLRDNORM) strcat(buf, " RDNORM");
  if (evs & POLLWRNORM) strcat(buf, " WRNORM");
  if (evs & POLLRDBAND) strcat(buf, " RDBAND");
  if (evs & POLLWRBAND) strcat(buf, " WRBAND");
#endif
  return buf;
}
#endif

static int check_monitor(struct react_corestr *core, int fd, short evs)
{
  if ((unsigned) fd >= core->pollrec.size) return 0;
  if (ALL_EVENTS & evs & core->pollrec.base[fd]) {
#if 0
    fprintf(stderr, "%d%s", fd, evtext(evs));
    fprintf(stderr, " conflicts with%s\n", evtext(core->pollrec.base[fd]));
#endif
    errno = react_EEVENTINUSE;
    return -1;
  }
  return 0;
}

static void add_monitor(struct react_corestr *core, int fd, short evs)
{
  /* Only put in valid events that can be explicitly requested. */
#if 0
  fprintf(stderr, "added %d%s\n", fd, evtext(evs));
#endif
  assert((core->pollrec.base[fd] & evs) == 0);
  core->pollrec.base[fd] |= evs & ALL_EVENTS & ~UNREQUESTED_EVENTS;
}

static int ensure_pollrecs(struct react_corestr *core, unsigned maxfd)
{
  assert(maxfd <= INT_MAX);
  if (maxfd >= core->pollrec.size) {
    unsigned nsize = core->pollrec.size;
    while (maxfd >= nsize) {
      const unsigned inc = 2 + nsize / 2;
      if (nsize > INT_MAX - inc + 1) {
        nsize = INT_MAX + 1u;
        break;
      }
      nsize += inc;
    }
    if (nsize != core->pollrec.size) {
      void *nbase = realloc(core->pollrec.base,
                            nsize * sizeof *core->pollrec.base);
      if (nbase == NULL) {
        errno = react_ENOMEM;
        return -1;
      }
      core->pollrec.base = nbase;
      while (core->pollrec.size < nsize)
        core->pollrec.base[core->pollrec.size++] = 0;
    }
  }
  assert(core->pollrec.size > maxfd);
  return 0;
}

static void trim_array(struct react_corestr *core)
{
  /* Reduce the effective size of the array if the last element is
     unused. */
  if (core->sysbuf.lim > 0 && core->sysbuf.base[core->sysbuf.lim - 1].fd < 0)
    core->sysbuf.lim--;
}

static void defuse_polls(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  /* Delete all our poll entries from the core. */
  index_type next;
  for (index_type i = r->data.polls.tabpos; i != (index_type) -1; i = next) {
    /* De-allocate the entry in the system buffer. */
    core->sysbuf.base[i].fd = -1;
    ref_type *ref = &core->sysbuf.ref_base[i].user;
    assert(ref->ev == r);
    next = ref->next;
    react_arrfree(core, i);

    /* Mark the events as unwatched. */
    assert(ref->fd >= 0);
    core->pollrec.base[ref->fd] &= ~ref->origevs;
  }

  trim_array(core);
}

static void on_polls(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  /* Check whether this entry is even active. */
  elem_type *elem = &core->sysbuf.base[core->sysbuf.idx];
  if (elem->fd < 0) return;

  /* Find out which events have occurred. */
  ref_type *ref = &core->sysbuf.ref_base[core->sysbuf.idx].user;
  assert(ref->ev == r);
  const short evs = elem->revents;

  /* Add the events to the ones recorded, so the user will find out
     about them. */
  r->data.polls.base[ref->offset].revents |= evs;

  if (evs & FATAL_EVENTS) {
    /* We're done with this FD, as there's a fatal event on it.  Stop
       watching it in future. */
    elem->fd = -1;

    /* Note that we still retain it in our singly-linked list, so we
       don't de-allocate yet.  We also retain the events in our record
       of busy events, so that no-one can prime on it until our user
       has been notified. */
  }

  /* Ensure the entry is queued. */
  react_queue(r);
}

static void init_array(struct react_corestr *core,
                       elem_type *elem, ref_type *ref)
{
  elem->fd = -1;
  ref->ev = NULL;
}

int react_prime_polls(struct react_reg *r, struct pollfd *base, nfds_t len)
{
  react_cancel(r);

  struct react_corestr *core = r->core;

  /* Check to see if any events are already being waited for. */
  int maxfd = -1;
  unsigned valid = 0;
  for (nfds_t i = 0; i < len; i++) {
    const int fd = base[i].fd;
    if (fd < 0) continue;
    if (fd > maxfd) maxfd = fd;
    if (check_monitor(core, fd, base[i].events) < 0)
      return -1;
    valid++;
  }

  if (valid == 0) {
    errno = react_EINVAL;
    return -1;
  }

  /* Ensure we have enough entries in the pollrec structure to keep
     track of events we're monitoring. */
  if (ensure_pollrecs(core, maxfd) < 0)
    return -1;

  /* Allocate space for our poll entries. */
  if (react_arrensure(core, valid, &init_array) < 0)
    return -1;

  /* Copy our details in. */
  r->data.polls.tabpos = -1;
  for (nfds_t i = 0; i < len; i++) {
    /* Ignore disabled entries. */
    if (base[i].fd < 0) continue;

    /* Allocate and locate an element. */
    index_type pos = react_arralloc(core);
    assert(pos != (index_type) -1);
    struct pollfd *ep = &core->sysbuf.base[pos];
    ref_type *ref = &core->sysbuf.ref_base[pos].user;
    assert(ep->fd < 0);

    /* Copy the essential characteristics into the system buffer. */
    ep->fd = base[i].fd;
    ep->events = base[i].events & ALL_EVENTS;

    /* Keep the original FD and events in case the user corrupts the
       table. */
    ref->fd = base[i].fd;
    ref->origevs = ep->events;

    /* Zero the output returned to the user. */
    base[i].revents = 0;

    /* Establish the reverse reference. */
    ref->ev = r;
    ref->offset = i;

    /* Link the element in, so we can find all our system elements. */
    ref->next = r->data.polls.tabpos;
    r->data.polls.tabpos = pos;

    /* Record the events we're monitoring, so no-one else can prime on
       them. */
    add_monitor(core, base[i].fd, base[i].events);
  }

  /* Record the user details and specify the behaviour when an event
     occurs or the handle is cancelled. */
  r->data.polls.base = base;
  r->data.polls.size = len;
  r->defuse = &defuse_polls;
  r->act = &on_polls;

  return 0;
}

static void on_poll(struct react_reg *r)
{
  (*r->data.polls.act)(r);
  if (r->data.polls.proact.poll.got)
    *r->data.polls.proact.poll.got =
      r->data.polls.proact.poll.singleton.revents;
}

int react_prime_poll(struct react_reg *r, int fd, short evs, short *got)
{
  react_cancel(r);

  r->data.polls.proact.poll.got = got;
  r->data.polls.proact.poll.singleton.fd = fd;
  r->data.polls.proact.poll.singleton.events = evs;
  int rc = react_prime_polls(r, &r->data.polls.proact.poll.singleton, 1);
  if (rc == 0)
    react_swapact(r, &on_poll, &r->data.polls.act);
  return rc;
}

int react_prime_fd(struct react_reg *r, int fd, react_iomode_t mode)
{
  short m;
  switch (mode) {
  default:
    errno = react_EINVAL;
    return -1;

  case react_MIN:
    m = IN_EVENTS;
    break;

  case react_MOUT:
    m = OUT_EVENTS;
    break;

  case react_MINPRI:
    m = EXC_EVENTS;
    break;
  }

  return react_prime_poll(r, fd, m, NULL);
}

static void on_fds(struct react_reg *r)
{
  (*r->data.polls.act)(r);

  struct react_corestr *core = r->core;

  /* Check whether this entry is even active. */
  elem_type *elem = &core->sysbuf.base[core->sysbuf.idx];
  const int fd = elem->fd;
  if (fd < 0) return;

  /* Find out which element of our dynamic array we're dealing
     with. */
  ref_type *ref = &core->sysbuf.ref_base[core->sysbuf.idx].user;
  assert(ref->ev == r);
  struct pollfd *ptr = react_ensuremem(r, 0);
  ptr += ref->offset;

  /* Set bits in the original FD sets corresponding to sought
     events. */
  if (r->data.polls.proact.fds.out[react_MIN] && (ptr->revents & IN_EVENTS))
    FD_SET(fd, r->data.polls.proact.fds.out[react_MIN]);
  if (r->data.polls.proact.fds.out[react_MOUT] && (ptr->revents & OUT_EVENTS))
    FD_SET(fd, r->data.polls.proact.fds.out[react_MOUT]);
  if (r->data.polls.proact.fds.out[react_MINPRI] && (ptr->revents & EXC_EVENTS))
    FD_SET(fd, r->data.polls.proact.fds.out[react_MINPRI]);
}

int react_prime_fds(struct react_reg *r, int nfds,
                    fd_set *in, fd_set *out, fd_set *exc)
{
  /* Count how many poll entries we need. */
  int count = 0;
  for (int i = 0; i < nfds; i++)
    if ((in && FD_ISSET(i, in)) ||
        (out && FD_ISSET(i, out)) ||
        (exc && FD_ISSET(i, exc)))
      count++;
  if (count == 0) {
    errno = react_EINVAL;
    return -1;
  }

  /* Allocate space for the poll entries. */
  struct pollfd *ptr = react_ensuremem(r, sizeof *ptr * count);
  if (ptr == NULL) {
    errno = react_ENOMEM;
    return -1;
  }

  /* Convert the sets' contents into event masks. */
  for (int n = 0; n < count; n++) {
    ptr[n].fd = -1;
    ptr[n].events = 0;
  }
  int n = 0;
  for (int i = 0; i < nfds; i++) {
    if (in && FD_ISSET(i, in)) {
      ptr[n].fd = i;
      ptr[n].events |= IN_EVENTS;
    }
    if (out && FD_ISSET(i, out)) {
      ptr[n].fd = i;
      ptr[n].events |= OUT_EVENTS;
    }
    if (exc && FD_ISSET(i, exc)) {
      ptr[n].fd = i;
      ptr[n].events |= EXC_EVENTS;
    }
    if (ptr[n].fd >= 0)
      n++;
  }
  assert(n == count);

  /* Attempt to prime on this dynamic array, and inject our additional
     behaviour. */
  int rc = react_prime_polls(r, ptr, count);
  if (rc < 0) return rc;
  react_swapact(r, &on_fds, &r->data.polls.act);
  r->data.polls.proact.fds.out[react_MIN] = in;
  r->data.polls.proact.fds.out[react_MOUT] = out;
  r->data.polls.proact.fds.out[react_MINPRI] = exc;
  if (in) FD_ZERO(in);
  if (out) FD_ZERO(out);
  if (exc) FD_ZERO(exc);

  return 0;
}
#endif
