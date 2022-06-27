// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <swis.h>

#include <riscos/vectors.h>
#include <riscos/swi/OS.h>
#include <riscos/swi/Socket.h>

#include "module.h"

/*
 * How to use this module
 *
 * First, if you can't set up a pollword yourself, you can use
 * ReactorHelp_ClaimWord:
 *
 * R0 <= address of pollword
 *
 * You should arrange to release it before exiting with
 * ReactorHelp_ReleaseWord:
 *
 * R0 => address of pollword
 *
 * Then, before calling Wimp_Poll or Wimp_PollIdle, call
 * ReactorHelp_WatchSockets:
 *
 * R0 => number of sockets
 *
 * R1 => ptr to FD set for reading
 *
 * R2 => ptr to FD set for writing
 *
 * R3 => ptr to FD set for exceptions
 *
 * R4 => pollword address
 *
 * R5 => bit index
 *
 * The first four arguments are those you would normally give to
 * Socket_Select.  This call first goes through all FDs mentioned in
 * the sets (upto R0-1), enables the Internet event on them, and
 * associates the pollword and bit index with them.  It then calls
 * Socket_Select with copies of the first four arguments and a zero
 * timeout.  If any events at this stage are detected, it sets the
 * specified bit in the pollword at once.  If the pollword is passed
 * to the subsequent Wimp call, it should return immediately because
 * the pollword is non-zero.  Otherwise, if a socket event occurs, the
 * Wimp call will return shortly after that.
 *
 * Then you should call ReactorHelp_RedeemSockets with the same first
 * five arguments as the ReactorHelp_WatchSockets call, with the
 * referenced FD sets being unchanged from that call.
 *
 * R0 => number of sockets
 *
 * R1 => ptr to FD set for reading
 *
 * R2 => ptr to FD set for writing
 *
 * R3 => ptr to FD set for exceptions
 *
 * R4 => pollword address
 *
 * R0 <= number of events
 *
 * R1 <= pollword value before resetting
 *
 * This second call turns off the Internet event for the specified
 * sockets, and disasociates them from the pollword.  It then calls
 * Socket_Select on the first four arguments plus a zero timeout, and
 * returns the result in R0.  It also atomically zeroes the pollword,
 * and returns the value just before zeroing in R1.
 */


#define ERROR_BASE 0x0c4380
#define Error_OutOfPollwords (ERROR_BASE + 0u)
#define Error_InvalidPollword (ERROR_BASE + 1u)
#define Error_PollwordsInUse (ERROR_BASE + 2u)


/* These are the pollwords associated with each socket, and which bit
   to set.  We pack 6 5-bit bit positions per 32-bit word. */
#define BITINDEX_BITS (5u)
#define BITINDEX_MASK ((1u << BITINDEX_BITS) - 1u)
#define INDICES_PER_WORD (32u / BITINDEX_BITS)
static unsigned *pollwords[FD_SETSIZE];
static unsigned pollbits[(FD_SETSIZE + INDICES_PER_WORD - 1u) /
                         INDICES_PER_WORD];

/* We keep a large, fixed-size table of potential pollwords to give
   out to users.  Set bits in 'pollword_mask' indicate which entries
   in 'pollword_table' are free. */
#define POLLWORD_LIMIT 256u
static unsigned pollword_table[POLLWORD_LIMIT];
static unsigned pollword_mask[(POLLWORD_LIMIT + 31u) / 32u];
unsigned pollwords_inuse;

_kernel_oserror *module_start(const char *tail, int podule_base, void *pw)
{
  _kernel_oserror *e;

  /* Enable Internet event. */
  e = _swix(XOS_Byte, _INR(0, 1), 14, 19);
  if (e) return e;

  /* Register the event handler. */
  e = _swix(XOS_Claim, _INR(0, 2), EventV, (int) &event_entry, (int) pw);
  if (e) {
    _swix(XOS_Byte, _INR(0, 1), 13, 19);
    return e;
  }

  /* Indicate that all pollwords are free. */
  for (unsigned w = 0; w < sizeof pollword_mask / sizeof pollword_mask[0]; w++)
    pollword_mask[w] = -1;

  return NULL;
}

_kernel_oserror *module_end(int fatal, int podule_base, void *pw)
{
  if (pollwords_inuse > 0) {
    /* Return error indicating that we don't want to quit. */
    static _kernel_oserror re = {
      .errnum = Error_PollwordsInUse,
      .errmess = "Reactor helper pollwords in use",
    };
    return &re;
  }

  /* Disable Internet event. */
  _swix(XOS_Byte, _INR(0, 1), 13, 19);

  /* Deregister the event handler. */
  _swix(XOS_Release, _INR(0, 2), EventV, (int) &event_entry, (int) pw);

  return NULL;
}

/* These are small machine-code routines to modify pollwords
   atomically. */
extern unsigned replace_atomic(unsigned *ptr, unsigned nv);
extern void set_bit_atomic(unsigned *ptr, unsigned bit);

static int gb(fd_mask *val);
static int gb2(unsigned *val);

_kernel_oserror *module_swi(int number, _kernel_swi_regs *r, void *pw)
{
  switch (number) {
  case ReactorHelp_WatchSockets:
  case XReactorHelp_WatchSockets: {
    /* R0 => number of sockets */
    /* R1 => ptr to FD set for reading */
    /* R2 => ptr to FD set for writing */
    /* R3 => ptr to FD set for exceptions */
    /* R4 => pollword address */
    /* R5 => bit index */
    const int nfds = r->r[0];
    const fd_set *sets[3] = {
      [0] = (fd_set *) r->r[1],
      [1] = (fd_set *) r->r[2],
      [2] = (fd_set *) r->r[3],
    };
    unsigned *pollword = (unsigned *) r->r[4];
    unsigned pollbit = r->r[5] & 0x1fu;

    /* How many elements of each fd_set array do we consult?  What bit
       mask do we use in the last case? */
    const unsigned lim = (nfds + NFDBITS - 1u) / NFDBITS;
    const unsigned lastmask =
      (fd_mask) -1 >> (NFDBITS - 1u - (nfds + NFDBITS - 1u) % NFDBITS);

    /* Record the events that the user is interested in.  Check if any
       of the events are currently active, and set the bit in the
       pollword if so. */
    const int on = 1;
    fd_set prepared;
    FD_ZERO(&prepared);
    for (unsigned m = 0; m < 3; m++) {
      if (sets[m] == NULL) continue;

      /* Iterate over all sockets in this mode efficiently. */
      for (unsigned w = 0; w < lim; w++) {
        fd_mask mask;
        if (w + 1u < lim)
          mask = -1;
        else
          mask = lastmask;

        /* Skip sockets the user's not interested in. */
        fd_mask word = sets[m]->fds_bits[w] & mask;
        while (word != 0) {
          /* Identify a bit in this word, and remove it. */
          int sock = gb(&word);

          if (FD_ISSET(sock, &prepared)) continue;

          /* Indicate which pollword and bit to set when it
             happens.*/
          FD_SET(sock, &prepared);
          pollwords[sock] = pollword;
          pollbits[sock / INDICES_PER_WORD] &=
            ~(BITINDEX_MASK << BITINDEX_BITS * (sock % INDICES_PER_WORD));
          pollbits[sock / INDICES_PER_WORD] |=
            pollbit << BITINDEX_BITS * (sock % INDICES_PER_WORD);

          /* Make the socket generate the Internet event. */
          _swix(XSocket_Ioctl, _INR(0, 2), sock, FIOASYNC, &on);
        }
      }
    }

    /* Check now for events.  Some sockets might already have had
       events on them before we told them to generate Internet
       events. */
    fd_set copy[3], *ptr[3];
    for (unsigned m = 0; m < 3; m++) {
      if (sets[m] != NULL) {
        ptr[m] = &copy[m];
        copy[m] = *sets[m];
      } else {
        ptr[m] = NULL;
      }
    }
    struct timeval zero = { 0, 0 };
    int rc;
    _kernel_oserror *e = _swix(XSocket_Select, _INR(0, 4) | _OUT(0), nfds,
                               (int) ptr[0], (int) ptr[1], (int) ptr[2],
                               (int) &zero, &rc);
    if (!e && rc > 0) {
      /* At least one thing has already happened, so set the
         pollword. */
      set_bit_atomic(pollword, pollbit);
    }

    return NULL;
  }

  case ReactorHelp_RedeemSockets:
  case XReactorHelp_RedeemSockets: {
    /* R0 => number of sockets */
    /* R1 => ptr to FD set for reading */
    /* R2 => ptr to FD set for writing */
    /* R3 => ptr to FD set for exceptions */
    /* R4 => pollword address */
    /* R0 <= number of events detected */
    /* R1 <= value obtained from now zero pollword */
    const int nfds = r->r[0];
    fd_set *sets[3] = {
      [0] = (fd_set *) r->r[1],
      [1] = (fd_set *) r->r[2],
      [2] = (fd_set *) r->r[3],
    };
    unsigned *pollword = (unsigned *) r->r[4];

    /* How many elements of each fd_set array do we consult?  What bit
       mask do we use in the last case? */
    const unsigned lim = (nfds + NFDBITS - 1u) / NFDBITS;
    const unsigned lastmask =
      (fd_mask) -1 >> (NFDBITS - 1u - (nfds + NFDBITS - 1u) % NFDBITS);

    /* Scan for occurred events, keep count, and report to the
       user. */
    const int off = 0;
    for (unsigned m = 0; m < 3 ; m++) {
      if (sets[m] == NULL) continue;
      for (unsigned w = 0; w < lim; w++) {
        fd_mask mask;
        if (w + 1u < lim)
          mask = -1;
        else
          mask = lastmask;

        /* Skip sockets the user's not interested in. */
        fd_mask word = sets[m]->fds_bits[w] & mask;
        while (word != 0) {
          /* Identify a bit in this word, and remove it. */
          int sock = gb(&word);

          /* Stop events for this socket from having any effect. */
          _swix(XSocket_Ioctl, _INR(0, 2), sock, FIOASYNC, &off);
          pollwords[sock] = NULL;
        }
      }
    }

    _kernel_oserror *e = _kernel_swi(XSocket_Select, r, r);
    r->r[1] = replace_atomic(pollword, 0);
    return e;
  }

  case ReactorHelp_ClaimWord:
  case XReactorHelp_ClaimWord: {
    /* R0 <= pollword address */

    /* Allocate a free pollword and return it. */
    for (unsigned b = 0;
         b < sizeof pollword_mask / sizeof pollword_mask[0]; b++) {
      if (pollword_mask[b] == 0) continue;

      /* Get the index of the lowest set bit, and clear it. */
      int val = gb2(&pollword_mask[b]);
      pollwords_inuse++;
      r->r[0] = (int) &pollword_table[b * 32 + val];
      return NULL;
    }

    /* We failed to find a free space.  Return an error. */
    static _kernel_oserror re = {
      .errnum = Error_OutOfPollwords,
      .errmess = "No free pollwords",
    };
    return &re;
  }

  case ReactorHelp_ReleaseWord:
  case XReactorHelp_ReleaseWord: {
    /* R0 => pollword address */

    /* Check if the pollword is one of ours. */
    unsigned base = (unsigned) pollword_table;
    if ((unsigned) r->r[0] < base ||
        (unsigned) r->r[0] >= base + sizeof pollword_table) {
      /* Return an error. */
      static _kernel_oserror re = {
        .errnum = Error_InvalidPollword,
        .errmess = "Not a reactor pollword",
      };
      return &re;
    }

    /* Deallocate the pollword. */
    unsigned pos = (unsigned *) r->r[0] - pollword_table;
    pollword_mask[pos / 32] |= 1u << (pos % 32);
    pollwords_inuse--;

    return NULL;
  }

  default:
    return error_BAD_SWI;
  }
}

int event_handler(_kernel_swi_regs *r, void *pw)
{
  /* Eliminate non-socket events. */
  switch (r->r[1]) {
  case 1:
  case 2:
  case 3:
    break;

  default:
    return 1;
  }
  int sock = r->r[2];

  if (pollwords[sock] != NULL) {
    unsigned pollbit = pollbits[sock / INDICES_PER_WORD];
    pollbit >>= BITINDEX_BITS * (sock % INDICES_PER_WORD);
    pollbit &= BITINDEX_MASK;
    set_bit_atomic(pollwords[sock], pollbit);
  }

  /* Pass the event on. */
  return 1;
}

static char assert_32_bits[2 * (NFDBITS == 32) - 1];

static int bc(fd_mask word)
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

static int gb(fd_mask *val)
{
  fd_mask sub = *val - 1;
  int bit = bc(*val ^ sub) - 1;
  *val &= sub;
  return bit;
}

static int gb2(unsigned *val)
{
  unsigned sub = *val - 1;
  int bit = bc(*val ^ sub) - 1;
  *val &= sub;
  return bit;
}
