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
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#include "react/event.h"
#include "react/core.h"
#include "react/idle.h"
#include "react/time.h"
#include "react/file.h"

typedef struct {
  react_t handle;
  int stars;
} idle_tester;

typedef struct {
  unsigned limit;
  idle_tester *idler;
  unsigned idle_inc;
  struct timeval when, increment;
  react_t handle;
  char message[100];
} timed_tester;

#if react_ALLOW_STDIN
typedef struct {
  react_t handle;
  idle_tester *idler;
} stdin_tester;
void stdin_event(void *);
#endif

void idle_event(void *);
void timed_event(void *);



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

void idle_event(void *ctv)
{
  idle_tester *ct = ctv;
  if (ct->stars-- > 0) {
    printf("*");
    fflush(stdout);
    if (react_prime_idle(ct->handle) < 0) {
      perror("react_prime_idle");
    }
  }
}

int timed_init(timed_tester *ct, react_core_t b, react_prio_t prio,
               const char *m, long s, long us,
               unsigned lim,
               idle_tester *idler, unsigned inc)
{
  ct->handle = react_open(b);
  if (ct->handle == react_ERROR)
    return -1;

  react_setprios(ct->handle, prio, 0);

  ct->limit = lim;
  ct->idler = idler;
  ct->idle_inc = inc;
  ct->increment.tv_sec = s;
  ct->increment.tv_usec = us;

  react_direct(ct->handle, &timed_event, ct);
  strncpy(ct->message, m, sizeof ct->message - 1);
  ct->message[sizeof ct->message - 1] = '\0';

  return 0;
}

void timed_set(timed_tester *ct)
{
#if defined __WIN32__ || defined __riscos || defined __riscos__
  ct->when.tv_sec += ct->increment.tv_sec;
  ct->when.tv_usec += ct->increment.tv_usec;
  if (ct->when.tv_usec > 1000000) {
    ct->when.tv_usec -= 1000000;
    ct->when.tv_sec++;
  }
#else
  timeradd(&ct->when, &ct->increment, &ct->when);
#endif
  if (react_prime_timeval(ct->handle, &ct->when) < 0)
    perror("react_prime_timeval");
}

void timed_start(timed_tester *ct, const struct timeval *now)
{
  ct->when = *now;
  timed_set(ct);
}

void timed_event(void *ctv)
{
  timed_tester *ct = ctv;
  printf("%s%s\n", ct->message, ct->limit > 0 ? "" : " STOPPING");
  if (ct->idler) idle_start(ct->idler, ct->idle_inc);
  if (ct->limit-- > 0)
    timed_set(ct);
}



#if react_ALLOW_STDIN
void stdin_event(void *ct)
{
  stdin_tester *inr = ct;
  char line[400];

  if (fgets(line, sizeof line, stdin) != NULL) {
    printf("Read: %s", line);
    if (inr->idler) idle_start(inr->idler, strlen(line));
    if (react_prime_stdin(inr->handle) < 0) {
      perror("react_prime(stdin)");
    }
  } else {
    printf("Input terminated\n");
    react_close(inr->handle), inr->handle = react_ERROR;
  }
}

int stdin_init(stdin_tester *inr, react_core_t b, react_prio_t prio,
              idle_tester *idler)
{  
  inr->handle = react_open(b);
  if (inr->handle == react_ERROR)
    return -1;

  react_setprios(inr->handle, prio, 0);
  react_direct(inr->handle, &stdin_event, inr);
  inr->idler = idler;

  if (react_prime_stdin(inr->handle) < 0) {
    perror("react_prime_fd(in)");
  }

  return 0;
}
#endif

int main(void)
{
  react_core_t core;
  struct timeval now;
  timed_tester c1, c2, c3;
#if react_ALLOW_STDIN
  stdin_tester rdr;
#endif
  idle_tester idle;

  core = react_opencore(5);
  idle_init(&idle, core, 4);
  timed_init(&c1, core, 1, "\n1Hz", 1, 0, 10, &idle, 30);
  timed_init(&c2, core, 2, "\n2Hz", 0, 500000, 7, &idle, 31);
  timed_init(&c3, core, 0, "\n0.5Hz", 2, 0, 12, &idle, 18);

  gettimeofday(&now, NULL);

#if react_ALLOW_STDIN
  stdin_init(&rdr, core, 3, &idle);
#endif

  timed_start(&c1, &now);
  timed_start(&c2, &now);
  timed_start(&c3, &now);
  idle_start(&idle, 10);

  printf("Beginning...\n");
  for ( ; ; ) {
    errno = 0;
    if (react_yield(core) < 0) {
      if (errno == EAGAIN
#ifndef __WIN32__
          || errno == EWOULDBLOCK
#endif
          )
        break;
      if (errno == EINTR)
        continue;
      perror("react_yield");
      break;
    }
  }

  printf("No more events left...\n");

  react_closecore(core);

  return EXIT_SUCCESS;
}
