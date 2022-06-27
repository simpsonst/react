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

#ifndef react_windows_CXXHDRINC
#define react_windows_CXXHDRINC

#include "windows.h"
#include "condhelp.hh"

namespace react {
#if __cplusplus >= 201103L
#if react_ALLOW_WINHANDLE
  struct WinHandleCondition : public HelpedCondition<HANDLE, DWORD *> {
    WinHandleCondition(HANDLE h, DWORD *st)
      : HelpedCondition(&react_prime_winhandle, h, st) { }
    WinHandleCondition(HANDLE h, DWORD &st)
      : WinHandleCondition(h, &st) { }
    WinHandleCondition(HANDLE h)
      : WinHandleCondition(h, (DWORD *) 0) { }
  };
#endif

  /* TODO: winmsg and winapc */
#endif

#ifdef react_ALLOW_WINHANDLE
  typedef UnaryCondition<HANDLE, react_CWINHANDLE> Condition_WINHANDLE;
#endif
}

#ifdef react_ALLOW_WINSOCK
#include "conds.h"
namespace react {
  typedef BinaryStructCondition<react_winsockcond_t,
                                SOCKET,
                                &react_winsockcond_t::handle,
                                long,
                                &react_winsockcond_t::mode,
                                react_CWINSOCK> Condition_WINSOCK;
}
#endif


#endif
