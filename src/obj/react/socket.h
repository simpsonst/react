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

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if react_ALLOW_SOCK
#if react_sock_DEFINED
  int react_prime_sock(struct react_reg *, react_sock_t, react_iomode_t);
  int react_prime_sockin(struct react_reg *, react_sock_t);
  int react_prime_sockout(struct react_reg *, react_sock_t);
  int react_prime_sockexc(struct react_reg *, react_sock_t);

#if react_ssize_DEFINED
#if react_buflen_DEFINED
  int react_prime_recv(struct react_reg *, react_sock_t,
                       void *, react_buflen_t, int, react_ssize_t *, int *en);
  int react_prime_send(struct react_reg *, react_sock_t,
                       const void *, react_buflen_t, int,
                       react_ssize_t *, int *en);
#if react_socklen_DEFINED
  int react_prime_sendto(struct react_reg *, react_sock_t,
                         const void *, react_buflen_t, int,
                         const struct sockaddr *, react_socklen_t,
                         react_ssize_t *, int *en);
  int react_prime_recvfrom(struct react_reg *, react_sock_t,
                           void *, react_buflen_t, int,
                           struct sockaddr *, react_socklen_t *,
                           react_ssize_t *, int *en);
#endif // socklen
#endif // buflen

#if react_ALLOW_SOCKMSG
  struct msghdr;
  int react_prime_recvmsg(struct react_reg *r, react_sock_t sockfd,
                          struct msghdr *msg, int flags,
                          react_ssize_t *rc, int *en);
  int react_prime_sendmsg(struct react_reg *, react_sock_t,
                          const struct msghdr *, int flags,
                          react_ssize_t *, int *en);
#endif // sockmsg
#endif // ssize

#if react_socklen_DEFINED
  struct sockaddr;
  int react_prime_accept(struct react_reg *, react_sock_t,
                         struct sockaddr *, react_socklen_t *,
                         react_sock_t *, int *en);
#if react_ALLOW_ACCEPT4
  int react_prime_accept4(struct react_reg *, react_sock_t,
                          struct sockaddr *, react_socklen_t *, int flags,
                          react_sock_t *, int *en);
#endif
  int react_prime_connect(struct react_reg *, react_sock_t,
                          const struct sockaddr *, react_socklen_t,
                          int *rc, int *en);
#endif // socklen
#endif // sock
#endif

#ifdef INVALID_SOCKET
  /* You need <WinSock2.h> to get this. */
#if react_ALLOW_WINSOCK
  struct react_reg;
  int react_prime_winsock(struct react_reg *r, SOCKET hdl,
                          long mode, long *got, DWORD *st);
#endif
#endif

#ifdef __cplusplus
}
#endif
