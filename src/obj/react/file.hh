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

#ifndef react_file_CXXHDRINC
#define react_file_CXXHDRINC

#include "file.h"
#include "types.hh"
#include "condhelp.hh"

namespace react {
#if __cplusplus >= 201103L
#if react_ALLOW_FILE
  struct FileCondition : public HelpedCondition<react_file_t, react_iomode_t> {
    FileCondition(react_file_t fd, react_iomode_t mode)
      : HelpedCondition(&react_prime_file, fd, mode) { }
  };
#endif

#if react_ALLOW_STDFILE
#ifdef FILENAME_MAX
  struct StdFileInCondition : public HelpedCondition<FILE *> {
    StdFileInCondition(FILE *fp)
      : HelpedCondition(&react_prime_stdfilein, fp) { }
  };
  struct StdFileOutCondition : public HelpedCondition<FILE *> {
    StdFileOutCondition(FILE *fp)
      : HelpedCondition(&react_prime_stdfileout, fp) { }
  };
#endif
#endif
#endif // C++ version
}

#if react_ALLOW_FILE
#include "conds.h"
namespace react {
  typedef BinaryStructCondition<react_filecond_t,
                                react_file_t,
                                &react_filecond_t::handle,
                                iomode_t,
                                &react_filecond_t::mode,
                                react_CFILE> Condition_FILE;
}
#endif

#endif
