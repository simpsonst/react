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

#ifdef __cplusplus
#error "Do not compile as C++"
#endif

#ifndef common_HDRINCLUDED
#define common_HDRINCLUDED

#include <stdio.h>

#include <ddslib/dllist.h>
#include <ddslib/bheap.h>

#include "features.h"
#include "react/types.h"

#define FDSEARCH_LIST 0
#define FDSEARCH_HASH 1
#define FDSEARCH_ARRAY 2


/* What sort of strategy do we take to store event details
   efficiently? */



typedef dllist_hdr(struct react_reg) react_evlist;

#if POLLCALL_WINDOWS
struct wsausage;
typedef dllist_hdr(struct wsausage) wsa_list;
struct handle_bucket {
  size_t size, alloc;
  HANDLE *base;
};
#endif


struct queue_set {
  size_t size, alloc;
  react_evlist *base;
};

#define SIZEOFARR(A) (sizeof (A) / sizeof (A)[0])

#if ARRAY_LIMIT
struct arrref {
  index_type next, prev;
  ref_type user;
};
#endif

#if KEEP_3FDSETS
typedef struct {
  struct react_reg *elem[react_M_MAX];
} fdmap_elem_type;
#endif

typedef void action_proc_t(struct react_reg *);
typedef void defuse_proc_t(struct react_reg *);


/* This is what a reactor event handle points to. */
struct react_reg {
  /* Each event belongs to a core. */
  struct react_corestr *core;
  dllist_elem(struct react_reg) in_core;

  /* These describe the action to be performed when processing the
     handle in a queue, i.e. (*proc)(proc_data). */
  react_proc_t *proc;
  void *proc_data;
  /* This emulates proactor behaviour.  It is executed as soon as the
     event is detected. */
  action_proc_t *act;

  /* If not null, this is invoked to deallocate the resources in the
     data member. */
  defuse_proc_t *defuse;

  /* When the event is triggered, it belongs to a queue. */
  react_prio_t prio;
  react_subprio_t subprio;
  dllist_elem(struct react_reg) in_queue;
  unsigned queued : 1;

  /* Many proactor-like events depend on a reactor-like event.  This
     entry lazily holds the latter. */
  struct react_reg *subev;
  struct {
    void *mem;
    size_t sz;
  } dyn;

  /* These describe the monitored event. */
  union {
    /* For idle events, we just belong to a list in the core. */
    struct {
      dllist_elem(struct react_reg) others;
    } idle;

    /* For timed events, the handle belongs to a binary heap ordered
       by increasing time. */
    struct {
      bheap_elem pos;
      moment_type when;
    } systime;

#if POLLCALL_WINDOWS
    struct {
      DWORD wakeMask;
    } winmsg;

    struct {
      index_type tabpos;
      HANDLE hdl;
      DWORD *status;
    } winhandle;

    struct {
      struct wsausage *usage;
      dllist_elem(struct react_reg) others;
      long mode, *result;
      DWORD *status;
    } winsock;
#endif

#if POLLCALL_POLL || POLLCALL_PPOLL
    struct {
      struct pollfd *base;
      nfds_t size;
      index_type tabpos; /* Points to any one of the entries in the
                            system table */

      action_proc_t *act;
      union {
        struct {
          /* These are used only to piggyback priming of fd_sets as
             struct pollfds. */
          fd_set *out[react_M_MAX];
        } fds;
        struct {
          /* Only used for an emulated single entry */
          short *got;
          struct pollfd singleton;
        } poll;
      } proact;
    } polls;
#endif

#if POLLCALL_RISCOS
    struct {
      unsigned mask;
      void *ctxt;
      react_wimpev_proc_t *func;
    } wimp_event;

    struct {
      unsigned mask, msgtype;
      void *ctxt;
      react_wimpmsg_proc_t *func;
    } wimp_msg;

    struct {
      unsigned mask;
      unsigned *got;
    } pollword;
#endif

#if KEEP_3FDSETS
    /* All these systems support traditional FD sets. */

    /* The user provides upto 3 FD sets to monitor. */
    struct {
      int lim;
      fd_set in[react_M_MAX]; // copies of inputs
      fd_set *out[react_M_MAX]; // outputs
    } fds;

    /* The user provides one FD and a mode (in, out, exception). */
    struct {
      int fd;
      react_iomode_t mode;
    } fd;
#endif
  } data;

  struct {
    int *en;
    action_proc_t *act;
    union {
#if react_ALLOW_FD
      struct {
        int fd;
        const struct iovec *v0;
        int nv;
        ssize_t *rc;
      } readwritev;
      struct {
        int fd;
        void *buf;
        size_t len;
        ssize_t *rc;
      } read;
      struct {
        int fd;
        const void *buf;
        size_t len;
        ssize_t *rc;
      } write;
      struct {
        int fd_in, fd_out;
#if react_ALLOW_POLLS
        struct pollfd polls[2];
#else
        fd_set ins, outs;
#endif
        loff_t *off_in, *off_out;
        size_t max;
        unsigned flags;
        ssize_t *rc;
      } splice;
#endif

#if react_ALLOW_SOCK
      struct {
        react_sock_t fd;
        void *buf;
        react_buflen_t len;
        int flags;
        react_ssize_t *rc;
        struct sockaddr *addr;
        react_socklen_t *addrlen;
      } recv;
      struct {
        react_sock_t fd;
        struct msghdr *msg;
        int flags;
        react_ssize_t *rc;
      } recvmsg;
      struct {
        react_sock_t fd;
        const void *buf;
        react_buflen_t len;
        int flags;
        react_ssize_t *rc;
        const struct sockaddr *addr;
        react_socklen_t addrlen;
      } send;
      struct {
        react_sock_t fd;
        const struct msghdr *msg;
        int flags;
        react_ssize_t *rc;
      } sendmsg;
      struct {
        react_sock_t fd;
        react_sock_t *rc;
        struct sockaddr *addr;
        react_socklen_t *addrlen;
        int flags;
      } accept;
      struct {
        react_sock_t fd;
        const struct sockaddr *addr;
        react_socklen_t addrlen;
        int *rc;
      } connect;
#endif
    } sub;
  } proact;
};

#if POLLCALL_RISCOS
struct bmtn {
  unsigned bitmap[8];
  unsigned short cap, len;
  void **child;
};
#endif

struct react_corestr {
  FILE *debug_str;
  unsigned debug_lvl;

  /* These are queues of triggered events. */
  struct {
    /* The number of queues in use */
    size_t size;

    /* The number of queues with space allocated */
    size_t alloc;

    /* Pointer to the first queue set (itself an array of queues) */
    struct queue_set *base;

    /* The index of the highest-priority non-empty queue */
    size_t top;
  } queues;

  /* This is a list of all events.  It's only really used to discard
     handles when closing down the core. */
  react_evlist members;

  /* These events are always triggered. */
  react_evlist idlers;

  /* These are timed events in a binary heap ordered by increasing
     time. */
  bheap timed;

#if POLLCALL_WINDOWS
  /* This is a table indexed by a hash of a Windows socket number.
     Each entry records a WSAEVENT that will be associated with the
     socket, and used in the Windows polling call. */
  wsa_list wsausage[19];

  /* We also keep a hash table of Windows HANDLEs that are in use, so
     we can tell the user that someone else is also watching the same
     HANDLE. */
  struct handle_bucket winhandle_hash[19];

  /* We permit a single event handle to wait on messages, and another
     one on APCs. */
  struct react_reg *msg_ev, *apc_ev;

  /* We use this pseudohandle to mark elements of the system buffer
     that are not in use.  It does not need to be closed when the core
     is closed. */
  HANDLE unused_handle;
#endif

#if react_ALLOW_INTR
  struct react_reg *sig_ev;
#endif

#if ARRAY_LIMIT > 0
  /* Keep a dynamic array of elements compatible with the poll call.
     For Windows, this is HANDLE.  For poll() and ppoll(), it is
     struct pollfd.  It is not used in other cases. */
  struct {
    /* This is the allocated size of the array.  It only increases. */
    index_type cap;

    /* This is the number of used elements of the array.  It increases
       and decreases as elements are allocated and released,
       respectively.  It is primarily used to determine whether the
       system buffer is completely full ('size == cap') or completely
       empty ('size == 0').  A completely empty buffer may require a
       different system call.  Allocating an entry when the buffer is
       full warrants a reallocation of the whole buffer. */
    index_type size;

    /* This is never more than 'cap', and all elements from this index
       to the end are definitely free.  Some earlier elements may also
       be free.  If an element is allocated at or above 'lim', 'lim'
       is set beyond it.  If an element is released, element 'lim-1'
       is checked to see if free, and then 'lim' is decremented if
       so.

       The range [0, lim) thus forms an approximate range of used
       elements that sloppily may include unused ones.  The system
       calls can cope with unused elements as they are properly
       marked, but this might allow us to reduce the total number that
       we have to pass to the system call. */
    index_type lim;

    /* This is the index of any free element.  The 'next' and 'prev'
       members of the elements of 'ref_base' form a closed
       doubly-linked list.  If 'size == cap', this member is
       invalid, as there are no free elements. */
    index_type free;

    /* This is an array parallel to the system buffer.  It relates the
       elements of the system buffer to other data in the core and its
       event handles, and keeps track of which ones are free. */
    struct arrref *ref_base;

    /* This is the actual system buffer to be passed to the poll call.
       On Windows, it is an array of 'HANDLE's.  When 'poll()' or
       'ppoll()' are in use, it is an array of 'struct pollfd's. */
    elem_type *base;

#if POLLCALL_POLL || POLLCALL_PPOLL
    /* We let the handle being activated know which of its entries in
       the system buffer is being worked on. */
    index_type idx;
#endif
  } sysbuf;
#endif

#if KEEP_POLLREC
  /* Keep a dynamic array, indexed by FD, of events monitored.  Each
     short is the union of events currently watched on an FD.  Unused
     sockets are kept at zero.  The array grows as required, but
     doesn't have to shrink. */
  struct {
    unsigned size;
    short *base;
  } pollrec;
#endif

#if KEEP_3FDSETS
  /* We are using select() or pselect().  Both require three fd_set
     objects, and we also keep track of the number of FDs, and the
     maximum in use, for more efficient calls to (p)select(). */

  struct fdsum {
    /* No FD >= nfds is being monitored by this core.  It is given as
       the first argument of select() or pselect().  When a new FD >=
       nfds is registered, nfds is set to the new FD plus one.  When
       copying fd_sets ready for passing to (p)select(), we check
       whether 'nfds-1' is in use in any mode.  If not, we decrement
       nfds, so we gradually reduce the number that we observe.  */
    int nfds;

    /* This is the total number of bits set in the fdsets, so we can
       quickly tell if there are any there. */
    size_t count;

#if POLLCALL_RISCOS
    /* Out and exceptional events can't properly be watched for.  We
       need to know if there is at least one. */
    size_t special;
#endif

    /* These sets keep track of what we're monitoring.  They are only
       altered by events priming and defusing. */
    fd_set in_set[react_M_MAX];

    /* These are the current mode and FD being acted upon. */
    int fd;
    react_iomode_t mode;
  } fdsum;

  /* We keep a dynamic array of upto FD_SETSIZE pointers to the event
     handle associated with a file descriptor and a mode.  This helps
     when we're using select() or pselect() as we can directly find
     the event handle from the file descriptor.  We also use it to
     determine that an FD event is in use. */
  struct {
    int size;
    fdmap_elem_type *base;
  } fdmap;
#endif

#if POLLCALL_RISCOS
  /* This records the sole permitted queued WIMP event, or is null if
     no WIMP event is queued.  No WIMP events are tested until this
     one has been processed. */
  struct react_reg *queued_wimp_ev;

  /* We keep a count of events in wimp_event_handler and msg_root. */
  unsigned wimp_ev_count;

  /* At most one handle can be primed on each event type.  We also
     don't use 0 (Null_Reason_Code) because it's already handled
     portably, or 14, 15, 16, which are undefined.  The message events
     17, 18 and 19 are only registered here if they are primed on
     unhandled messages. */
  struct react_reg *wimp_event_handler[20];

  /* We keep a bitmask of primed WIMP event types. */
  unsigned wimp_mask;

  /* At most one handle can be primed on each bit in the pollword.  We
     also forbid 31, because the reactor uses it itself.  When these
     events are acted upon, we don't put them in queued_wimp_ev, so
     they could be delayed by other WIMP events with higher
     priorities. */
  struct react_reg *pollword_event_handler[31];

  /* We keep a bitmap trie of event handles indexed by message
     type. */
  struct bmtn msg_root;

  /* We use a single buffer for receiving all WIMP events, and pass it
     to the event handler. */
  unsigned char wimp_buf[256];

  /* These are to be used by the action of some event handles. */
  int ev_code;
  unsigned msg_code;
  unsigned pwval;

  /* We keep a pollword in RMA. */
  unsigned *pollword;

  /* We need to establish the difference between the monotonic time
     and the real time.  These are initialized when the core is
     created. */
  unsigned long long epoch_rt;
  unsigned epoch_mt;
#endif

#if react_HAVE_SIGSET
  /* This is the set of signals to be blocked while polling, default
     none. */
  sigset_t sigmask;
#endif
};


/* Call and clear the defuse function of the handle, causing it to
   release all remaining resources with the reactor. */
void react_defuse(struct react_reg *);

/* Ensure that the handle is properly queued.  Do nothing if it is. */
void react_queue(struct react_reg *);

/* Ensure that the handle is not queued.  Do nothing if it is not. */
void react_dequeue(struct react_reg *);

/* Replace the native action with another one.  The old one is
   recorded in **fp, and is replaced with *f.  Conventionally, *f
   should call **fp first, and then do its own thing. */
void react_swapact(struct react_reg *r, action_proc_t *f, action_proc_t **fp);

#if POLLCALL_WINDOWS
/* Make sure that all WSA usage records are properly discarded.  Some
   might remain if lazily discarded. */
void react_cleanupwsausages(struct react_corestr *core);
#endif

/* Open a handle to be subservient to an existing handle.  It will
   automatically be closed when the existing handle is closed.  Its
   default action will be to do nothing, as its purpose will not be to
   be queued, but to trigger some other action. */
struct react_reg *react_ensuresub(struct react_reg *r);

/* Ensure that enough memory exists for additional uses. */
void *react_ensuremem(struct react_reg *, size_t);

/* This is available internally on some systems even when not
   generally available to users. */
int react_prime_fd(struct react_reg *, int, react_iomode_t);

#if POLLCALL_RISCOS
int react_msgremove(struct react_corestr *core, unsigned key,
                    unsigned evtypes);
struct react_reg *react_msgfind(struct react_corestr *core, unsigned key,
                                unsigned evtype);
int react_msginsert(struct react_corestr *core,
                    unsigned key, struct react_reg *r,
                    unsigned evtypes);
#endif

#define UNREACHABLE() (fprintf(stderr, "reached %s:%d in %s\n", \
                               __FILE__, __LINE__, __func__), abort())

#endif /* header inclusion protection */
