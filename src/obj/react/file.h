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

#if react_ALLOW_FILE
  int react_prime_file(struct react_reg *, int, react_iomode_t);
  int react_prime_filein(struct react_reg *r, int fd);
  int react_prime_fileout(struct react_reg *r, int fd);
  int react_prime_fileexc(struct react_reg *r, int fd);
#endif

#if react_ALLOW_STDFILE
#ifdef FILENAME_MAX
  int react_prime_stdfilein(struct react_reg *r, FILE *);
  int react_prime_stdfileout(struct react_reg *r, FILE *);
#endif
#endif

#if react_ALLOW_STDIN
  int react_prime_stdin(struct react_reg *r);
#endif

#ifdef __cplusplus
}
#endif
