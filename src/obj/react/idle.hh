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

#ifndef react_idle_CXXHDRINC
#define react_idle_CXXHDRINC

#include "idle.h"
#include "condhelp.hh"

namespace react {
#if 0
#if __cplusplus >= 201103L
  class IdleCondition : public HelpedCondition<> {
  public:
    IdleCondition() : HelpedCondition(&::react_prime_idle) { }
  };
#else
  class IdleCondition : public StaticCondition {
  public:
    IdleCondition() : StaticCondition(react_CIDLE) { }
  };
#endif
#endif

  class IdleCondition : public Condition {
    int prime(react_t h);
  };
}

#endif
