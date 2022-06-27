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

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/time.h>

#include "react/core.hh"
#include "react/event.hh"
#include "react/handlerhelp.hh"
#include "react/idle.hh"
#include "react/file.hh"
#include "react/time.hh"

using namespace std;

class IdleTester {
  react::InnerHandler<IdleTester> idleDelegate;
  react::Event idleEvent;

  int stars;

  void idleReady() {
    if (stars > 0) {
      cout << "*" << flush;
      stars--;

      react::IdleCondition cond;
      idleEvent->prime(cond);
    }
  }

public:
  IdleTester(react::Core core, react::prio_t prio)
    : idleDelegate(*this, &IdleTester::idleReady),
      idleEvent(core->open(idleDelegate, prio)),
      stars(0) { }

  void go(unsigned inc) {
    stars += inc;
    react::IdleCondition cond;
    idleEvent->prime(cond);
  }
};

class TimedTester {
  react::InnerHandler<TimedTester> occasionDelegate;
  react::Event occasionEvent;

  IdleTester *idler;
  unsigned idle_inc;

  string message;
  unsigned limit;
  timeval when, increment;

  void primeOccasion() {
    react::TimeValCondition cond(when);
    occasionEvent->prime(cond);

    when.tv_sec += increment.tv_sec;
    when.tv_usec += increment.tv_usec;
    when.tv_sec += when.tv_usec / 1000000;
    when.tv_usec %= 1000000;
  }

  void occasionReady() {
    cout << endl << message;
    if (limit == 0)
      cout << " TERMINATING";
    cout << endl << flush;
    if (limit-- > 0)
      primeOccasion();

    if (idler)
      idler->go(idle_inc);
  }

public:
  TimedTester(react::Core core, react::prio_t prio,
              const string &message, int secs, int usecs,
              unsigned limit,
              IdleTester &idler, unsigned idle_inc)
    : occasionDelegate(*this, &TimedTester::occasionReady),
      occasionEvent(core->open(occasionDelegate, prio)),
      idler(&idler), idle_inc(idle_inc),
      message(message), limit(limit) {
    increment.tv_sec = secs;
    increment.tv_usec = usecs;
  }

  void go() {
    gettimeofday(&when, 0);
    primeOccasion();
  }
};

#if react_ALLOW_STDFILE
class StreamTester {
  react::InnerHandler<StreamTester> inputDelegate;
  react::Event inputEvent;

  FILE *fp;

  void primeInput() {
    react::StdFileInCondition cond(fp);
    inputEvent->prime(cond);
  }

  void inputReady() {
    char line[256];
    if (fgets(line, sizeof line, fp)) {
      cout << "You typed: " << line << flush;
      primeInput();
    } else {
      cout << "Input terminated" << endl;
    }
  }

public:
  StreamTester(react::Core core, react::prio_t prio, FILE *fp)
    : inputDelegate(*this, &StreamTester::inputReady),
      inputEvent(core->open(inputDelegate, prio)),
      fp(fp) { }

  void go() {
    primeInput();
  }
};
#endif


int main(int argc, const char *const *argv)
{
  react::Core core = react::newCore();

  IdleTester idle_test(core, 2);
  TimedTester timed_test1(core, 1, "1Hz", 1, 0, 16, idle_test, 17);
  TimedTester timed_test2(core, 1, "2Hz", 0, 500000, 23, idle_test, 8);
  TimedTester timed_test3(core, 1, "0.5Hz", 2, 0, 7, idle_test, 40);
#if react_ALLOW_STDFILE
  StreamTester stream_test(core, 0, stdin);
  stream_test.go();
#endif
  timed_test1.go();
  timed_test2.go();
  timed_test3.go();

  while (core->yield())
    ;

  return EXIT_SUCCESS;
}
