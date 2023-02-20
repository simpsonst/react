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
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "common.h"
#include "react/event.h"

/* Ensure that there are sufficient queues for an event with the
   specified priority and subpriority.  Return negative on failure
   (e.g., react_ENOMEM for out of memory, EDOM for excessive
   priorities). */
static int ensure_queues(struct react_corestr *core,
                         react_prio_t prio, react_subprio_t subprio)
{
  /* Check that we don't exceed fixed limits. */
  if (prio >= react_MAXPRIOS || subprio >= react_MAXSUBPRIOS) {
    errno = EDOM;
    return -1;
  }

  {
    /* Check whether we have enough space for the major queue. */
    size_t newalloc = core->queues.alloc;
    while (prio >= newalloc) {
      newalloc += 4;
      newalloc *= 2;
      if (newalloc >= react_MAXPRIOS) {
        newalloc = react_MAXPRIOS;
        break;
      }
    }
    assert(newalloc >= core->queues.alloc);
    assert(newalloc <= react_MAXPRIOS);
    assert(newalloc > prio);

    /* Make space for additional queues, if necessary. */
    if (newalloc > core->queues.alloc) {
      void *newbase = realloc(core->queues.base,
                              newalloc * sizeof *core->queues.base);
      if (!newbase) {
        errno = react_ENOMEM;
        return -1;
      }
      core->queues.base = newbase;
      core->queues.alloc = newalloc;
    }
  }

  /* Initialize new queue sets. */
  bool all_done = core->queues.top == core->queues.size;
  while (core->queues.size <= prio) {
    struct queue_set *sq = &core->queues.base[core->queues.size];
    sq->size = 0;
    sq->alloc = 0;
    sq->base = NULL;
    core->queues.size++;
  }
  if (all_done)
    core->queues.top = core->queues.size;

  assert(core->queues.size > prio);

  struct queue_set *sq = &core->queues.base[prio];
  {
    /* Check whether we have enough space for the minor queue within
       the specified major queue. */
    size_t newalloc = sq->alloc;
    while (subprio >= newalloc) {
      newalloc += 4;
      newalloc *= 2;
      if (newalloc >= react_MAXSUBPRIOS) {
        newalloc = react_MAXSUBPRIOS;
        break;
      }
    }
    assert(newalloc >= sq->alloc);
    assert(newalloc <= react_MAXSUBPRIOS);

    /* Make space for additional queues, if necessary. */
    if (newalloc > sq->alloc) {
      void *newbase = realloc(sq->base, newalloc * sizeof *sq->base);
      if (!newbase) {
        errno = react_ENOMEM;
        return -1;
      }
      sq->base = newbase;
      sq->alloc = newalloc;
    }
  }

  /* Initialize the subqueues. */
  while (sq->size <= subprio) {
    dllist_init(&sq->base[sq->size]);
    sq->size++;
  }

  assert(sq->size > subprio);
  return 0;
}

struct react_reg *react_open(struct react_corestr *core)
{
  /* Make sure this core has at least one queue. */
  if (ensure_queues(core, 0, 0) < 0)
    return react_ERROR;

  /* Try to allocate an event handle. */
  struct react_reg *r = malloc(sizeof *r);
  if (!r) {
    errno = react_ENOMEM;
    return react_ERROR;
  }

  /* Zero-initialize all fields. */
  static const struct react_reg null;
  *r = null;

  /* Associate this event with this core. */
  r->core = core;
  dllist_append(&core->members, in_core, r);

  return r;
}

void *react_ensuremem(struct react_reg *r, size_t sz)
{
  if (sz != 0 && (sz > r->dyn.sz || sz < r->dyn.sz / 3)) {
    void *np = realloc(r->dyn.mem, sz);
    if (!np) return NULL;
    r->dyn.mem = np;
    r->dyn.sz = sz;
  }
  return r->dyn.mem;
}

void react_close(struct react_reg *r)
{
  if (r == react_ERROR) return;

  react_cancel(r);
  react_close(r->subev);
  if (r->core)
    dllist_unlink(&r->core->members, in_core, r);
  free(r->dyn.mem);
  free(r);
}

struct react_reg *react_ensuresub(struct react_reg *r)
{
  if (r->subev == react_ERROR)
    r->subev = react_open(r->core);
  return r->subev;
}

void react_direct(struct react_reg *r, react_proc_t *proc, void *proc_data)
{
  if (r->queued) return;
  r->proc = proc;
  r->proc_data = proc_data;
}

react_prio_t react_getprio(struct react_reg *r)
{
  return r->prio;
}

void react_getprios(struct react_reg *r, react_prio_t *p, react_subprio_t *sp)
{
  *p = r->prio;
  *sp = r->subprio;
}

int react_setprios(struct react_reg *r, react_prio_t p, react_subprio_t sp)
{
  if (r->queued) {
    errno = react_EBADSTATE;
    return -1;
  }

  if (ensure_queues(r->core, p, sp) < 0)
    return -1;

  r->prio = p;
  r->subprio = sp;
  
  return 0;
}

int react_setprio(struct react_reg *r, react_prio_t p)
{
  return react_setprios(r, p, r->subprio);
}

int react_setsubprio(struct react_reg *r, react_subprio_t p)
{
  return react_setprios(r, r->prio, p);
}

react_prio_t react_minprio(struct react_corestr *core)
{
  return react_MAXPRIOS - 1;
}

react_prio_t react_maxprio(struct react_corestr *core)
{
  return 0;
}

int react_isprimed(struct react_reg *r)
{
  return r->defuse != 0;
}

int react_istriggered(struct react_reg *r)
{
  return r->queued && r->defuse == 0;
}

int react_isqueued(struct react_reg *r)
{
  return r->queued;
}

int react_isactive(struct react_reg *r)
{
  return r->defuse != 0 || r->queued;
}

void react_dequeue(struct react_reg *r)
{
  if (!r || !r->queued) return;

  react_prio_t p = r->prio;
  assert(p < r->core->queues.size);
  struct queue_set *sq = &r->core->queues.base[p];
  react_prio_t sp = r->subprio;
  assert(sp < sq->size);
  dllist_unlink(&sq->base[sp], in_queue, r);
  r->queued = 0;
}

void react_queue(struct react_reg *r)
{
  /* Only event handles with user actions get queued. */
  if (!r->proc) return;

  if (r->queued) return;

  assert(r->prio < r->core->queues.size);
  struct queue_set *sq = &r->core->queues.base[r->prio];
  assert(r->subprio < sq->size);

  /* If it's a higher priority than the current 'top' queue, make this
     the 'top' queue.  This tells us where to search for a job to be
     done. */
  if (r->prio < r->core->queues.top)
    r->core->queues.top = r->prio;

  /* Add the event to the configured queue. */
  dllist_append(&sq->base[r->subprio], in_queue, r);
  r->queued = 1;
}

void react_trigger(struct react_reg *r)
{
  react_queue(r);
  react_defuse(r);
}

void react_defuse(struct react_reg *r)
{
  if (r) {
    defuse_proc_t *f = r->defuse;
    if (f) {
      r->defuse = 0;
      (*f)(r);
    }
    react_defuse(r->subev);
  }
}

void react_cancel(struct react_reg *r)
{
  react_defuse(r);
  react_dequeue(r);
}

void react_swapact(struct react_reg *r, action_proc_t *f, action_proc_t **fp)
{
  *fp = r->act;
  r->act = f;
}
