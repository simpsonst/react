// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
 *  React++, C++ veneer for C reactor
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


#include <time.h>
#include <stdio.h>

#include "features.h"

#include "react/condhelp.hh"
#include "react/idle.hh"
#include "react/time.hh"
#include "react/windows.hh"
#include "react/socket.hh"
#include "react/fd.hh"
#include "react/file.hh"
#include "react/pipe.hh"

int react::IdleCondition::prime(react_t h)
{
  return react_prime_idle(h);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

int react::StaticCondition::prime(react_t h)
{
  return react_prime(h, t, NULL);
}

#pragma GCC diagnostic pop
