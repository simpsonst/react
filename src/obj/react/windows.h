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

#include "features.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MAKEWORD
  /* You need <WinDef.h> to get these. */
#if react_ALLOW_WINHANDLE
  struct react_reg;
  int react_prime_winhandle(struct react_reg *r, HANDLE hdl, DWORD *st);
#endif
#if react_ALLOW_WINMSG
  struct react_reg;
  int react_prime_winmsg(struct react_reg *r, DWORD wakeMask);
#endif
#endif

#if react_ALLOW_WINAPC
  struct react_reg;
  int react_prime_winapc(struct react_reg *r);
#endif

#ifdef __cplusplus
}
#endif
