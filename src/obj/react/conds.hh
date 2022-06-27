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

/* Everything in here will eventually be deprecated.  It's the old way
   of passing arbitrary parameters to a single priming function, now
   replaced by multiple (and platform-specific) priming functions that
   take the right arguments. */

#ifndef react_conds_CXXHDRINC
#define react_conds_CXXHDRINC

#include <ctime>

#include "features.h"

#include "condtype.h"
#include "conds.h"
#include "condition.hh"
#include "condhelp.hh"

#include "idle.hh"
#include "time.hh"
#include "windows.hh"
#include "socket.hh"
#include "fd.hh"
#include "file.hh"
#include "pipe.hh"

#endif
