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

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "common.h"
#include "mytime.h"

#include "react/core.h"
#include "react/event.h"

#if POLLCALL_RISCOS
#include <kernel.h>
#include <swis.h>
#include "module.h"
#endif

static int compare_times(void *vc, const void *va, const void *vb)
{
  const struct react_reg *ra = va;
  const struct react_reg *rb = vb;
  return react_systime_cmp(&ra->data.systime.when,
                           &rb->data.systime.when);
}

struct react_corestr *react_opencoref(size_t prios, unsigned flags)
{
#if POLLCALL_RISCOS
  /* We need to convert between two clocks, so we record the current
     epoch with both, and use these later.  We'll record them now
     before we try to allocate anything, since it's easier to deal
     with failure at this stage. */
  unsigned long long epoch_rt;
  unsigned epoch_mt;

  {
    /* Read the current monotonic time.  This is a 32-bit centisecond
       clock, reset at start-up, which will occasionally cycle after
       pronlonged periods. */
    _kernel_oserror *ep = _swix(OS_ReadMonotonicTime, _OUT(0), &epoch_mt);
    if (ep) {
      errno = ep->errnum;
      return react_COREERROR;
    }
  }

  {
    /* Read the current real time, to compare against the monotonic
       time. */
    struct timeval erk;
    if (gettimeofday(&erk, NULL) < 0)
      return react_COREERROR;
    epoch_rt = erk.tv_sec * 1000000ll + erk.tv_usec;
  }
#endif

  /* Create the basic core structure, with zero-initialized fields. */
  struct react_corestr *core = malloc(sizeof *core);
  if (!core) {
    errno = react_ENOMEM;
    goto error;
  }
  static const struct react_corestr null;
  *core = null;

#if POLLCALL_RISCOS
  core->epoch_rt = epoch_rt;
  core->epoch_mt = epoch_mt;

  {
    /* We need to get a pollword from the support module. */
    int val;
    _kernel_oserror *ep = _swix(XReactorHelp_ClaimWord, _OUT(0), &val);
    if (ep) {
      errno = ep->errnum;
      goto error;
    }
    core->pollword = (unsigned *) val;
  }
#endif

#if ENABLE_SIGMASK
  if (sigemptyset(&core->sigmask) < 0) {
    goto error;
  }
#endif

#if POLLCALL_WINDOWS
  // NULL doesn't work.
  core->unused_handle = GetCurrentThread();
#endif

  /* Start with no timed events. */
  bheap_init(&core->timed, /* root */
             struct react_reg, data.systime.pos, /* structure */
             core, &compare_times); /* comparison */

#if KEEP_3FDSETS
  /* Start with no descriptor events. */
  for (react_iomode_t m = 0; m < react_M_MAX; m++)
    FD_ZERO(&core->fdsum.in_set[m]);
#endif

  return core;


 error:
  if (core) {
#ifdef KEEP_ARRAY
    assert(core->queues.base == NULL);
    free(core->sysbuf.base);
    free(core->sysbuf.link_base);
#endif
#if POLLCALL_RISCOS
    _swix(ReactorHelp_ReleaseWord, _IN(0), (int) core->pollword);
#endif
    free(core);
  }
  return react_COREERROR;
}

void react_closecore(struct react_corestr *core)
{
  /* Cancel all events. */
  for (struct react_reg *r = dllist_first(&core->members);
       r != NULL; r = dllist_next(in_core, r))
    react_cancel(r);

#if KEEP_POLLREC
  free(core->pollrec.base);
#endif

#if POLLCALL_WINDOWS
  /* These should have emptied now, because we cancelled all their
     users. */
  react_cleanupwsausages(core);
  for (unsigned i = 0;
       i < sizeof core->winhandle_hash / sizeof core->winhandle_hash[0]; i++)
    free(&core->winhandle_hash[i].base);
#endif

#if ARRAY_LIMIT
  /* TODO: Can we allocate a single block by exploiting alignment
     calculations? */
  free(core->sysbuf.base);
  free(core->sysbuf.ref_base);
#endif

#if KEEP_3FDSETS
  free(core->fdmap.base);
#endif

#if POLLCALL_RISCOS
  _swix(ReactorHelp_ReleaseWord, _IN(0), (int) core->pollword);
#endif

  /* Delete all queues. */
  for (size_t maj = 0; maj < core->queues.size; maj++)
    free(core->queues.base[maj].base);
  free(core->queues.base);

  /* Detach all events. */
  for (struct react_reg *r = dllist_first(&core->members);
       r != NULL; r = dllist_first(&core->members)) {
    dllist_unlink(&core->members, in_core, r);
    r->core = NULL;
  }

  /* Finally, release the core. */
  free(core);
}

void react_debug(struct react_corestr *core, FILE *fp, unsigned lvl)
{
  core->debug_str = fp;
  core->debug_lvl = lvl;
}
