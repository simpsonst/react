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

#ifndef react_condhelp_CXXHDRINC
#define react_condhelp_CXXHDRINC

#if __cplusplus >= 201103L
#include <tuple>
#endif

#include "condtype.h"
#include "condition.hh"

namespace react {
#if __cplusplus >= 201103L
  template <typename... Types>
  class HelpedCondition : public Condition {
    typedef int func_t(react_t, Types...);

  private:
    func_t *func;
    std::tuple<Types...> args;

    /* See
       <http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer>
       for these incantations: */
    template<int ...> struct seq {};

    template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};

    template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

    template<int ...S>
    int callFunc(react_t r, seq<S...>) {
      return func(r, std::get<S>(args) ...);
    }
    /* End of incantations */

    int prime(react_t r) {
      return callFunc(r, typename gens<sizeof...(Types)>::type());
    }

  public:
    HelpedCondition(func_t *f, Types... args)
      : func(f), args(std::make_tuple(args...)) { }
  };
#endif

  /* Legacy classes */
  class StaticCondition : public Condition {
    react_cond_t t;

    int prime(react_t h);

  public:
    StaticCondition(react_cond_t t) : t(t) { }
  };

  template <class StoreType, react_cond_t condType>
  class UnaryCondition : public Condition {
    StoreType c;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    int prime(react_t h) {
      return react_prime(h, condType, &c);
    }

#pragma GCC diagnostic pop

  public:
    UnaryCondition(StoreType c) : c(c) { }
  };

  template <class StoreType,
            class HandleType,
            HandleType StoreType::*handleMember,
            class ModeType,
            ModeType StoreType::*modeMember,
            react_cond_t condType>
  class BinaryStructCondition : public Condition {
    StoreType c;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    int prime(react_t h) {
      return react_prime(h, condType, &c);
    }

#pragma GCC diagnostic pop

  public:
    BinaryStructCondition(HandleType s, ModeType m) {
      c.*handleMember = s;
      c.*modeMember = m;
    }
  };
}

#endif
