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

#ifndef react_socket_CXXHDRINC
#define react_socket_CXXHDRINC

#include "socket.h"
#include "types.hh"
#include "condhelp.hh"

namespace react {
#if __cplusplus >= 201103L
#if react_ALLOW_SOCK
  struct SocketCondition :
    public HelpedCondition<react_sock_t, react_iomode_t> {
    SocketCondition(react_sock_t sockfd, react_iomode_t mode)
      : HelpedCondition(&react_prime_sock, sockfd, mode) { }
  };
#endif

#if react_ALLOW_WINSOCK
  struct WinSockCondition
    : public HelpedCondition<SOCKET, long, long *, DWORD *> {
    WinSockCondition(SOCKET sock, long mode, long *got, DWORD *st)
      : HelpedCondition(&react_prime_winsock, sock, mode, got, st) { }
    WinSockCondition(SOCKET sock, long mode, long &got, DWORD &st)
      : WinSockCondition(sock, mode, &got, &st) { }
    WinSockCondition(SOCKET sock, long mode, DWORD &st)
      : WinSockCondition(sock, mode, (long *) 0, &st) { }
    WinSockCondition(SOCKET sock, long mode, long &got)
      : WinSockCondition(sock, mode, &got, (DWORD *) 0) { }
    WinSockCondition(SOCKET sock, long mode)
      : WinSockCondition(sock, mode, (long *) 0, (DWORD *) 0) { }
  };
#endif
  /* TODO: Proactor-like conditions */
#endif
}


#ifdef react_ALLOW_SOCK
#include "conds.h"
namespace react {
  typedef BinaryStructCondition<react_sockcond_t,
                                react_sock_t,
                                &react_sockcond_t::handle,
                                iomode_t,
                                &react_sockcond_t::mode,
                                react_CSOCK> Condition_SOCK;
}
#endif


#endif
