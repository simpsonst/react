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

#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "react/event.h"
#include "react/windows.h"
#include "react/socket.h"
#include "array.h"

#if POLLCALL_WINDOWS
#if 0
#define ENTRY(X) { .mask = FD_##X, .pos = FD_##X##_BIT, .name = #X }
static const struct {
  long mask;
  unsigned pos;
  const char *name;
} event_info[FD_MAX_EVENTS] = {
  ENTRY(READ),
  ENTRY(WRITE),
  ENTRY(OOB),
  ENTRY(ACCEPT),
  ENTRY(CONNECT),
  ENTRY(CLOSE),
  ENTRY(QOS),
  ENTRY(GROUP_QOS),
  ENTRY(ROUTING_INTERFACE_CHANGE),
  ENTRY(ADDRESS_LIST_CHANGE),
};

static int print_events(FILE *out, long mode)
{
  int rc = 0;
  const char *sep = "";
  for (size_t i = 0; i < FD_MAX_EVENTS; i++)
    if (mode & event_info[i].mask) {
      rc += fprintf(out, "%s%s", sep, event_info[i].name);
      sep = "|";
    }
  return rc;
}
#endif

/* ----- Hash tables of Windows HANDLEs ----- */

/* Get a hash of a Windows handle.  It is aliased as an array of
   bytes, from which the hash is computed. */
static unsigned hash_handle(HANDLE hdl)
{
  union {
    unsigned char byte[sizeof(HANDLE)];
    HANDLE hdl;
  } key = { { 0 } };
  key.hdl = hdl;
  unsigned h = 1;
  for (size_t i = 0; i < sizeof key.byte; i++) {
    h *= 31;
    h += key.byte[i];
  }
  return h;
}

/* To keep track of Windows handles in use, we keep a hash table of
   buckets.  Having identified which bucket to use, this function
   ensures that there is room for one more handle, returning 0 on
   success, and -1 on error, setting errno. */
static int ensure_cap(struct handle_bucket *bkt)
{
  if (bkt->size < bkt->alloc)
    return 0;

  size_t ncap = bkt->alloc + 2 * 2;

  void *nbase = realloc(bkt->base, sizeof *bkt->base * ncap);
  if (!nbase) {
    errno = react_ENOMEM;
    return -1;
  }

  bkt->base = nbase;
  bkt->alloc = ncap;
  return 0;
}

/* Check whether a handle is registered with a reactor.  The hash is
   computed to select the appropriate bucket, then we scan it for the
   handle.  If found, the handle is already in use, and we return -1
   with errno == react_EEVENTINUSE.  Otherwise, the handle is added to
   the bucket, and we return 0. */
static int check_and_add_unique_handle(struct react_corestr *core, HANDLE hdl)
{
  unsigned h = hash_handle(hdl) %
    (sizeof core->winhandle_hash / sizeof core->winhandle_hash[0]);
  struct handle_bucket *bkt = &core->winhandle_hash[h];
  for (size_t i = 0; i < bkt->size; i++) {
    if (bkt->base[i] == hdl) {
      errno = react_EEVENTINUSE;
      return -1;
    }
  }
  if (ensure_cap(bkt) < 0) return -1;
  bkt->base[bkt->size++] = hdl;
  return 0;
}

/* Deregister a handle from a reactor.  The hash is computed to select
   the appropriate bucket, then we scan it for the handle.  If found,
   the final handle in the bucket overwrites it, and we shorten the
   bucket by one. */
static void remove_handle(struct react_corestr *core, HANDLE hdl)
{
  unsigned h = hash_handle(hdl) %
    (sizeof core->winhandle_hash / sizeof core->winhandle_hash[0]);
  struct handle_bucket *bkt = &core->winhandle_hash[h];
  for (size_t i = 0; i < bkt->size; i++) {
    if (bkt->base[i] == hdl) {
      bkt->base[--bkt->size] = bkt->base[i];
      return;
    }
  }
}






/* ----- Priming on Windows HANDLE ----- */


static void trim_array(struct react_corestr *core)
{
  if (core->sysbuf.lim > 0 &&
      core->sysbuf.base[core->sysbuf.lim - 1] == core->unused_handle)
    core->sysbuf.lim--;
}

/* Release resources used to prime an event handle on a Windows
   handle. */
static void defuse_handle(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  /* Mark the Windows handle as unused, so an event handle can be
     primed again on it. */
  remove_handle(core, r->data.winhandle.hdl);

  index_type pos = r->data.winhandle.tabpos;
  if (pos != (index_type) -1) {
    /* Remove the element from the system buffer. */
    core->sysbuf.base[pos] = core->unused_handle;
    react_arrfree(core, pos);
    trim_array(core);
  }
}

static void on_handle(struct react_reg *r)
{
  struct react_corestr *core = r->core;

  /* Remove the HANDLE and its related data from the system buffer, so
     we don't watch for it again. */
  index_type pos = r->data.winhandle.tabpos;
  assert(pos != (index_type) -1);
  core->sysbuf.base[pos] = core->unused_handle;
  react_arrfree(core, pos);
  trim_array(core);
  r->data.winhandle.tabpos = -1;

  /* Make sure we're queued. */
  react_queue(r);
}

static void init_array(struct react_corestr *core,
                       elem_type *elem, ref_type *ref)
{
  *elem = core->unused_handle;
  ref->fire = FALSE;
  ref->ev = NULL;
}

int react_prime_winhandle(struct react_reg *r, HANDLE hdl, DWORD *st)
{
  struct react_corestr *core = r->core;

  react_cancel(r);

  /* Ensure that there is space in the system buffer for at least
     one more HANDLE. */
  if (react_arrensure(core, 1, &init_array) < 0)
    return -1;

  /* Detect whether the handle is already in use, and mark it as in
     use if not. */
  if (check_and_add_unique_handle(core, hdl) < 0)
    return -1;

  /* Allocate an entry in the system buffer.  Store the Windows
     handle there in the new space, and indicate that it points to
     this event handle. */
  size_t pos = react_arralloc(core);
  assert(pos != (size_t) -1);
  assert(core->sysbuf.base[pos] == core->unused_handle);
  core->sysbuf.base[pos] = hdl;
  core->sysbuf.ref_base[pos].user.ev = r;
  assert(!core->sysbuf.ref_base[pos].user.fire);

  /* Record the event condition. */
  r->data.winhandle.tabpos = pos;
  r->data.winhandle.hdl = hdl;
  r->data.winhandle.status = st;

  r->defuse = &defuse_handle;
  r->act = &on_handle;

  return 0;
}



/* ----- Shared priming on Windows HANDLE ----- */


/* Each of these structures maintains the WSA event for a socket, and
   the set of event handles using it. */
struct wsausage {
  /* Every usage belongs to a list in a hash table.  Here, we remember
     which list and which other members are in that list. */
  dllist_elem(struct wsausage) others;
  wsa_list *list;

  /* Each structure itself is a user of the event reactor, and holds a
     reference to an event handle, and primes it on the WSAEVENT,
     which is compatible with HANDLE. */
  struct react_reg *href;
  action_proc_t *act;
  WSAEVENT wsae;
  DWORD status;

  /* This is the socket to be monitored, and a set of WSA network
     events that it is currently set to report on. */
  SOCKET sock;
  long mode;

  /* This is the set of WSA network events that are considered busy.
     It is only updated by users coming and going, and it is the union
     of their data.winsock.mode event sets. */
  long busyMode;

  /* These are the users of the socket, with mutually non-intersecting
     modes.  They use the data.winsock member.  When the WSAEVENT is
     signalled, we iterate over these users to find ones whose WSA
     event sets intersect with the signalled event set.  The
     corresponding bits of those users' sets are cleared, and if they
     become zero, the user must be defused, and so detached from the
     list.

     This structure must not be deleted while there are users, or when
     the inuse flag is set.

     TODO: Work out whether the inuse flag is really needed. */
  react_evlist users;
  unsigned inuse : 1;
};


/* If a WSA usage is no longer in use, release its resources. */
static void cleanup_wsausage(struct wsausage *ws)
{
  assert(ws);

  /* Only do this if not in use any more. */
  if (ws->inuse || !dllist_isempty(&ws->users)) {
    /* It's likely that we've been called because some modes have been
       removed, so we should indicate that we want to observe fewer
       now.  There */
    assert(ws->mode != 0);
    int rc = WSAEventSelect(ws->sock, ws->wsae, ws->mode);
    assert(rc == 0);
    return;
  }

  /* We should not be looking for anything by now. */
  assert(ws->mode == 0);

#if 0
  /* We can't really afford to have this anymore, unless we deal with
     lNetworkWorks!=0 properly.  Indeed, if events can come in even
     while we're outside of the WaitOn... call, we can't afford to
     delete the WSAEVENT even if we've just tested it. */
  WSANETWORKEVENTS evs;
  int rc = WSAEnumNetworkEvents(ws->sock, ws->wsae, &evs);
  assert(rc == 0);

  printf("WSAEVENT.%03d-=", (int) ws->sock);
  print_events(stdout, evs.lNetworkEvents);
  printf("\n");
  assert(evs.lNetworkEvents == 0);
#endif

#if 0
  printf("WSAEVENT.%03d-\n", ws->sock);
#endif

  react_close(ws->href);
  WSACloseEvent(ws->wsae);
  dllist_unlink(ws->list, others, ws);
  free(ws);
}

void react_cleanupwsausages(struct react_corestr *core)
{
  for (unsigned i = 0;
       i < sizeof core->wsausage / sizeof core->wsausage[0]; i++) {
    for (struct wsausage *ws = dllist_first(&core->wsausage[i]);
         ws != NULL; ws = dllist_first(&core->wsausage[i])) {
      assert(ws->inuse == 0);
      assert(dllist_isempty(&ws->users));
      cleanup_wsausage(ws);
    }
  }
}

static int prime_wsausage(struct wsausage *ws);

/* This is the internal event handler for a WSA usage. */
static void on_winsock(struct react_reg *r)
{
  struct wsausage *ws = r->proc_data;

#if 0
  printf("WSAEVENT.%03d? ", (int) ws->sock);
  print_events(stdout, ws->mode);
  printf("\n");
#endif

  /* Determine what events to report. */
  WSANETWORKEVENTS evs;
  if (ws->status == WAIT_OBJECT_0) {
    /* Find out which events occurred on the socket.  Presumably, this
       unsignals the handle ws->wsae...? */
    int rc = WSAEnumNetworkEvents(ws->sock, ws->wsae, &evs);
    assert(rc == 0);
    /* TODO: If rc != 0, consult evs.iErrorCode to get code for each
       event. */
#if 0
    printf("WSAEVENT.%03d= ", (int) ws->sock);
    print_events(stdout, evs.lNetworkEvents);
    printf("\n");
#endif
  } else {
    /* Pretend that all events occurred.  That will tell all users
       that something useful has happened, so they might consult the
       status.  Even if not, they will attempt to use the handle
       (directly or indirectly), and get failure. */
    evs.lNetworkEvents = ws->mode;
  }

  /* We should only receive events which we have asked for. */
  assert((evs.lNetworkEvents & ~ws->mode) == 0);

  /* Queue all the relevant events, but ensure that we can't be
     deleted. */
  ws->inuse = 1;
  for (struct react_reg *ro = dllist_first(&ws->users);
       ro != NULL; ro = dllist_next(data.winsock.others, ro)) {
    /* We're only interested in users whose event bits match the ones
       detected. */
    long matched = ro->data.winsock.mode & evs.lNetworkEvents;
    if (matched == 0) continue;

    /* Pass on the status. */
    if (ro->data.winsock.status)
      *ro->data.winsock.status = ws->status;

    /* Make sure the user knows which events have occurred. */
    if (ro->data.winsock.result)
      *ro->data.winsock.result |= matched;

    /* Ensure that the event handle is queued. */
    (*ro->act)(ro);
  }
  ws->inuse = 0;

  /* Account for the events that have occurred.  If they're all used
     up, we are no longer needed. */
  ws->mode &= ~evs.lNetworkEvents;
  if (ws->mode != 0) {
    /* Stop waiting for the events that have already occurred by
       repriming the WSAEVENT. */
    int rc = prime_wsausage(ws);
    assert(rc == 0);
  }
}

/* Prime a WSA usage by priming its internal event handle on the WSA
   event with the appropriate modes enabled. */
static int prime_wsausage(struct wsausage *ws)
{
  assert(ws != NULL);
  assert(ws->mode != 0);

  /* Make sure the WSAEVENT knows what we're waiting for. */
  if (WSAEventSelect(ws->sock, ws->wsae, ws->mode) != 0) {
    errno = react_EINVAL;
    return -1;
  }

#if 0
  printf("WSAEVENT.%03d! ", (int) ws->sock);
  print_events(stdout, ws->mode);
  printf("\n");
#endif

  /* If it's primed, it's a waste of time re-priming. */
  if (ws->href->defuse != 0 &&
      ws->href->data.winhandle.tabpos != (index_type) -1)
    return 0;

  /* Now we can safely prime it, and tell it to invoke us at once. */
  int rc = react_prime_winhandle(ws->href, ws->wsae, &ws->status);
  react_swapact(ws->href, &on_winsock, &ws->act);

  return rc;
}

/* Create a new WSA usage record for a socket.  We need: the core to
   make a new internal event handle to watch the created Windows
   HANDLE, a pointer to the list that the record will belong to in the
   hash table, and the socket itself. */
static struct wsausage *make_wsausage(struct react_corestr *core,
                                      wsa_list *list, SOCKET sock)
{
  /* Allocate the record. */
  struct wsausage *ws = malloc(sizeof *ws);
  if (!ws) {
    errno = react_ENOMEM;
    goto failure;
  }

  /* Make sure everything that ws points to has a fail-safe value. */
  ws->list = NULL;
  ws->sock = sock;
  ws->mode = 0;
  ws->wsae = WSA_INVALID_EVENT;
  ws->href = react_ERROR;
  ws->inuse = 0;
  dllist_init(&ws->users);

  /* Create a brand new WSA event handle, which we'll later associate
     the socket with and the event types we're interested in. */
  ws->wsae = WSACreateEvent();
  if (ws->wsae == WSA_INVALID_EVENT)
    goto failure;

  /* Create the reactor event handle. */
  ws->href = react_open(core);
  if (ws->href == react_ERROR)
    goto failure;

  react_direct(ws->href, 0, ws);

  /* Link this usage into the specified bucket. */
  ws->list = list;
  dllist_append(ws->list, others, ws);

#if 0
  printf("WSAEVENT.%03d+\n", (int) ws->sock);
#endif
  return ws;

 failure:
  assert(ws);
  if (ws->wsae != WSA_INVALID_EVENT)
    WSACloseEvent(ws->wsae);
  if (ws->href != react_ERROR)
    react_close(ws->href);
  free(ws);
  return NULL;
}

/* Ensure that a WSA usage record exists for a given socket.  The
   record is looked up by socket in the hash table. */
static struct wsausage *ensure_wsausage(struct react_corestr *core,
                                        SOCKET sock)
{
  /* Determine the bucket. */
  int hi = sock % (sizeof core->wsausage / sizeof core->wsausage[0]);
  struct wsausage *ws;

  /* Search for an existing record (matching on socket) in the
     bucket. */
  for (ws = dllist_first(&core->wsausage[hi]);
       ws && ws->sock != sock; ws = dllist_next(others, ws))
    ;
  if (ws != NULL)
    return ws;

  /* No existing record was found.  Create a new record, telling it
     which bucket it should add itself to. */
  return make_wsausage(core, &core->wsausage[hi], sock);
}


/* Defuse a reactor event handle by detaching it from the shared WSA
   usage record, and offering it a chance to delete itself. */
static void defuse_socket(struct react_reg *r)
{
  struct wsausage *ws = r->data.winsock.usage;

  assert(ws != NULL);
  dllist_unlink(&ws->users, data.winsock.others, r);
  ws->mode &= ~r->data.winsock.mode;
  ws->busyMode &= ~r->data.winsock.mode;
  cleanup_wsausage(ws);
}

int react_prime_winsock(struct react_reg *r, SOCKET sock,
                        long mode, long *got, DWORD *st)
{
  /* Cancel the previous priming, while preserving the WSA usage.
     This prevents it from being de-allocated only to be asked for
     again a moment later. */
  struct wsausage *ws = NULL;
  if (sock != INVALID_SOCKET && r->defuse == &defuse_socket) {
    ws = r->data.winsock.usage;
    if (ws->sock != sock)
      ws = NULL;
  }
  if (ws) ws->inuse++;
  react_cancel(r);
  if (ws) ws->inuse--;

  /* The user must specify a real socket and at least one event it's
     interested in. */
  if (sock == INVALID_SOCKET || mode == 0) {
    errno = react_EINVAL;
    return -1;
  }

  /* Which socket usage record do we want?  Create one if
     necessary. */
  struct react_corestr *core = r->core;
  if (ws == NULL)
    ws = ensure_wsausage(core, sock);
  if (!ws)
    return -1;
  assert(ws->sock == sock);

  if (ws->busyMode & mode) {
    /* The new events intersect with existing events, so we have to
       reject this request. */
    assert(!dllist_isempty(&ws->users));
    errno = react_EEVENTINUSE;
    return -1;
  }

  /* Update the set of shared events we're looking for. */
  ws->mode |= mode; /* The ones we have yet to receive */
  ws->busyMode |= mode; /* The ones that no-one else can prime on */

  /* Link us in to the shared usage record. */
  r->data.winsock.usage = ws;
  dllist_append(&ws->users, data.winsock.others, r);

  /* Record the other data. */
  r->data.winsock.status = st;
  r->data.winsock.mode = mode;
  r->data.winsock.result = got;

  /* Specify how to deallocate this reactor event and act on it. */
  r->defuse = &defuse_socket;
  r->act = &react_queue;

  /* Re-prime the underlying WSA usage if not already triggered. */
  if (prime_wsausage(ws) != 0) {
    /* We'll have to cancel the bits we just added. */
    react_cancel(r);
    return -1;
  }

  return 0;
}

#if react_ALLOW_WINMSG
static void defuse_winmsg(struct react_reg *r)
{
  struct react_corestr *core = r->core;
  assert(core->msg_ev == r);
  core->msg_ev = NULL;
}

int react_prime_winmsg(struct react_reg *r, DWORD wakeMask)
{
  react_cancel(r);
  struct react_corestr *core = r->core;
  if (core->msg_ev) {
    errno = react_EEVENTINUSE;
    return -1;
  }
  core->msg_ev = r;
  r->data.winmsg.wakeMask = wakeMask;
  r->act = &react_queue; /* On this event, we only queue, but remain
                            'primed' so no other handle can prime
                            until we're done. */
  r->defuse = &defuse_winmsg;
  return 0;
}
#endif

#if react_ALLOW_WINAPC
static void defuse_winapc(struct react_reg *r)
{
  struct react_corestr *core = r->core;
  assert(core->apc_ev == r);
  core->apc_ev = NULL;
}

int react_prime_winapc(struct react_reg *r)
{
  react_cancel(r);
  struct react_corestr *core = r->core;
  if (core->apc_ev) {
    errno = react_EEVENTINUSE;
    return -1;
  }
  core->apc_ev = r;
  r->act = &react_queue; /* On this event, we only queue, but remain
                            'primed' so no other handle can prime
                            until we're done. */
  r->defuse = &defuse_winapc;
  return 0;
}
#endif

#endif /* Windows */
