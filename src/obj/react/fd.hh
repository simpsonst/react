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

#ifndef react_fd_CXXHDRINC
#define react_fd_CXXHDRINC

#include "fd.h"
#include "types.hh"
#include "condhelp.hh"

namespace react {
#if __cplusplus >= 201103L
#if react_ALLOW_FD
  struct DescriptorCondition :
    public HelpedCondition<int, react_iomode_t> {
    DescriptorCondition(int fd, react_iomode_t mode)
      : HelpedCondition(&react_prime_fd, fd, mode) { }
  };
#endif

  /* TODO: Proactor-like conditions */
#endif
}

#ifdef react_ALLOW_FD
#include "conds.h"
namespace react {
  typedef BinaryStructCondition<react_fdcond_t,
                                int,
                                &react_fdcond_t::handle,
                                iomode_t,
                                &react_fdcond_t::mode,
                                react_CFD> Condition_FD;
}
#endif


#endif
