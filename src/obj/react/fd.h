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

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


#if react_ALLOW_FD

#include <sys/types.h>

  int react_prime_fd(struct react_reg *, int, react_iomode_t);
  int react_prime_fdin(struct react_reg *r, int fd);
  int react_prime_fdout(struct react_reg *r, int fd);
  int react_prime_fdexc(struct react_reg *r, int fd);

#ifdef FD_ZERO
  int react_prime_fds(struct react_reg *r, int nfds,
                      fd_set *in, fd_set *out, fd_set *exc);
#endif

  int react_prime_read(struct react_reg *, int fd, void *, size_t,
                       ssize_t *, int *en);
  int react_prime_write(struct react_reg *, int fd, const void *, size_t,
                        ssize_t *, int *en);
#if react_ALLOW_FDSPLICE
#ifdef O_CREAT
  int react_prime_splice_writefirst(struct react_reg *,
                                    int fd_in, loff_t *, int fd_out, loff_t *,
                                    size_t,
                                    unsigned flags, ssize_t *, int *en);
  int react_prime_splice_dual(struct react_reg *,
                              int fd_in, loff_t *, int fd_out, loff_t *,
                              size_t,
                              unsigned flags, ssize_t *, int *en);
#define react_prime_splice react_prime_splice_dual
#endif
#endif

  struct iovec;
  int react_prime_readv(struct react_reg *, int fd,
                        const struct iovec *, int, ssize_t *, int *en);
  int react_prime_writev(struct react_reg *, int fd,
                         const struct iovec *, int, ssize_t *, int *en);
#endif

#if react_ALLOW_POLL
  int react_prime_poll(struct react_reg *, int fd, short evs, short *got);
#endif

#if react_ALLOW_POLLS
#ifdef POLLIN
  struct pollfd;
  int react_prime_polls(struct react_reg *, struct pollfd *, nfds_t);
#endif
#endif

#ifdef __cplusplus
}
#endif
