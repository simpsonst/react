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

#include <stdio.h>

#include "common.h"
#include "react/file.h"
#include "react/fd.h"
#include "react/windows.h"

#if react_ALLOW_FILE
int react_prime_file(struct react_reg *r, int fd, react_iomode_t m)
{
#if react_IMPL_FILE_FD
  return react_prime_fd(r, fd, m);
#else
#error "No implementation"
  return -1;
#endif
}

int react_prime_filein(struct react_reg *r, int fd)
{
  return react_prime_file(r, fd, react_MIN);
}

int react_prime_fileout(struct react_reg *r, int fd)
{
  return react_prime_file(r, fd, react_MOUT);
}

int react_prime_fileexc(struct react_reg *r, int fd)
{
  return react_prime_file(r, fd, react_MEXC);
}
#endif

#if react_ALLOW_STDFILE
#ifdef __WIN32__
static HANDLE handle_of_stream(FILE *fp)
{
  /* From
     <http://stackoverflow.com/questions/3989545/how-do-i-get-the-file-handle-from-the-fopen-file-structure>.
     Doesn't seem to work, though.  Do we have to modify the handle to
     say what we're waiting for?  Do we have to destroy the handle
     after use?  */
  return (HANDLE) _get_osfhandle(_fileno(fp));
}
#endif

int react_prime_stdfilein(struct react_reg *r, FILE *fp)
{
#if react_IMPL_STDFILE_FD
  return react_prime_fdin(r, fileno(fp));
#elif react_IMPL_STDFILE_WINHANDLE
  return react_prime_winhandle(r, handle_of_stream(fp), NULL);
#else
#error "No implementation"
  return -1;
#endif
}

int react_prime_stdfileout(struct react_reg *r, FILE *fp)
{
#if react_IMPL_STDFILE_FD
  return react_prime_fdout(r, fileno(fp));
#elif react_IMPL_STDFILE_WINHANDLE
  return react_prime_winhandle(r, handle_of_stream(fp), NULL);
#else
#error "No implementation"
  return -1;
#endif
}
#endif


#if react_ALLOW_STDIN
int react_prime_stdin(struct react_reg *r)
{
#if react_IMPL_STDIN_STDFILE
  return react_prime_stdfilein(r, stdin);
#elif react_IMPL_STDIN_WINHANDLE
  return react_prime_winhandle(r, GetStdHandle(STD_INPUT_HANDLE), NULL);
#else
#error "No implementation"
  return -1;
#endif
}
#endif

