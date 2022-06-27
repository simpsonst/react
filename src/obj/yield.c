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

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include "common.h"
#include "mytime.h"
#include "fdextract.h"

#if POLLCALL_RISCOS
#include <riscos/wimp/events.h>
#include <riscos/swi/OS.h>
#include <riscos/swi/Wimp.h>
#include "module.h"
#endif

typedef enum {
  JUSTFINE,
  TIMEDOUT,
  WOULDBLOCK,
  PROBLEM,
} rc_type;

#define PRINTPOLL(out, str, code)       \
  if ((str)->events & POLL ## code)     \
    fprintf((out), " POLL%s", #code)

#if POLLCALL_PPOLL || POLLCALL_POLL
static rc_type wait_on_poll(struct react_corestr *core,
                            delay_type *timeout)
{
  if (core->debug_str != NULL) {
    fprintf(core->debug_str,
            "%s:%d pollfds=%zu/%zu/%zu timeout=" DELAY_FMT "\n",
            __FILE__, __LINE__,
            (size_t) core->sysbuf.size,
            (size_t) core->sysbuf.lim,
            (size_t) core->sysbuf.cap,
            DELAY_ARG(timeout));
    for (index_type i = 0; i < core->sysbuf.lim; i++) {
      if (core->sysbuf.base[i].fd < 0) continue;
      fprintf(core->debug_str, "[%4u] %6d:",
              (unsigned) i, core->sysbuf.base[i].fd);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], IN);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], OUT);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], PRI);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], ERR);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], HUP);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], NVAL);
#if _GNU_SOURCE
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], RDHUP);
#endif
#if _XOPEN_SOURCE
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], RDBAND);
      PRINTPOLL(core->debug_str, &core->sysbuf.base[i], WRBAND);
#endif
      putc('\n', core->debug_str);
    }
  }

  if (core->sysbuf.size == 0 && timeout == NULL && core->sig_ev == NULL)
    return WOULDBLOCK;

  int rc;
#if POLLCALL_PPOLL
  rc = ppoll(core->sysbuf.base, core->sysbuf.lim, timeout, &core->sigmask);
#else
  sigset_t origmask;
  pthread_sigmask(SIG_SETMASK, &core->sigmask, &origmask);
  rc = poll(core->sysbuf.base, core->sysbuf.lim, timeout ? *timeout : -1);
  pthread_sigmask(SIG_SETMASK, &origmask, NULL);
#endif
  if (rc < 0) {
    if (errno == EINTR) {
      struct react_reg *p = core->sig_ev;
      if (p) {
        (*p->act)(p);
        return JUSTFINE;
      }
    }
    return PROBLEM;
  }

  /* Scan system buffer entries for detected events. */
  for (index_type i = 0; i < core->sysbuf.lim && rc > 0; i++) {
    /* Skip elements with no new events. */
    struct pollfd *elem = &core->sysbuf.base[i];
    if (elem->fd < 0) continue;
    if (elem->revents == 0) continue;

    rc--;

    /* Notify the relevant handle.  It may use core->sysbuf.idx to
       determine which system buffer entry it is dealing with, if it
       has more than one. */
    core->sysbuf.idx = i;
    struct react_reg *p = core->sysbuf.ref_base[i].user.ev;
    (*p->act)(p);
  }
  assert(rc == 0);

  return JUSTFINE;
}
#endif

#if POLLCALL_RISCOS
static unsigned delay_to_monotonic(struct react_corestr *core,
                                   const struct timeval *now,
                                   const struct timeval *delay)
{
  /* How long is it since the recorded epoch? */
  unsigned long long since =
    now->tv_sec * 1000000ll + now->tv_usec - core->epoch_rt;

  /* How long since the epoch is the next event? */
  unsigned long long event = since +
    delay->tv_sec * 1000000ll + delay->tv_usec;

  /* Round the current time to the most recent monotonic tick. */
  since /= 10000;

  /* Round the event time up to the next monotonic tick. */
  event += 99999;
  event /= 10000;

  /* (event - since) is a number of monotonic ticks we need to
     wait.  Is it too great? */
  if (event - since > 0x7fffffffu)
    return 0x7fffffffu;

  /* No, so compute it. */
  return (core->epoch_mt + event) & 0x7fffffffu;
}
#endif

#if POLLCALL_PSELECT || POLLCALL_SELECT || POLLCALL_RISCOS
static void copy_fd_sets(struct react_corestr *core,
                         fd_set out_set[static react_M_MAX])
{
  bool top_disused = core->fdsum.nfds > 0;
  for (react_iomode_t i = 0; i < react_M_MAX; i++) {
    out_set[i] = core->fdsum.in_set[i];
    if (top_disused && FD_ISSET(core->fdsum.nfds - 1, &out_set[i]))
      top_disused = false;
  }
  if (top_disused)
    core->fdsum.nfds--;
}

static void report_fd_sets(struct react_corestr *core,
                           fd_set out_set[static react_M_MAX],
                           delay_type *timeout)
{
  if (core->debug_str == NULL) return;

  int used_max = core->fdsum.nfds;
  fprintf(core->debug_str, "selecting on:\n");
  for (react_iomode_t i = 0; i < react_M_MAX; i++) {
    for (int fd = 0; fd < used_max; fd++)
      if (FD_ISSET(fd, &out_set[i]))
        putc('1', core->debug_str);
      else
        putc('0', core->debug_str);
    putc('\n', core->debug_str);
  }
  fprintf(core->debug_str, "  timeout: %lu.%0"
#if POLLCALL_PSELECT
    "9"
#else
    "6"
#endif
    "lu (%s)\n",
    (unsigned long) (timeout ? timeout->tv_sec : 0),
    (unsigned long) (timeout ? timeout->
#if POLLCALL_PSELECT
                     tv_nsec
#else
                     tv_usec
#endif
                     : 0),
    timeout ? "okay" : "null");
}

static void hunt_bad_fds(struct react_corestr *core)
{
  /* At least one of the FDs is bad.  Re-copy the sets, because they
     are undefined at this point.  All the ones giving us problems are
     in these sets, and we need a copy because we modify them as we
     scan. */
  fd_set out_set[react_M_MAX];
  for (react_iomode_t i = 0; i < react_M_MAX; i++)
    out_set[i] = core->fdsum.in_set[i];

  /* Scan the descriptors to find all the bad ones, and activate
     their handles, subtracting them from the FD sets. */
  const int used_max = core->fdsum.nfds;
  for (react_iomode_t m = 0; m < react_M_MAX; m++) {
    core->fdsum.mode = m;
    fd_set *curset = &out_set[m];
    for (int fd = -1; react_findfd(curset, used_max, &fd); ) {
      /* Detect whether this FD is bad. */
      if (fcntl(fd, F_GETFD) != -1)
        continue;

      /* Notify the corresponding event.  It will use the current
         values of fdsum.mode and fdsum.fd to know what event it is
         actually being notified about. */
      struct react_reg *r = core->fdmap.base[fd].elem[m];
      assert(r != NULL);
      core->fdsum.fd = fd;
      (*r->act)(r);
    } /* end of per-FD loop */
  } /* end of per-mode loop */
}

static void hunt_fd_events(struct react_corestr *core, fd_set *out_set)
{
  /* Scan the file descriptor sets, and notify the events
     corresponding to FDs that are set. */
  const int used_max = core->fdsum.nfds;
  for (react_iomode_t m = 0; m < react_M_MAX; m++) {
    core->fdsum.mode = m;
    fd_set *curset = &out_set[m];
    for (int fd = -1; react_findfd(curset, used_max, &fd); ) {
      /* Notify the corresponding event.  It will use the current
         values of fdsum.mode and fdsum.fd to know what event it is
         actually being notified about. */
      struct react_reg *r = core->fdmap.base[fd].elem[m];
      assert(r != NULL);
      core->fdsum.fd = fd;
      (*r->act)(r);
    } /* end of per-FD loop */
  } /* end of per-mode loop */
}
#endif

#if POLLCALL_PSELECT || POLLCALL_SELECT
static rc_type wait_on_select(struct react_corestr *core,
                              delay_type *timeout)
{
  if (core->fdsum.count == 0 && timeout == NULL &&
      (core->sig_ev == NULL || core->sig_ev->queued))
    return WOULDBLOCK;

  /* Get a local copy of the interesting file descriptors.
     Simultaneously, decrement the maximum FD, if it's not being
     used. */
  fd_set out_set[react_M_MAX];
  copy_fd_sets(core, out_set);
  report_fd_sets(core, out_set, timeout);

  int used_max = core->fdsum.nfds;
  int rc;
#if POLLCALL_PSELECT
  rc = pselect(used_max,
               &out_set[react_MIN],
               &out_set[react_MOUT],
               &out_set[react_MEXC],
               timeout, &core->sigmask);
#else
  sigset_t origmask;
  pthread_sigmask(SIG_SETMASK, &core->sigmask, &origmask);
  rc = select(used_max,
              &out_set[react_MIN],
              &out_set[react_MOUT],
              &out_set[react_MEXC],
              timeout);
  pthread_sigmask(SIG_SETMASK, &origmask, NULL);
#endif
  switch (rc) {
  case 0:
    return TIMEDOUT;

  case -1:
    if (errno == EINTR) {
      struct react_reg *p = core->sig_ev;
      if (p) {
        (*p->act)(p);
        return JUSTFINE;
      }
    } else if (errno == EBADF) {
      hunt_bad_fds(core);

      /* Allow other events to be picked up. */
      return JUSTFINE;
    }
    return PROBLEM;

  default:
    hunt_fd_events(core, out_set);
    return JUSTFINE;
  }
}
#endif

#if POLLCALL_RISCOS
static int bc(unsigned word)
{
  switch (word) {
  case 0xffffffffu:
    return 32;
  case 0x7fffffffu:
    return 31;
  case 0x3fffffffu:
    return 30;
  case 0x1fffffffu:
    return 29;
  case 0xfffffffu:
    return 28;
  case 0x7ffffffu:
    return 27;
  case 0x3ffffffu:
    return 26;
  case 0x1ffffffu:
    return 25;
  case 0xffffffu:
    return 24;
  case 0x7fffffu:
    return 23;
  case 0x3fffffu:
    return 22;
  case 0x1fffffu:
    return 21;
  case 0xfffffu:
    return 20;
  case 0x7ffffu:
    return 19;
  case 0x3ffffu:
    return 18;
  case 0x1ffffu:
    return 17;
  case 0xffffu:
    return 16;
  case 0x7fffu:
    return 15;
  case 0x3fffu:
    return 14;
  case 0x1fffu:
    return 13;
  case 0xfffu:
    return 12;
  case 0x7ffu:
    return 11;
  case 0x3ffu:
    return 10;
  case 0x1ffu:
    return 9;
  case 0xffu:
    return 8;
  case 0x7fu:
    return 7;
  case 0x3fu:
    return 6;
  case 0x1fu:
    return 5;
  case 0xfu:
    return 4;
  case 0x7u:
    return 3;
  case 0x3u:
    return 2;
  case 0x1u:
    return 1;
  case 0x0u:
  default:
    return -1;
  }
}

static int gb(unsigned *val)
{
  unsigned sub = *val - 1;
  unsigned bit = bc(*val ^ sub) - 1;
  *val &= sub;
  return bit;
}

static rc_type wait_on_riscos(struct react_corestr *core,
                              struct timeval *timeout,
                              const struct timeval *now)
{
  if (core->fdsum.count == 0 && timeout == NULL &&
      (core->sig_ev == NULL || core->sig_ev->queued) &&
      core->queued_wimp_ev == NULL)
    return WOULDBLOCK;

  /* Get a local copy of the interesting file descriptors.
     Simultaneously, decrement the maximum FD, if it's not being
     used. */
  fd_set out_set[react_M_MAX];
  copy_fd_sets(core, out_set);
  report_fd_sets(core, out_set, timeout);

  int used_max = core->fdsum.nfds;

  if (core->wimp_ev_count == 0 || core->queued_wimp_ev != NULL) {
    /* There is no WIMP event, or it is waiting to be processed.  Just
       use select() with zero timeout. */
    assert(timeout != NULL);
    assert(timeout->tv_sec == 0);
    assert(timeout->tv_usec == 0);
    int rc = select(used_max,
                    &out_set[react_MIN],
                    &out_set[react_MOUT],
                    &out_set[react_MEXC],
                    timeout);
    switch (rc) {
    case 0:
      return TIMEDOUT;

    case -1:
      if (errno == EINTR) {
#if 0
        struct react_reg *p = core->sig_ev;
        if (p) {
          (*p->act)(p);
          return JUSTFINE;
        }
#endif
      } else if (errno == EBADF) {
        hunt_bad_fds(core);

        /* Allow other events to be picked up. */
        return JUSTFINE;
      }
      return PROBLEM;

    default:
      hunt_fd_events(core, out_set);
      return JUSTFINE;
    }

#if 0
    _kernel_oserror *ep =
      _swix(Socket_Select, _INR(0,4)|_OUT(0), used_max,
            &out_set[react_MIN],
            &out_set[react_MOUT],
            &out_set[react_MEXC],
            timeout, &rc);
    if (ep) {
      errno = ep->errnum;
      return PROBLEM;
    }
#endif

  } else {
    /* Report all the FDs to the module. */
    _swix(XReactorHelp_WatchSockets, _INR(0, 5),
          used_max, (int) &out_set[react_MIN],
          (int) &out_set[react_MOUT], (int) &out_set[react_MEXC],
          (int) core->pollword, 31);
    /* TODO: Check for kernel error. */

    /* Poll for WIMP events. */
    unsigned mask = core->wimp_mask;
    mask &= ~(1u << PollWord_NonZero);
    _kernel_oserror *ep;
    if (timeout) {
      /* Work out the earliest time when the call should return with a
         Null event.  Our timeout is already computed as a delay from
         'now', but we need it back as an absolute time. */
      unsigned when = delay_to_monotonic(core, now, timeout);

      /* Enable the null event.  Do we need to do this? */
      mask &= ~(1u << Null_Reason_Code);

      /* Call SWI Wimp_PollIdle. */
      ep = _swix(Wimp_PollIdle, _INR(0,3)|_OUT(0),
                 mask, (int) core->wimp_buf, (int) when,
                 (int) core->pollword, &core->ev_code);
    } else {
      /* Disable the null event. */
      mask |= 1u << Null_Reason_Code;

      /* Call SWI Wimp_Poll. */
      ep = _swix(Wimp_Poll, _IN(0)|_IN(1)|_IN(3)|_OUT(0),
                 mask, (int) core->wimp_buf,
                 (int) core->pollword, &core->ev_code);
    }

    /* Cancel all the FDs reported to the module. */
    int rc;
    _swix(XReactorHelp_RedeemSockets, _INR(0, 4) | _OUTR(0, 1),
          used_max, (int) &out_set[react_MIN],
          (int) &out_set[react_MOUT], (int) &out_set[react_MEXC],
          (int) core->pollword, &rc, (int *) &core->pwval);
    /* TODO: Check for kernel error. */

    if (ep == NULL) {
      /* Act on pollword events. */
      while (core->pwval != 0) {
        unsigned bit = gb(&core->pwval);
        struct react_reg *r = core->pollword_event_handler[bit];
        if (r == NULL) continue;
        (*r->act)(r);
      }

      /* Pass on the WIMP event for processing. */
      switch (core->ev_code) {
      case PollWord_NonZero:
        /* We've already handled the pollword. */
        break;

      default:
        assert(core->ev_code >= 0);
        assert((unsigned) core->ev_code < SIZEOFARR(core->wimp_event_handler));
        struct react_reg *evev = core->wimp_event_handler[core->ev_code];
        if (evev) {
          (*evev->act)(evev);
        } else {
          /* TODO: Report an error? */
        }
        break;

      case User_Message:
      case User_Message_Recorded:
      case User_Message_Acknowledge:
        /* Get the message code, and look it up in the bitmap trie. */
        core->msg_code = core->wimp_buf[16] |
          (core->wimp_buf[17] << 8) |
          (core->wimp_buf[18] << 16) |
          (core->wimp_buf[19] << 24);
        struct react_reg *msgev =
          react_msgfind(core, core->msg_code, core->ev_code);
        if (msgev == NULL)
          msgev = core->wimp_event_handler[core->ev_code];
        if (msgev) {
          (*msgev->act)(msgev);
        } else {
          /* TODO: Report an error? */
        }
        break;
      }
    } else {
      /* TODO: Handle error from WIMP. */
    }

    /* Process FDs according to what we redeemed. */
    switch (rc) {
    case 0:
      return TIMEDOUT;

    case -1:
      if (errno == EINTR) {
#if 0
        struct react_reg *p = core->sig_ev;
        if (p) {
          (*p->act)(p);
          return JUSTFINE;
        }
#endif
      } else if (errno == EBADF) {
        hunt_bad_fds(core);

        /* Allow other events to be picked up. */
        return JUSTFINE;
      }
      return PROBLEM;

    default:
      hunt_fd_events(core, out_set);
      return JUSTFINE;
    }
  }

  return PROBLEM;
}
#endif

#if POLLCALL_WINDOWS
static void setdword(DWORD *p, DWORD val)
{
  if (p) *p = val;
}

static rc_type wait_on_windows(struct react_corestr *core,
                               const DWORD *timeout)
{
  DWORD rsp;

  /* We don't look for these events if they are already queued.  */
  BOOL msg_ev_watch = core->msg_ev && !core->msg_ev->queued;
  BOOL apc_ev_watch = core->apc_ev && !core->apc_ev->queued;

  /* Record that no handles have been detected as signalled.  It is
     safe to include the unused entries, as we will eliminate them
     later. */
  for (DWORD i = 0; i < core->sysbuf.lim; i++)
    assert(!core->sysbuf.ref_base[i].user.fire);

  /* Find the first entry in the system buffer that is signalled,
     using the specified timeout.  Then, keep searching the remaining
     parts of the buffer for more, using a zero timeout.  These
     handles are then considered to have been signalled
     simultaneously. */
  DWORD start = 0;
  BOOL first = TRUE;
  do {
    assert(core->sysbuf.lim >= start);
    const DWORD remaining = core->sysbuf.lim - start;

    if (core->debug_str != NULL) {
      fprintf(core->debug_str, "POLL: %llums%s %lu+%lu%s%s%s\n",
              (unsigned long long) (timeout ? *timeout : INFINITE),
              timeout ? "" : "!",
              (unsigned long) start, (unsigned long) remaining,
              first ? " FIRST" : "",
              msg_ev_watch ? " MSG" : "",
              apc_ev_watch ? " APC" : "");
      for (unsigned long i = start; i < core->sysbuf.lim; i++) {
        fprintf(core->debug_str, "  %2lu:", i);
        fprintf(core->debug_str, " (%5lu, %5lu)",
                (unsigned long) core->sysbuf.ref_base[i].prev,
                (unsigned long) core->sysbuf.ref_base[i].next);
        if (core->sysbuf.ref_base[i].next == ARRAY_LIMIT) {
          assert(core->sysbuf.ref_base[i].prev == ARRAY_LIMIT);
          fprintf(core->debug_str, " in use (%p, ev=%p, act=%p)",
                  (void *) &core->sysbuf.ref_base[i].user,
                  (void *) core->sysbuf.ref_base[i].user.ev,
                  (void *) core->sysbuf.ref_base[i].user.ev->act);
        } else {
          fprintf(core->debug_str, " free");
          assert(core->sysbuf.base[i] == core->unused_handle);
        }
        if (i == core->sysbuf.free)
          fprintf(core->debug_str, " (1st)");
        putc('\n', core->debug_str);
      }
    }

    if (msg_ev_watch) {
      /* We always wait for the first handle, never all of them, but
         we might also wait for APCs. */
      DWORD modFlags = 0;
      if (apc_ev_watch)
        modFlags |= MWMO_ALERTABLE; /* Wait for APCs. */

      rsp = MsgWaitForMultipleObjectsEx(remaining,
                                        core->sysbuf.base + start,
                                        timeout ? *timeout : INFINITE,
                                        core->msg_ev->data.winmsg.wakeMask,
                                        modFlags);
    } else if (remaining > 0) {
      rsp = WaitForMultipleObjectsEx(remaining,
                                     core->sysbuf.base + start,
                                     FALSE, timeout ? *timeout : INFINITE,
                                     apc_ev_watch);
    } else if (timeout) {
      /* WaitForMultipleObjects can't cope with a zero-length
         array. */
      rsp = SleepEx(*timeout, apc_ev_watch);
      if (rsp == 0) rsp = WAIT_TIMEOUT;
    } else {
      return WOULDBLOCK;
    }

    if (rsp == WAIT_FAILED) {
      /* This should never happen, but if it does, let's at least
         print a diagnostic to stderr. */
      TCHAR szBuf[80]; 
      LPVOID lpMsgBuf;
      DWORD errnum = GetLastError();
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    errnum,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0, NULL);
      wsprintf(szBuf, "Wait failed with error %d: %s", errnum, lpMsgBuf); 
      fprintf(stderr, "react: %s\n", szBuf);

      /* TODO: Perhaps allow events we've already found to be
         processed, but still return PROBLEM?  This would involve
         declaring a DWORD before the loop with JUSTFINE as default,
         setting it to PROBLEM here, and breaking out of the loop. */
      return PROBLEM;
    }

    if (rsp == WAIT_TIMEOUT) {
      /* A timeout is only significant on the first iteration. */
      if (first)
        return TIMEDOUT;

      /* Otherwise, we've just tested the last batch of HANDLEs, so
         stop here, and process what we've found. */
      break;
    }
    /* Subsequent timeouts simply will mean that we found nothing
       else. */
    first = FALSE;

    DWORD done = 0;
    if (rsp == WAIT_IO_COMPLETION) {
      /* An APC has been detected.  Stop watching for further APCs. */
      assert(apc_ev_watch);
      apc_ev_watch = FALSE;
    } else if (rsp == WAIT_OBJECT_0 + remaining) {
      /* A Windows message has been detected.  Stop watching for
         further Windows messages. */
      assert(msg_ev_watch);
      msg_ev_watch = FALSE;
    } else if (rsp >= WAIT_ABANDONED_0 &&
               (done = rsp - WAIT_ABANDONED_0) < remaining) {
      /* One of the handles has been detected as abandoned.  Record
         that state (if the user wants it), and act on the event at
         the specified position in the buffer. */
      assert(core->sysbuf.base[done + start] != NULL);
      ref_type *p = &core->sysbuf.ref_base[done + start].user;
      assert(!p->fire);
      setdword(p->ev->data.winhandle.status, WAIT_ABANDONED);
      p->fire = TRUE;
      done++;
    } else if ((rsp == WAIT_OBJECT_0 || rsp > WAIT_OBJECT_0) &&
               (done = rsp - WAIT_OBJECT_0) < remaining) {
      /* One of the handles has been detected as signalled.  Record
         that state (if the user wants it), and act on the event at
         the specified position in the buffer. */
      assert(core->sysbuf.base[done + start] != NULL);
      ref_type *p = &core->sysbuf.ref_base[done + start].user;
      assert(!p->fire);
      setdword(p->ev->data.winhandle.status, WAIT_OBJECT_0);
      p->fire = TRUE;
      done++;
    } else {
      /* There should be no other return codes that have not already
         been dealt with. */
      UNREACHABLE();
    }
    static const DWORD zeroTimeout = 0;
    timeout = &zeroTimeout;
    start += done;
  } while (start < core->sysbuf.lim);

  /* Activate all the events we detected. */

  /* We have detected a Windows message/APC if we had one to look for,
     but are no longer looking for it now.  This will also be the case
     if the reactor handle had already been queued before entry, but
     that's okay, because the configured action is idempotent ("ensure
     we are queued"). */
  if (!msg_ev_watch && core->msg_ev)
    (*core->msg_ev->act)(core->msg_ev);
  if (!apc_ev_watch && core->apc_ev)
    (*core->apc_ev->act)(core->apc_ev);

  /* Scan the HANDLEs for the 'fire' flag.  The flags of only non-null
     HANDLEs should ever have been set. */
  for (DWORD i = 0; i < core->sysbuf.lim; i++) {
    ref_type *p = &core->sysbuf.ref_base[i].user;
    if (p->fire) {
      p->fire = FALSE;
      assert(p->ev);
      (*p->ev->act)(p->ev);
    }
  }

  return JUSTFINE;
}
#endif

static inline rc_type wait_on_system(struct react_corestr *core,
                                     delay_type *timeout,
                                     moment_type *now)
{
#if POLLCALL_PSELECT || POLLCALL_SELECT
  return wait_on_select(core, timeout);
#elif POLLCALL_PPOLL || POLLCALL_POLL
  return wait_on_poll(core, timeout);
#elif POLLCALL_RISCOS
  return wait_on_riscos(core, timeout, now);
#elif POLLCALL_WINDOWS
  return wait_on_windows(core, timeout);
#else
#error "No implementation"
  return PROBLEM;
#endif
}

static int detect_events(struct react_corestr *core)
{
  struct react_reg *first = bheap_peek(&core->timed);

  delay_type delay, *timeout;
  moment_type now;

  /* Work out the maximum time we will have to wait. */
  if (core->queues.top < core->queues.size || !dllist_isempty(&core->idlers)) {
    /* We have jobs to do straight away.  Idle events always fire, and
       we might have queued but unprocessed events left from the
       previous call. */
    react_systime_zero(timeout = &delay);
#if 0
    fprintf(stderr, "react: must act now\n");
#endif
  } else if (first) {
    /* Work out the extact delay. */
    timeout = &delay;

    /* What is the current time? */
    if (react_systime_now(&now) < 0)
      return -1;

    /* How long until the first event? */
    if (react_systime_diff(timeout, &first->data.systime.when, &now))
      /* It's already overdue. */
      react_systime_zero(timeout);
#if 0
    fprintf(stderr, "react: must act within %s by %s\n",
            react_systime_fmtdelay(timeout),
            react_systime_fmt(&first->data.systime.when));
#endif
  } else {
    /* If there are no pending events, no idlers, and no timed events,
       the timeout is indefinite. */
    timeout = NULL;
#if 0
    fprintf(stderr, "react: no time limit\n");
#endif
  }

  switch (wait_on_system(core, timeout, &now)) {
  case WOULDBLOCK:
    errno = EAGAIN;
    return -1;

  case PROBLEM:
    return -1;

  default:
    break;
  }

  /* Fall through to handle timed events and idlers. */
  return 0;
}

int react_yield(struct react_corestr *core)
{
  /* Detect and trigger/queue platform-specific events. */
  if (detect_events(core) < 0) {
    assert(errno != 0);
    return -1;
  }

  /* Notify any timed events that should have gone off by now. */
  moment_type now;
  if (react_systime_now(&now) < 0)
    return -1;
  for (struct react_reg *r = bheap_peek(&core->timed);
       r != NULL &&
         react_systime_cmp(&now, &r->data.systime.when) >= 0;
       r = bheap_peek(&core->timed))
    (*r->act)(r);

  /* Notify all idlers. */
  for (struct react_reg *r = dllist_first(&core->idlers);
       r != NULL; r = dllist_first(&core->idlers))
    (*r->act)(r);

  /* Process all events in the non-empty queue with the highest
     priority. */
#if 0
  fprintf(stderr, "Processing events...\n");
#endif
  bool found = false;
  for (unsigned p = core->queues.top; p < core->queues.size;
       core->queues.top = ++p) {
#if 0
    fprintf(stderr, "  Searching priority %u\n", p);
#endif
    struct queue_set *sq = &core->queues.base[p];
    if (found) {
      /* We've already processed events in a higher-priority queue.
         All we are doing now is checking for any events that we must
         leave unprocessed until the next call to react_yield().  We
         check all subqueues of this queue for non-emptiness. */
      for (unsigned sp = 0; sp < sq->size; sp++) {
        if (!dllist_isempty(&sq->base[sp])) {
          /* There is at least one pending event.  Start here next
             time, unless ahigher-priority event is triggered before
             then. */
          core->queues.top = p;
#if 0
          fprintf(stderr, "  Found future handle in %u/%u\n", p, sp);
#endif
          return 0;
        }
      }
    } else {
      /* We have only found empty major queues so far, so, if we find
         any here, we process them, and record that we found some. */
      for (unsigned sp = 0; sp < sq->size; sp++) {
#if 0
        fprintf(stderr, "  Searching subpriority %u/%u\n", p, sp);
#endif
        for (struct react_reg *r = dllist_first(&sq->base[sp]);
             r != NULL; r = dllist_first(&sq->base[sp])) {
#if 0
          fprintf(stderr, "  Got %p\n", (void *) r);
#endif
          assert(r->queued);
          dllist_unlink(&sq->base[sp], in_queue, r);
          r->queued = false;
          react_defuse(r);
          (*r->proc)(r->proc_data);
          found = true;
        }
      }
    }
  }

  /* We've processed all triggered events.  We don't need to check the
     queues again until more are triggered. */
  core->queues.top = core->queues.size;
  return 0;
}


/* TODO: Get rid of what's below.  We only keep it around to remind
   ourselves of what types have been chosen for each platform-specific
   call. */

#if 0
#ifdef react_YIELD_RISCOS
int react_yieldevent(struct react_corestr *core,
                     unsigned mask, void *buf,
                     const volatile unsigned *pollword,
                     unsigned pollmask, int *evcode)
{
  return -1;
}
#endif

#if defined react_YIELD_RISCOS
#include <kernel.h>
typedef const struct react_riscospd {
  unsigned mask;
  void *buf;
  const volatile unsigned *pollword;
  unsigned pollmask;
  int *evcodep;
} *react_polldata;
#else
typedef const void *react_polldata;
#endif
#endif
