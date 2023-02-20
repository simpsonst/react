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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "array.h"

#if ARRAY_LIMIT
static void print_status(struct react_corestr *core, const char *msg, ...)
{
#if 0
  size_t rem = core->sysbuf.cap - core->sysbuf.size;
  fprintf(stderr, "System buffer: %zu/%zu/%zu (%zu free; 1st=%zu) ",
          (size_t) core->sysbuf.size,
          (size_t) core->sysbuf.cap,
          (size_t) core->sysbuf.lim,
          rem,
          (size_t) core->sysbuf.free);
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  putc('\n', stderr);
  va_end(ap);
  for (size_t i = 0; i < core->sysbuf.cap; i++) {
    fprintf(stderr, "  %2zu:", i);
    fprintf(stderr, " (%5zu, %5zu)",
            (size_t) core->sysbuf.ref_base[i].prev,
            (size_t) core->sysbuf.ref_base[i].next);
    if (core->sysbuf.ref_base[i].next == ARRAY_LIMIT) {
      assert(core->sysbuf.ref_base[i].prev == ARRAY_LIMIT);
      fprintf(stderr, " in use");
    } else {
      fprintf(stderr, " free");
    }
    if (i == core->sysbuf.free)
      fprintf(stderr, " (1st)");
    fprintf(stderr, " %p", (void *) &core->sysbuf.ref_base[i]);
    putc('\n', stderr);
  }
#endif
}

index_type react_arralloc(struct react_corestr *core)
{
  if (core->sysbuf.size == core->sysbuf.cap) return -1;
  const index_type result = core->sysbuf.free;
  const index_type prev = core->sysbuf.ref_base[result].prev;
  const index_type next = core->sysbuf.ref_base[result].next;
  if (prev == result) {
    /* This is the last free element. */
    assert(next == result);
    assert(core->sysbuf.size + 1 == core->sysbuf.cap);
  } else {
    /* Unlink this element. */
    assert(next != result);
    assert(prev != result);
    if (core->sysbuf.free == result)
      core->sysbuf.free = next;
    core->sysbuf.ref_base[prev].next = next;
    core->sysbuf.ref_base[next].prev = prev;
  }

  /* Record this element as in use. */
  core->sysbuf.ref_base[result].next =
    core->sysbuf.ref_base[result].prev = ARRAY_LIMIT;
  core->sysbuf.size++;

  /* Update the approximate range of used elements. */
  if (result >= core->sysbuf.lim)
    core->sysbuf.lim = result + 1;

  assert(core->sysbuf.size <= core->sysbuf.lim);
  assert(core->sysbuf.lim <= core->sysbuf.cap);

  print_status(core, "allocated at %zu", (size_t) result);
  return result;
}

void react_arrfree(struct react_corestr *core, index_type i)
{
  assert(i < core->sysbuf.cap);
  assert(core->sysbuf.ref_base[i].next == ARRAY_LIMIT);
  assert(core->sysbuf.ref_base[i].prev == ARRAY_LIMIT);

  /* Link the element with other free elements. */
  if (core->sysbuf.size == core->sysbuf.cap) {
    /* There are no other free elements. */
    core->sysbuf.ref_base[i].prev = i;
    core->sysbuf.ref_base[i].next = i;
    core->sysbuf.free = i;
  } else {
    const index_type before = core->sysbuf.free;
    const index_type after = core->sysbuf.ref_base[before].next;
    core->sysbuf.ref_base[before].next = i;
    core->sysbuf.ref_base[after].prev = i;
    core->sysbuf.ref_base[i].next = after;
    core->sysbuf.ref_base[i].prev = before;

    /* Make earlier entries in the system buffer favoured for future
       allocation. */
    if (i < core->sysbuf.free)
      core->sysbuf.free = i;
  }

  /* Record this element as out of use. */
  core->sysbuf.base[i] = ARRAY_NULL;
  core->sysbuf.size--;

#if 0 /* This is not our job any more.  The caller should determine
         whether the last item in the array is unused, and
         decrease the range itself. */
  /* Update the approximate range of used elements. */
  if (core->sysbuf.lim > 0 && i == core->sysbuf.lim - 1 &&
      core->sysbuf.ref_base[core->sysbuf.lim - 1].next == ARRAY_LIMIT) {
    assert(core->sysbuf.ref_base[core->sysbuf.lim - 1].prev == ARRAY_LIMIT);
    core->sysbuf.lim--;
  }
#endif

  assert(core->sysbuf.size <= core->sysbuf.lim);
  assert(core->sysbuf.lim <= core->sysbuf.cap);

  print_status(core, "freed %zu", (size_t) i);
}

int react_arrensure(struct react_corestr *core, size_t n,
                    arrproc_t *init)
{
  if (n == 0) return 0;

  /* Do we already have enough? */
  const size_t rem = core->sysbuf.cap - core->sysbuf.size;
  if (rem >= n) return 0;

  /* How many do we need? */
  const size_t req = n - rem;
  size_t ncap = core->sysbuf.cap;

  /* What is the new target size?  Is it beyond what we permit? */
  const size_t target = ncap + req;
  if (target > ARRAY_LIMIT) {
    errno = react_ECOREBUSY;
    return -1;
  }

  /* Choose a new capacity that reaches the target. */
  while (ncap < target) {
    const size_t inc = 3 + ncap / 2;
    if (ncap > ARRAY_LIMIT - inc) {
      ncap = ARRAY_LIMIT;
      break;
    }
    ncap += inc;
  }
  assert(ncap >= target);
  assert(ncap <= ARRAY_LIMIT);

  const size_t extra = ncap - core->sysbuf.cap;

  /* Make the new allocation.  TODO: Can we allocate a single block by
     exploiting alignment calculations? */
  struct arrref *nl = realloc(core->sysbuf.ref_base, sizeof *nl * ncap);
  elem_type *na = realloc(core->sysbuf.base, sizeof *na * ncap);
  if (nl)
    core->sysbuf.ref_base = nl;
  if (na)
    core->sysbuf.base = na;
  if (!na || !nl) {
    errno = react_ENOMEM;
    return -1;
  }

  /* Link the new elements into the free list, and initialize them. */
  index_type before, after;
  if (core->sysbuf.size == core->sysbuf.cap) {
    /* There are no free elements. */
    before = ncap - 1;
    after = core->sysbuf.cap;
    core->sysbuf.free = core->sysbuf.cap;
  } else {
    before = core->sysbuf.free;
    after = core->sysbuf.ref_base[before].next;
  }
  for (size_t i = core->sysbuf.cap; i < ncap; i++) {
    struct arrref *ref = &core->sysbuf.ref_base[i];
    ref->next = i == ncap - 1 ? after : i + 1;
    ref->prev = i == core->sysbuf.cap ? before : i - 1;
    core->sysbuf.base[i] = ARRAY_NULL;
    (*init)(core, &core->sysbuf.base[i], &core->sysbuf.ref_base[i].user);
  }

  /* Link the existing elements in. */
  core->sysbuf.ref_base[before].next = core->sysbuf.cap;
  core->sysbuf.ref_base[after].prev = ncap - 1;

  core->sysbuf.cap = ncap;

  print_status(core, "increased cap for %zu by %zu", n, extra);
  return 0;
}
#endif
