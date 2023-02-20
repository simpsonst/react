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

#ifndef react_event_HDRINCLUDED
#define react_event_HDRINCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "features.h"
#include "types.h"
#include "condtype.h"

  /* Create a new event handle within a specific core. */
  struct react_corestr;
  struct react_reg;
  struct react_reg *react_open(struct react_corestr *);

  /* Destroy an event handle. */
  void react_close(struct react_reg *);

  /* Specify the behaviour of an event handle.  When a triggered
     handle is processed, (*f)(ctxt) will be invoked. */
  void react_direct(struct react_reg *, react_proc_t *f, void *ctxt);

  /* Prime an event handle to be triggered when a condition occurs.
     The behaviour and priority must be specified by this point.
     Return negative on error, with errno == react_ENOMEM if out of
     memory, react_EEVENTINUSE if another handle of the core is
     already waiting for that condition, react_ECOREBUSY for other
     resource contention, and react_EINVAL for other errors. */
  react_deprecated(int react_prime(struct react_reg *,
                                   react_cond_t, const void *));

  /* Test if the event handle is primed but not yet triggered. */
  int react_isprimed(struct react_reg *);

  /* Test if the event handle has been queued but not yet processed,
     and is no longer primed. */
  int react_istriggered(struct react_reg *);

  /* Test if the event handle has been queued but not yet
     processed. */
  int react_isqueued(struct react_reg *);

  /* Test if the event handle is primed or queued. */
  int react_isactive(struct react_reg *);

  /* Deactivate the event handle, so it is no longer primed or queued.
     Beware of doing this with conditions that cannot be
     redetected. */
  void react_cancel(struct react_reg *);

  /* Manually trigger the event handle, placing in the queue
     appropriate to its set priority, and preventing it from detecting
     further events. */
  void react_trigger(struct react_reg *);

  /* Get or set the major priority of an event handle.  Setting is
     forbidden while the binding is primed.

     Deprecated: use react_set/getprios instead. */
  react_deprecated(react_prio_t react_getprio(struct react_reg *));
  react_deprecated(int react_setprio(struct react_reg *, react_prio_t));

  /* Set the major and minor priorities of an event handle.  This
     determines what queue an event handle is added to when triggered.
     Return negative on error, with errno == react_ENOMEM if unable to
     allocate space for sufficient queues, EDOM if either of the
     priorities is outside its implementation-defined limit, or
     react_EBADSTATE if the event is already in a queue.

     Major priorities can prevent lower-priority events from being
     processed.  On each call to react_yield(), all event handles for
     detected events are triggered, but only those of the highest
     non-empty major priority are processed.  A subsequent call to
     react_yield() is required to process handles of the next
     non-empty priority, but this might be prevented if newer,
     higher-priority events occur.

     Minor priorities only define a process ordering within a major
     priority. */
  int react_setprios(struct react_reg *, react_prio_t, react_subprio_t);

  /* Get the current major and minor priorities of an event handle.
     They are written to *majp and *minp respectively. */
  void react_getprios(struct react_reg *,
                      react_prio_t *majp, react_subprio_t *minp);

#ifdef __cplusplus
}
#endif

#endif
