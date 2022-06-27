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

#ifndef react_pipe_CXXHDRINC
#define react_pipe_CXXHDRINC

#include "pipe.h"
#include "types.hh"
#include "condhelp.hh"

namespace react {
#if __cplusplus >= 201103L
#if react_ALLOW_PIPE
  struct PipeCondition : public HelpedCondition<react_pipe_t, react_iomode_t> {
    PipeCondition(react_pipe_t fd, react_iomode_t mode)
      : HelpedCondition(&react_prime_pipe, fd, mode) { }
  };
#endif
#endif // C++ version
}

#ifdef react_ALLOW_PIPE
namespace react {
  typedef BinaryStructCondition<react_pipecond_t,
                                react_pipe_t,
                                &react_pipecond_t::handle,
                                iomode_t,
                                &react_pipecond_t::mode,
                                react_CPIPE> Condition_PIPE;
}
#endif

#endif
