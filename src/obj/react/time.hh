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
 *  Contact Steven Simpson ss at comp dot lancs dot ac dot uk
 */

#ifndef react_time_CXXHDRINC
#define react_time_CXXHDRINC

#include "time.h"
#include "condhelp.hh"

#if __cplusplus >= 201103L
#if react_ALLOW_TIMEVAL
struct timeval;
namespace react {
  struct TimeValCondition : public HelpedCondition<const timeval *> {
    TimeValCondition(const timeval *t)
      : HelpedCondition(&react_prime_timeval, t) { }
    TimeValCondition(const timeval &t)
      : TimeValCondition(&t) { }
  };
}
#endif

#if react_ALLOW_TIMESPEC
struct timespec;
namespace react {
  struct TimeSpecCondition : public HelpedCondition<const timespec *> {
    TimeSpecCondition(const timespec *t)
      : HelpedCondition(&react_prime_timespec, t) { }
    TimeSpecCondition(const timespec &t)
      : TimeSpecCondition(&t) { }
  };
}
#endif

#if react_ALLOW_WINFILETIME
#ifdef FillMemory
namespace react {
  struct WinFileTimeCondition : public HelpedCondition<const FILETIME *> {
    WinFileTimeCondition(const FILETIME *t)
      : HelpedCondition(&react_prime_winfiletime, t) { }
    WinFileTimeCondition(const FILETIME &t)
      : WinFileTimeCondition(&t) { }
  };
}
#endif
#endif

#if react_ALLOW_STDTIME
#ifdef CLOCKS_PER_SEC
namespace react {
  struct StdTimeCondition : public HelpedCondition<const time_t *> {
    StdTimeCondition(const time_t *t)
      : HelpedCondition(&react_prime_stdtime, t) { }
    StdTimeCondition(const time_t &t)
      : StdTimeCondition(&t) { }
  };
}
#endif
#endif

#endif


#if react_ALLOW_TIMEVAL
struct timeval;
namespace react {
  typedef UnaryCondition< ::timeval, react_CTIMEVAL> Condition_TIMEVAL;
}
#endif

#if react_ALLOW_TIMESPEC
struct timespec;
namespace react {
  typedef UnaryCondition< ::timespec, react_CTIMESPEC> Condition_TIMESPEC;
}
#endif

#if react_ALLOW_STDTIME
#ifdef CLOCKS_PER_SEC
namespace react {
  typedef UnaryCondition< ::time_t, react_CSTDTIME> Condition_STDTIME;
}
#endif
#endif

#endif
