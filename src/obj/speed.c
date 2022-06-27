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
#include <stdio.h>
#include <errno.h>

#include "react/types.h"
#include "react/core.h"
#include "react/event.h"
#include "react/idle.h"
#include "react/file.h"

typedef struct {
  react_t handle;
  int stars;
} idle_tester;

void idle_event(void *ctv)
{
  idle_tester *ct = ctv;
  if (ct->stars-- > 0) {
#if 0
    printf("*");
    fflush(stdout);
#endif
    if (react_prime_idle(ct->handle) < 0) {
      perror("react_prime_idle");
    }
  }
}

int idle_init(idle_tester *ct, react_core_t b, react_prio_t prio)
{
  ct->stars = 0;
  ct->handle = react_open(b);
  if (ct->handle == react_ERROR)
    return -1;

  react_setprios(ct->handle, prio, 0);
  react_direct(ct->handle, &idle_event, ct);
  return 0;
}

void idle_start(idle_tester *ct, int stars)
{
  ct->stars += stars;
  if (react_isprimed(ct->handle)) return;
  if (react_prime_idle(ct->handle) < 0) {
    perror("react_prime_idle");
  }
}



int main(void)
{
  react_core_t core;
  idle_tester idle;

  core = react_opencore(5);

  idle_init(&idle, core, 4);
  idle_start(&idle, 100000000/* 0 */);

  unsigned cycles = 0;
  for ( ; ; cycles++) {
    errno = 0;
    if (react_yield(core) < 0) {
      if (errno != EINTR) {
        perror("yield");
        break;
      }
    }
  }

  printf("No more events left...  cycled %u times\n", cycles);

  react_closecore(core);

  return EXIT_SUCCESS;
}
