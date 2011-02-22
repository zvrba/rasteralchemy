/*
  This file is a part of the Raster Alchemy package.
  Copyright (C) 2004  Zeljko Vrba

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  You can reach me at the following e-mail addresses:

  zvrba@globalnet.hr
  mordor@fly.srk.fer.hr
*/
extern "C" {
#include <lua/lua.h>
#include <lua/lualib.h>
}
#include <luabind/luabind.hpp>
#include <luabind/out_value_policy.hpp>
//#include <luabind/functor.hpp>

#include "hf-hl.h"
#include "hf-sclxform.h"

static char rcsid[] UNUSED = "$Id: lua-sclxform.cc,v 1.1.2.5 2004/09/24 17:18:23 zvrba Exp $";

#define REGISTERop1_0(sname) class_<op1_##sname, bases<scalar_op1> >(L, STR_(_hfop1_##sname)).def(constructor<>())
#define REGISTERop1_1(sname) class_<op1_##sname, bases<scalar_op1> >(L, STR_(_hfop1_##sname)).def(constructor<D>())
#define REGISTERop2_0(sname) class_<op2_##sname, bases<scalar_op2> >(L, STR_(_hfop2_##sname)).def(constructor<>())

// TODO: report error if not passed a function!
struct lua_scalar_op1 : public scalar_op1 {
	//luabind::functor f_;
	luabind::object f_;

	lua_scalar_op1(luabind::object f) : f_(f) { }
	
	const char *_type() const {
		return "lua_scalar_op1";
	}

	PTYPE operator()(PTYPE p) const {
		return luabind::object_cast<PTYPE>(f_(p));
	}
};

struct lua_scalar_op2 : public scalar_op2 {
	//luabind::functor f_;
	luabind::object f_;

	lua_scalar_op2(luabind::object f) : f_(f) { }
	
	const char *_type() const {
		return "lua_scalar_op2";
	}
	
	PTYPE operator()(PTYPE p1, PTYPE p2) const {
		return luabind::object_cast<PTYPE>(f_(p1, p2));
	}
};

void register_lua_sclxform(lua_State *L)
{
	using namespace luabind;

	class_<scalar_op1>(L, "_hfop_scalar_op1");
	class_<scalar_op2>(L, "_hfop_scalar_op2");
	class_<scalar_op1_fc>(L, "_hfop_scalar_op1_fc");
	class_<lua_scalar_op1, bases<scalar_op1> >(L, "_hfop_lua_scalar_op1")
		.def(constructor<object>());
	class_<lua_scalar_op2, bases<scalar_op2> >(L, "_hfop_lua_scalar_op2")
		.def(constructor<object>());

	REGISTERop1_0(sin);
	REGISTERop1_0(asin);
	REGISTERop1_0(cos);
	REGISTERop1_0(acos);
	REGISTERop1_0(tan);
	REGISTERop1_0(atan);
	REGISTERop1_0(abs);
	REGISTERop1_0(inv);
	REGISTERop1_0(log);
	
	REGISTERop1_1(disc);
	REGISTERop1_1(mod);
	REGISTERop1_1(pow1);
	REGISTERop1_1(add1);
	REGISTERop1_1(sub1);
	REGISTERop1_1(mul1);
	REGISTERop1_1(div1);

	class_<op1_floor, bases<scalar_op1> >(L, "_hfop_floor")
		.def(constructor<D, D, D, D>());
	class_<op1_ceil, bases<scalar_op1> >(L, "_hfop_ceil")
		.def(constructor<D, D, D, D>());

	REGISTERop2_0(add2);
	REGISTERop2_0(sub2);
	REGISTERop2_0(mul2);
	REGISTERop2_0(div2);
	REGISTERop2_0(rmag);
	REGISTERop2_0(pow2);
	REGISTERop2_0(com);
	REGISTERop2_0(comn);

	function(L, "_hf_op1", h_op1);
	function(L, "_hf_op2", h_op2);
}
