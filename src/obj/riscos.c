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
#include <string.h>
#include <stdbool.h>

#include "common.h"

#include "react/riscos.h"

#if POLLCALL_RISCOS

#include <riscos/wimp/events.h>

#define CN(V)                                                         \
  (V) == 0 ? 0u : (V) == 1 ? 1u : (V) == 2 ? 1u : (V) == 3 ? 2u :     \
    (V) == 4 ? 1u : (V) == 5 ? 2u : (V) == 6 ? 2u : (V) == 7 ? 3u :   \
    (V) == 8 ? 1u : (V) == 9 ? 2u : (V) == 10 ? 2u : (V) == 11 ? 3u : \
    (V) == 12 ? 2u : (V) == 13 ? 3u : (V) == 14 ? 3u : 4u
#define NP(A, B) ((A) | ((B) << 4u))
#define B2(N) NP(CN(N), CN((N) + 1))
#define B8(N) B2(N), B2((N) + 2), B2((N) + 4), B2((N) + 6)
#define B32(N) B8(N), B8((N) + 8), B8((N) + 16), B8((N) + 24)
#define B256(N) B32(N), B32((N) + 32), B32((N) + 64), B32((N) + 96)

static const unsigned char PACKED_BIT_COUNT[128] = { B256(0) };

static inline unsigned count_bits8(unsigned byte)
{
  return (PACKED_BIT_COUNT[byte / 2] >> ((byte % 2) & 4u)) & 0xfu;
}

static inline unsigned count_bits32(unsigned word)
{
  return count_bits8(word & 0xffu) +
    count_bits8((word >> 8u) & 0xffu) +
    count_bits8((word >> 16u) & 0xffu) +
    count_bits8((word >> 24u) & 0xffu);
}

static unsigned get_index(const unsigned bm[static 8], unsigned key)
{
  assert(key < 256);

  /* How many words do we consider? */
  unsigned wlim = (key + 31) / 32;
  assert(wlim <= 8);
  if (wlim == 0) return 0;

  /* What mask do we use for the last word? */
  unsigned mask = ~(~0u << ((key + 31) % 32 + 1));

  /* Count all the set bits before the key. */
  unsigned count = 0;
  for (unsigned w = 0; w < wlim - 1; w++)
    count += count_bits32(bm[w]);
  count += count_bits32(bm[wlim - 1] & mask);

  return count;
}

static void **walk_trie(void **vrootp, unsigned byte)
{
  assert(vrootp != NULL);
  struct bmtn *root = *vrootp;
  if (root == NULL) return vrootp;

  /* Is the byte recorded at this level? */
  if (root->bitmap[byte / 32] & (1u << (byte % 32))) {
    unsigned idx = get_index(root->bitmap, byte);
    return &root->child[idx];
  }

  return NULL;
}

static int ensure_cap(struct bmtn *node, unsigned min, unsigned scale)
{
  if (node->cap >= min) return 0;
  unsigned ncap = node->cap;
  while (ncap < min)
    ncap += ncap / 2 + 16;
  if (ncap > 256) ncap = 256;
  void *nbase = realloc(node->child, ncap * sizeof *node->child * scale);
  if (nbase == NULL) return -1;
  node->child = nbase;
  node->cap = ncap;
  return 0;
}

static void **insert_entry(struct bmtn *node, unsigned pos, unsigned scale)
{
  assert(pos < 256);
  if (ensure_cap(node, pos + 1, scale) < 0) return NULL;
  unsigned idx = get_index(node->bitmap, pos);
  assert(idx <= node->len);
  memmove(&node->child[(idx + 1u) * scale],
          &node->child[idx * scale], (node->len - idx) & scale);
  for (unsigned s = 0; s < scale; s++)
    node->child[idx * scale + s] = NULL;
  node->bitmap[pos / 32] |= 1u << (pos % 32);
  return &node->child[idx * scale];
}

#if 0
static void **get_entry(struct bmtn *node, unsigned pos, unsigned scale)
{
  assert(pos < 256);
  if (node->bitmap[pos / 32] & (1u << (pos % 32))) {
    unsigned idx = get_index(node->bitmap, pos);
    assert(idx <= node->len);
    return &node->child[idx * scale];
  } else {
    return NULL;
  }
}
#endif

/* Return -1 for insufficient memory, -2 if something else is in the
   way, 0 if the message type should already be enabled, and 1 if it
   should now be enabled. */
int react_msginsert(struct react_corestr *core,
                    unsigned key, struct react_reg *r,
                    unsigned evtypes)
{
  if ((evtypes &
       (User_Message | User_Message_Recorded | User_Message_Acknowledge)) == 0)
    return 0;
  if (r == NULL) return 0;

  void *pos = &core->msg_root;
  for (unsigned i = 0; i < 4; i++) {
    unsigned byte = key & 0xffu;
    key >>= 8u;
    void **newpos = walk_trie(&pos, byte);
    if (newpos == NULL) {
      if (i == 3) {
        void **ptr = insert_entry(pos, byte, 3);
        if (insert_entry(pos, byte, 3) == NULL) return -1;
        if (evtypes & (1u << User_Message)) {
          ptr[0] = r;
          core->wimp_ev_count++;
        }
        if (evtypes & (1u << User_Message_Recorded)) {
          ptr[1] = r;
          core->wimp_ev_count++;
        }
        if (evtypes & (1u << User_Message_Acknowledge)) {
          ptr[2] = r;
          core->wimp_ev_count++;
        }
        return 1;
      } else {
        void **ptr = insert_entry(pos, byte, 1);
        if (ptr == NULL) return -1;
        struct bmtn *newnode = malloc(sizeof *newnode);
        if (newnode == NULL) return -1;
        ptr[0] = newnode;
        pos = newnode;
      }
    } else if (i == 3) {
      /* Work out what we need to add. */
      unsigned doit[3] = {
        evtypes & (1u << User_Message),
        evtypes & (1u << User_Message_Recorded),
        evtypes & (1u << User_Message_Acknowledge),
      };

      /* Eliminate things we don't have to do, while checking for
         things we can't do. */
      for (unsigned s = 0; s < 3; s++)
        if (doit[s]) {
          if (newpos[s] == r)
            doit[s] = false;
          else if (newpos[s] != NULL)
            return -2;
        }

      /* Stop if we've eliminated all things to do. */
      if (!doit[0] && !doit[1] && !doit[2]) return 0;

      /* Record whether the message type is currently disabled. */
      bool allzero = newpos[0] == NULL && newpos[1] == NULL && newpos[2] == NULL;

      /* Set the new values. */
      for (unsigned s = 0; s < 3; s++)
        if (doit[s]) {
          assert(newpos[s] == NULL);
          newpos[s] = r;
          core->wimp_ev_count++;
        }

      /* We definitely set something, so the message type should be
         enabled if it was not already. */
      return allzero;
    } else {
      pos = *newpos;
    }
  }
  UNREACHABLE();
  return -3;
}

struct react_reg *react_msgfind(struct react_corestr *core, unsigned key,
                                unsigned evtype)
{
  void *pos = &core->msg_root;
  for (unsigned i = 0; i < 4; i++) {
    unsigned byte = key & 0xffu;
    key >>= 8u;
    void **newpos = walk_trie(&pos, byte);
    if (newpos == NULL) return NULL;
    if (i == 3) {
      switch (evtype) {
      case User_Message:
        return newpos[0];
      case User_Message_Recorded:
        return newpos[1];
      case User_Message_Acknowledge:
        return newpos[2];
      default:
        return NULL;
      }
    }
    pos = *newpos;
  }
  UNREACHABLE();
  return NULL;
 }

/* Return non-zero if the message type should be disabled. */
int react_msgremove(struct react_corestr *core, unsigned key,
                    unsigned evtypes)
{
  if ((evtypes &
       (User_Message | User_Message_Recorded | User_Message_Acknowledge)) == 0)
    return 0;

  void *pos = &core->msg_root;
  for (unsigned i = 0; i < 4; i++) {
    unsigned byte = key & 0xffu;
    key >>= 8u;
    void **newpos = walk_trie(&pos, byte);
    if (newpos == NULL) return 0;
    if (i == 3) {
      /* We know we do at least one of these. */
      unsigned doit[3] = {
        evtypes & (1u << User_Message),
        evtypes & (1u << User_Message_Recorded),
        evtypes & (1u << User_Message_Acknowledge),
      };

      /* Remove the specified entries. */
      unsigned oldcount = core->wimp_ev_count;
      for (unsigned s = 0; s < 3; s++)
        if (doit[s] && newpos[s] != NULL) {
          newpos[s] = NULL;
          core->wimp_ev_count--;
        }

      /* The message type should be disabled if we removed an entry,
         and now all entries are null. */
      return core->wimp_ev_count != oldcount && newpos[0] == NULL &&
        newpos[1] == NULL && newpos[2] == NULL;
    }
    pos = *newpos;
  }
  UNREACHABLE();
  return 0;
}

#endif
