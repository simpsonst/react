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

#include <assert.h>
#include <errno.h>

#include "common.h"
#include "react/fd.h"
#include "react/event.h"

#if react_ALLOW_FD
#include <sys/uio.h>

int react_prime_fdin(struct react_reg *r, int fd)
{
  return react_prime_fd(r, fd, react_MIN);
}

int react_prime_fdout(struct react_reg *r, int fd)
{
  return react_prime_fd(r, fd, react_MOUT);
}

int react_prime_fdexc(struct react_reg *r, int fd)
{
  return react_prime_fd(r, fd, react_MEXC);
}

static inline void setint(int *p, int v)
{
  if (p) *p = v;
}

static inline void setrsz(react_ssize_t *p, react_ssize_t v)
{
  if (p) *p = v;
}

static void on_readv(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = readv(r->proact.sub.readwritev.fd,
                           r->proact.sub.readwritev.v0,
                           r->proact.sub.readwritev.nv);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.readwritev.rc, rc);
  react_trigger(r);
}

int react_prime_readv(struct react_reg *r, int fd,
                      const struct iovec *v0, int nv, ssize_t *rc, int *en)
{
  int orc = react_prime_fdin(r, fd);
  if (orc < 0) return orc;
  react_swapact(r, &on_readv, &r->proact.act);
  r->proact.sub.readwritev.rc = rc;
  r->proact.sub.readwritev.fd = fd;
  r->proact.sub.readwritev.v0 = v0;
  r->proact.sub.readwritev.nv = nv;
  r->proact.en = en;
  return 0;
}

static void on_writev(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = writev(r->proact.sub.readwritev.fd,
                            r->proact.sub.readwritev.v0,
                            r->proact.sub.readwritev.nv);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.readwritev.rc, rc);
  react_trigger(r);
}

int react_prime_writev(struct react_reg *r, int fd,
                       const struct iovec *v0, int nv, ssize_t *rc, int *en)
{
  int orc = react_prime_fdout(r, fd);
  if (orc < 0) return orc;
  react_swapact(r, &on_writev, &r->proact.act);
  r->proact.sub.readwritev.rc = rc;
  r->proact.sub.readwritev.fd = fd;
  r->proact.sub.readwritev.v0 = v0;
  r->proact.sub.readwritev.nv = nv;
  r->proact.en = en;
  return 0;
}

static void on_read(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = read(r->proact.sub.read.fd,
                          r->proact.sub.read.buf,
                          r->proact.sub.read.len);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.read.rc, rc);
  react_trigger(r);
}

int react_prime_read(struct react_reg *r, int fd, void *buf, size_t len,
                     ssize_t *rc, int *en)
{
  int orc = react_prime_fdin(r, fd);
  if (orc < 0) return orc;
  react_swapact(r, &on_read, &r->proact.act);
  r->proact.sub.read.rc = rc;
  r->proact.sub.read.fd = fd;
  r->proact.sub.read.buf = buf;
  r->proact.sub.read.len = len;
  r->proact.en = en;
  return 0;
}

static void on_write(struct react_reg *r)
{
  (*r->proact.act)(r);
  react_ssize_t rc = write(r->proact.sub.write.fd,
                           r->proact.sub.write.buf,
                           r->proact.sub.write.len);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.write.rc, rc);
  react_trigger(r);
}

int react_prime_write(struct react_reg *r, int fd, const void *buf, size_t len,
                      ssize_t *rc, int *en)
{
  int orc = react_prime_fdin(r, fd);
  if (orc < 0) return orc;
  react_swapact(r, &on_write, &r->proact.act);
  r->proact.sub.write.rc = rc;
  r->proact.sub.write.fd = fd;
  r->proact.sub.write.buf = buf;
  r->proact.sub.write.len = len;
  r->proact.en = en;
  return 0;
}

#if react_ALLOW_FDSPLICE
static void on_spliceread(struct react_reg *r)
{
  (*r->proact.act)(r);
  ssize_t rc = splice(r->proact.sub.splice.fd_in,
                      r->proact.sub.splice.off_in,
                      r->proact.sub.splice.fd_out,
                      r->proact.sub.splice.off_out,
                      r->proact.sub.splice.max,
                      r->proact.sub.splice.flags);
  if (rc < 0)
    setint(r->proact.en, errno);
  setrsz(r->proact.sub.splice.rc, rc);
  react_trigger(r);
}

static void on_splicewrite(struct react_reg *r)
{
  (*r->proact.act)(r);
  int orc = react_prime_fdin(r, r->proact.sub.splice.fd_in);
  if (orc < 0) {
    setint(r->proact.en, errno);
    setrsz(r->proact.sub.splice.rc, -2);
    return;
  }
  react_swapact(r, &on_spliceread, &r->proact.act);
}

int react_prime_splice_writefirst(struct react_reg *r,
                                  int fd_in, loff_t *off_in,
                                  int fd_out, loff_t *off_out, size_t max,
                                  unsigned flags, ssize_t *rc, int *en)
{
  int orc = react_prime_fdout(r, fd_out);
  if (orc < 0) return orc;
  react_swapact(r, &on_splicewrite, &r->proact.act);
  r->proact.sub.splice.rc = rc;
  r->proact.sub.splice.fd_in = fd_in;
  r->proact.sub.splice.fd_out = fd_out;
  r->proact.sub.splice.off_in = off_in;
  r->proact.sub.splice.off_out = off_out;
  r->proact.sub.splice.flags = flags;
  r->proact.sub.splice.max = max;
  r->proact.en = en;
  return 0;
}

static void on_splicedualready(struct react_reg *r)
{
  (*r->proact.act)(r);

#if react_ALLOW_POLLS
  if (r->proact.sub.splice.polls[0].revents & POLLHUP) {
    /* The input has ended.  We don't even need to call splice(), just
       report a zero-length transfer. */
    setrsz(r->proact.sub.splice.rc, 0);
    react_trigger(r);
    return;
  }

  if (r->proact.sub.splice.polls[0].revents & POLLERR) {
    setint(r->proact.en, EBADF);
    setrsz(r->proact.sub.splice.rc, -4);
    react_trigger(r);
    return;
  }

  if (r->proact.sub.splice.polls[0].revents & POLLNVAL) {
    setint(r->proact.en, EINVAL);
    setrsz(r->proact.sub.splice.rc, -4);
    react_trigger(r);
    return;
  }

  if (r->proact.sub.splice.polls[1].revents & POLLERR) {
    setint(r->proact.en, EBADF);
    setrsz(r->proact.sub.splice.rc, -5);
    react_trigger(r);
    return;
  }

  if (r->proact.sub.splice.polls[1].revents & POLLNVAL) {
    setint(r->proact.en, EINVAL);
    setrsz(r->proact.sub.splice.rc, -5);
    react_trigger(r);
    return;
  }
#endif

  int read_okay, write_okay;
#if react_ALLOW_POLLS
  read_okay = r->proact.sub.splice.polls[0].revents & POLLIN;
  write_okay = r->proact.sub.splice.polls[1].revents & POLLOUT;
#else
  read_okay = FD_ISSET(r->proact.sub.splice.fd_in, &r->proact.sub.splice.ins);
  write_okay = FD_ISSET(r->proact.sub.splice.fd_out, &r->proact.sub.splice.outs);
#endif
  if (read_okay) {
    if (write_okay) {
      /* Both FDs are ready. */
      ssize_t rc = splice(r->proact.sub.splice.fd_in,
                          r->proact.sub.splice.off_in,
                          r->proact.sub.splice.fd_out,
                          r->proact.sub.splice.off_out,
                          r->proact.sub.splice.max,
                          r->proact.sub.splice.flags);
      if (rc < 0)
        setint(r->proact.en, errno);
      setrsz(r->proact.sub.splice.rc, rc);
      react_trigger(r);
      return;
    } else {
      /* Input is ready, but we still have to wait for output. */
      int orc;
#if react_ALLOW_POLLS
      orc = react_prime_polls(r, &r->proact.sub.splice.polls[1], 1);
#else
      FD_ZERO(&r->proact.sub.splice.outs);
      FD_SET(r->proact.sub.splice.fd_out, &r->proact.sub.splice.outs);
      orc = react_prime_fds(r, r->proact.sub.splice.fd_out + 1, NULL,
                            &r->proact.sub.splice.outs, NULL);
#endif
      
      if (orc < 0) {
        setint(r->proact.en, errno);
        setrsz(r->proact.sub.splice.rc, -3);
        react_trigger(r);
        return;
      }
    }
  } else {
    /* Input is not ready, but we shouldn't have been called unless
       something is ready, so it must be the output. */
    assert(write_okay);

    /* Watch for the input event. */
    int orc;
#if react_ALLOW_POLLS
    orc = react_prime_polls(r, &r->proact.sub.splice.polls[0], 1);
#else
    FD_ZERO(&r->proact.sub.splice.ins);
    FD_SET(r->proact.sub.splice.fd_in, &r->proact.sub.splice.ins);
    orc = react_prime_fds(r, r->proact.sub.splice.fd_in + 1,
                          &r->proact.sub.splice.ins, NULL, NULL);
#endif
    if (orc < 0) {
      setint(r->proact.en, errno);
      setrsz(r->proact.sub.splice.rc, -2);
      react_trigger(r);
      return;
    }
  }

  /* Check the return value for the priming. */
  react_swapact(r, &on_splicedualready, &r->proact.act);
}

int react_prime_splice_dual(struct react_reg *r,
                            int fd_in, loff_t *off_in,
                            int fd_out, loff_t *off_out, size_t max,
                            unsigned flags, ssize_t *rc, int *en)
{
  int orc;
#if react_ALLOW_POLLS
  /* Define pollfd structures for our two events. */
  r->proact.sub.splice.polls[0].fd = fd_in;
  r->proact.sub.splice.polls[0].events = POLLIN;
  r->proact.sub.splice.polls[1].fd = fd_out;
  r->proact.sub.splice.polls[1].events = POLLOUT;
  orc = react_prime_polls(r, r->proact.sub.splice.polls, 2);
#else
  /* Create FD sets of the descriptors.  We'll watch for them
     together. */
  FD_ZERO(&r->proact.sub.splice.ins);
  FD_SET(fd_in, &r->proact.sub.splice.ins);
  FD_ZERO(&r->proact.sub.splice.outs);
  FD_SET(fd_out, &r->proact.sub.splice.outs);
  const int maxfd = (fd_in > fd_out ? fd_in : fd_out) + 1;
  orc = react_prime_fds(r, maxfd, &r->proact.sub.splice.ins,
                        &r->proact.sub.splice.outs, NULL);
#endif
  if (orc < 0) return orc;
  react_swapact(r, &on_splicedualready, &r->proact.act);
  r->proact.sub.splice.rc = rc;
  r->proact.sub.splice.fd_in = fd_in;
  r->proact.sub.splice.fd_out = fd_out;
  r->proact.sub.splice.off_in = off_in;
  r->proact.sub.splice.off_out = off_out;
  r->proact.sub.splice.flags = flags;
  r->proact.sub.splice.max = max;
  r->proact.en = en;
  return 0;
}

#endif // splice supported

#endif
