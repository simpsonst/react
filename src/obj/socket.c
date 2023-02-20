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

#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <errno.h>

#include "common.h"
#include "react/socket.h"
#include "react/fd.h"
#include "react/idle.h"
#include "react/event.h"
#include "react/windows.h"

#if react_ALLOW_SOCK
int react_prime_sock(struct react_reg *r, react_sock_t fd, react_iomode_t m)
{
#if react_IMPL_SOCK_WIN
  /* Translate the portable mode into Windows modes. */
  long mode;
  switch (m) {
  case react_MIN:
    mode = FD_READ | FD_ACCEPT | FD_CLOSE;
    break;

  case react_MOUT:
    mode = FD_WRITE | FD_CONNECT;
    break;

  case react_MINPRI:
    mode = FD_OOB;
    break;

  default:
    errno = react_EINVAL;
    return -1;
  }

  return react_prime_winsock(r, fd, mode, NULL, NULL);
#elif react_IMPL_SOCK_FD || react_IMPL_SOCK_RISCOS
  return react_prime_fd(r, fd, m);
#else
#error "No implementation"
  return -1;
#endif  
}


int react_prime_sockin(struct react_reg *r, react_sock_t fd)
{
  return react_prime_sock(r, fd, react_MIN);
}

int react_prime_sockout(struct react_reg *r, react_sock_t fd)
{
  return react_prime_sock(r, fd, react_MOUT);
}

int react_prime_sockexc(struct react_reg *r, react_sock_t fd)
{
  return react_prime_sock(r, fd, react_MEXC);
}


static inline void setint(int *p, int v)
{
  if (p) *p = v;
}

static inline void setrsz(react_ssize_t *p, react_ssize_t v)
{
  if (p) *p = v;
}

static inline void setrsl(react_socklen_t *p, react_socklen_t v)
{
  if (p) *p = v;
}

static void on_recv(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = recv(r->proact.sub.recv.fd,
                          r->proact.sub.recv.buf,
                          r->proact.sub.recv.len,
                          r->proact.sub.recv.flags);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.recv.rc, rc);
  react_trigger(r);
}

int react_prime_recv(struct react_reg *r, react_sock_t sockfd,
                     void *buf, react_buflen_t len, int flags,
                     react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_recv, &r->proact.act);
  r->proact.sub.recv.fd = sockfd;
  r->proact.sub.recv.buf = buf;
  r->proact.sub.recv.len = len;
  r->proact.sub.recv.flags = flags;
  r->proact.sub.recv.rc = rc;
  r->proact.en = en;
  return 0;
}

static void on_recvfrom(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = recvfrom(r->proact.sub.recv.fd,
                              r->proact.sub.recv.buf,
                              r->proact.sub.recv.len,
                              r->proact.sub.recv.flags,
                              r->proact.sub.recv.addr,
                              r->proact.sub.recv.addrlen);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.recv.rc, rc);
  react_trigger(r);
}

int react_prime_recvfrom(struct react_reg *r, react_sock_t sockfd,
                         void *buf, react_buflen_t len, int flags,
                         struct sockaddr *addr, react_socklen_t *addrlen,
                         react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_recvfrom, &r->proact.act);
  r->proact.sub.recv.fd = sockfd;
  r->proact.sub.recv.buf = buf;
  r->proact.sub.recv.len = len;
  r->proact.sub.recv.flags = flags;
  r->proact.sub.recv.rc = rc;
  r->proact.sub.recv.addr = addr;
  r->proact.sub.recv.addrlen = addrlen;
  r->proact.en = en;
  return 0;
}








static void on_send(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = send(r->proact.sub.send.fd,
                          r->proact.sub.send.buf,
                          r->proact.sub.send.len,
                          r->proact.sub.send.flags);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.send.rc, rc);
  react_trigger(r);
}

int react_prime_send(struct react_reg *r, react_sock_t sockfd,
                     const void *buf, react_buflen_t len, int flags,
                     react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_send, &r->proact.act);
  r->proact.sub.send.fd = sockfd;
  r->proact.sub.send.buf = buf;
  r->proact.sub.send.len = len;
  r->proact.sub.send.flags = flags;
  r->proact.sub.send.rc = rc;
  r->proact.en = en;
  return 0;
}

static void on_sendto(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = sendto(r->proact.sub.send.fd,
                            r->proact.sub.send.buf,
                            r->proact.sub.send.len,
                            r->proact.sub.send.flags,
                            r->proact.sub.send.addr,
                            r->proact.sub.send.addrlen);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.send.rc, rc);
  react_trigger(r);
}

int react_prime_sendto(struct react_reg *r, react_sock_t sockfd,
                       const void *buf, react_buflen_t len, int flags,
                       const struct sockaddr *addr, react_socklen_t addrlen,
                       react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_sendto, &r->proact.act);
  r->proact.sub.send.fd = sockfd;
  r->proact.sub.send.buf = buf;
  r->proact.sub.send.len = len;
  r->proact.sub.send.flags = flags;
  r->proact.sub.send.rc = rc;
  r->proact.sub.send.addr = addr;
  r->proact.sub.send.addrlen = addrlen;
  r->proact.en = en;
  return 0;
}

#if react_ALLOW_SOCKMSG
static void on_sendmsg(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = sendmsg(r->proact.sub.sendmsg.fd,
                             r->proact.sub.sendmsg.msg,
                             r->proact.sub.sendmsg.flags);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.sendmsg.rc, rc);
  react_trigger(r);
}

int react_prime_sendmsg(struct react_reg *r, react_sock_t sockfd,
                        const struct msghdr *msg, int flags,
                        react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_sendmsg, &r->proact.act);
  r->proact.sub.sendmsg.fd = sockfd;
  r->proact.sub.sendmsg.msg = msg;
  r->proact.sub.sendmsg.flags = flags;
  r->proact.sub.sendmsg.rc = rc;
  r->proact.en = en;
  return 0;
}

static void on_recvmsg(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = recvmsg(r->proact.sub.recvmsg.fd,
                             r->proact.sub.recvmsg.msg,
                             r->proact.sub.recvmsg.flags);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.recvmsg.rc, rc);
  react_trigger(r);
}

int react_prime_recvmsg(struct react_reg *r, react_sock_t sockfd,
                        struct msghdr *msg, int flags,
                        react_ssize_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_recvmsg, &r->proact.act);
  r->proact.sub.recvmsg.fd = sockfd;
  r->proact.sub.recvmsg.msg = msg;
  r->proact.sub.recvmsg.flags = flags;
  r->proact.sub.recvmsg.rc = rc;
  r->proact.en = en;
  return 0;
}
#endif




static void on_accept(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_sock_t sock =
#if react_ALLOW_ACCEPT4
    r->proact.sub.accept.flags != 0 ?
    accept4(r->proact.sub.accept.fd,
            r->proact.sub.accept.addr,
            r->proact.sub.accept.addrlen,
            r->proact.sub.accept.flags) :
#endif
    accept(r->proact.sub.accept.fd,
           r->proact.sub.accept.addr,
           r->proact.sub.accept.addrlen);
  if (sock == react_INVALID_SOCKET)
    setint(r->proact.en, errno);
  *r->proact.sub.accept.rc = sock;
  react_trigger(r);
}

int react_prime_accept(struct react_reg *r, react_sock_t sockfd,
                       struct sockaddr *addr, react_socklen_t *addrlen,
                       react_sock_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_accept, &r->proact.act);
  r->proact.sub.accept.fd = sockfd;
  r->proact.sub.accept.rc = rc;
  r->proact.sub.accept.addr = addr;
  r->proact.sub.accept.addrlen = addrlen;
  r->proact.sub.accept.flags = 0;
  r->proact.en = en;
  return 0;
}

#if react_ALLOW_ACCEPT4
int react_prime_accept4(struct react_reg *r, react_sock_t sockfd,
                        struct sockaddr *addr, react_socklen_t *addrlen,
                        int flags,
                        react_sock_t *rc, int *en)
{
  int orc = react_prime_sock(r, sockfd, react_MIN);
  if (orc < 0) return orc;
  react_swapact(r, &on_accept, &r->proact.act);
  r->proact.sub.accept.fd = sockfd;
  r->proact.sub.accept.rc = rc;
  r->proact.sub.accept.addr = addr;
  r->proact.sub.accept.addrlen = addrlen;
  r->proact.sub.accept.flags = flags;
  r->proact.en = en;
  return 0;
}
#endif

static void on_connect(struct react_reg *r)
{
  (*r->proact.act)(r);
  int rc = connect(r->proact.sub.connect.fd,
                     r->proact.sub.connect.addr,
                     r->proact.sub.connect.addrlen);
  setint(r->proact.sub.connect.rc, rc);
  if (rc < 0)
    setint(r->proact.en, errno);
  react_trigger(r);
}

int react_prime_connect(struct react_reg *r, react_sock_t sockfd,
                        const struct sockaddr *addr, react_socklen_t addrlen,
                        int *rc, int *en)
{
  react_ioctlflag_t flag = 1;
  if (ioctlsocket(sockfd, FIONBIO, &flag) != 0) {
    setint(en, errno);
    setint(rc, react_SOCKET_ERROR);
  }
  int crc = connect(sockfd, addr, addrlen);
  if (crc == 0) {
    setint(rc, crc);
    return react_prime_idle(r);
  }
#ifdef __WIN32__
  const int okay = WSAGetLastError() == WSAEWOULDBLOCK;
#else
  const int okay = errno == EINPROGRESS;
#endif
  if (!okay) {
    setint(en, errno);
    setint(rc, crc);
    return react_prime_idle(r);
  }
  int orc = react_prime_sock(r, sockfd, react_MOUT);
  if (orc < 0) return orc;
  react_swapact(r, &on_connect, &r->proact.act);
  r->proact.sub.connect.fd = sockfd;
  r->proact.sub.connect.addr = addr;
  r->proact.sub.connect.addrlen = addrlen;
  r->proact.sub.connect.rc = rc;
  r->proact.en = en;
  return 0;
}
#endif
