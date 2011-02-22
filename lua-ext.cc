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

#include <GL/gl.h>
#include <unistd.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfCompression.h>
#include <ImfChannelList.h>

#include "rdispwin.h"
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: lua-ext.cc,v 1.1.2.13 2004/09/24 17:18:23 zvrba Exp $";

// just send a message to GUI through a socket
static void display(RasterDisplayWindow*, const hfield *hf)
{
	extern int GuiSocket[2];

	if(write(GuiSocket[1], &hf, sizeof(hf)) < sizeof(hf)) {
		fxerror("lua-ext.cc: display: can't send message.\n");
	}
}

/**
   Save HF to EXR file. Real images have 1 channel named "H"; complex images
   have 2 channels named "RE" and "IM" (all in uppercase).  Uses 'ZIP'
   compression.
*/
static bool save(const char *fname, const hfield *hf) try {
	using namespace Imf;
	unsigned int w = hf->xsize, h = hf->ysize;

	Header header(w, h);
	FrameBuffer fb;

	if(!hf->c) {
		header.channels().insert("H", Channel(FLOAT));
		fb.insert("H", Slice(FLOAT, (char*)hf->a,
							 sizeof(float), sizeof(float) * w));
	} else {
		header.channels().insert("RE", Channel(FLOAT));
		header.channels().insert("IM", Channel(FLOAT));
		fb.insert("RE", Slice(FLOAT, (char*)hf->a,
							  sizeof(float), sizeof(float) * w));
		fb.insert("IM", Slice(FLOAT, (char*)(hf->a + w*h),
							  sizeof(float), sizeof(float) * w));
	}
	header.compression() = Compression(ZIP_COMPRESSION);

	OutputFile f(fname, header);
	f.setFrameBuffer(fb);
	f.writePixels(h);

	return true;
} catch(Iex::BaseExc &e) {
	fxmessage("lua-ext.cc: save: %s\n", e.what());
	return false;
}

/**
   Load HF from EXR file. The file must either have 1 channel named H or 2
   channels named RE, IM. In any other case loading fails and NULL pointer
   is returned.
*/
static hfield *load(const char *fname)
{
	using namespace Imf;
	using namespace Imath;

	hfield *hf = 0;

	InputFile f(fname);
	Box2i dw = f.header().dataWindow();
	Box2i dispw = f.header().displayWindow();
	if(dw != dispw) {
		fxmessage("lua-ext.cc: load: different display and data window sizes.\n");
		return 0;
	}
	
	FrameBuffer fb;
	unsigned int w = dw.max.x - dw.min.x + 1, h = dw.max.y - dw.min.y + 1;

	// allocate complex or real image
	const ChannelList &chl = f.header().channels();
	const Channel *ch0 = chl.findChannel("H"), *ch1 = chl.findChannel("IM");
	if(ch0) {
		if(ch1) {
			fxmessage("lua-ext.cc: load: incosistent channels.\n");
			return 0;
		}

		hf = h_newr(w, h);
		fb.insert("H", Slice(FLOAT, (char*)hf->a,
							 sizeof(float), sizeof(float) * w));
	} else if(ch1) {
		if(ch0) {
			fxmessage("lua-ext.cc: load: incosistent channels.\n");
			return 0;
		}
		if(!(ch0 = chl.findChannel("RE"))) {
			fxmessage("lua-ext.cc: load: imaginary without real channel.\n");
			return 0;
		}

		hf = h_newc(w, h);
		fb.insert("RE", Slice(FLOAT, (char*)hf->a,
							  sizeof(float), sizeof(float) * w));
		fb.insert("IM", Slice(FLOAT, (char*)(hf->a + w*h),
							  sizeof(float), sizeof(float) * w));
	}

	try {
		f.setFrameBuffer(fb);
		f.readPixels(dw.min.y, dw.max.y);
	} catch(Iex::BaseExc &e) {
		fxmessage("lua-ext.cc: load: %s\n", e.what());
		h_delete(hf); hf = 0;
	}

	return hf;
}

static const char *rdispwin_type_string(RasterDisplayWindow*)
{
	return "RasterDisplayWindow";
}

void register_lua_rdispwin(lua_State *L)
{
	using namespace luabind;

	extern RasterDisplayWindow *RasterWindow;

	class_<RasterDisplayWindow>(L, "RasterDisplayWindow")
		.def("setImage", display)
		.def("_type", rdispwin_type_string);
	
	object globals = get_globals(L);
	globals["RasterWindow"] = RasterWindow;
}

static const char *hfparams_type_string(struct HF_PARAMS*)
{
	return "HF_PARAMS";
}

/**
   @todo Must adjust LUABIND_MAX_ARITY to 8 (or greater, if necessary)
   to wrap some functions with many parameters.
*/
void register_lua_hf(lua_State *L)
{
	using namespace luabind;

	object globals = get_globals(L);
	object hf = globals["hf"] = newtable(L);

	globals["PI"] = M_PI;
	globals["E"] = M_E;

	class_<struct HF_PARAMS>(L, "HF_PARAMS")
		.def("_type", hfparams_type_string)
		.def_readwrite("rand_gauss", &HF_PARAMS::rand_gauss)
		.def_readwrite("rnd_seed", &HF_PARAMS::rnd_seed)
		.def_readwrite("rnd_seed_stale", &HF_PARAMS::rnd_seed_stale)
		.def_readwrite("histbins", &HF_PARAMS::histbins)
		.def_readwrite("tile_mode", &HF_PARAMS::tile_mode)
		.def_readwrite("tile_tol", &HF_PARAMS::tile_tol)
		.def_readwrite("gaufac", &HF_PARAMS::gaufac)
		.enum_("TILE")
		[
			value("TILE_AUTO", 0),
			value("TILE_OFF", 1),
			value("TILE_ON", 2)
		];
	hf["PARAMS"] = &HF_PARAMS;
	
	class_<hfield>(L, "hfield")
		.def("_type", &hfield::_type)
		.def("_hkey", &hfield::_hkey)
		.def_readonly("width", &hfield::xsize)
		.def_readonly("height", &hfield::ysize)
		.def_readonly("min", &hfield::min)
		.def_readonly("max", &hfield::max)
		.def_readonly("cplx", &hfield::c)
		.def(const_self == other<hfield>());

	function(L, "_hf_delete", h_delete);
	function(L, "_hf_minmax", h_minmax);
	function(L, "_hf_rminmax", r_minmax, pure_out_value(_2) + pure_out_value(_3));
	function(L, "_hf_iminmax", i_minmax, pure_out_value(_2) + pure_out_value(_3));
	function(L, "_hf_histeq", histeq);
	function(L, "_hf_hist", h_hist);
	function(L, "_hf_hshift", h_hshift);
	function(L, "_hf_norm", norm);
	function(L, "_hf_negate", negate);
	function(L, "_hf_slopelim", h_slopelim);
	function(L, "_hf_diff", h_diff);
	function(L, "_hf_rotate", h_rotate);
	function(L, "_hf_peak", h_peak);
	function(L, "_hf_is_tilable", is_tilable);
	function(L, "_hf_tilable", h_tilable);
	function(L, "_hf_zero", h_zero);
	function(L, "_hf_const", h_const);
	function(L, "_hf_cswap", c_swap);
	function(L, "_hf_cjoin", c_join);
	function(L, "_hf_csplit", c_split, pure_out_value(_2) + pure_out_value(_3));
	function(L, "_hf_cmag", c_mag);
	function(L, "_hf_cdiff", c_diff);
	function(L, "_hf_cconvert", c_convert);
	function(L, "_hf_ran1", ran1);
	function(L, "_hf_sran1", seed_ran1);
	function(L, "_hf_gforge", gforge);
	function(L, "_hf_fill", fillarray);
	function(L, "_hf_fft", h_fft);
	function(L, "_hf_rand", gen_rand);
	function(L, "_hf_gaussinit", initgauss);
	function(L, "_hf_gauss", gauss);
	function(L, "_hf_gaussn", gaussn);
	function(L, "_hf_fourfilt", h_fourfilt);
	function(L, "_hf_realfilt", h_realfilt);
	function(L, "_hf_yslope", yslope);
	function(L, "_hf_ghill", h_gauss);
	function(L, "_hf_ring", h_ring);
	function(L, "_hf_crater", h_crater);
	function(L, "_hf_smooth", smooth);
	function(L, "_hf_join", h_join);
	function(L, "_hf_double", h_double);
	function(L, "_hf_half", h_half);
	function(L, "_hf_rescale", rescale);
	function(L, "_hf_clip", h_clip);
	function(L, "_hf_warp", h_warp);
	function(L, "_hf_cwarp", h_cwarp);
	function(L, "_hf_zedge", h_zedge);
	function(L, "_hf_nsmooth", h_nsmooth);
	function(L, "_hf_fillb", h_fillb);
	function(L, "_hf_find_ua", h_find_ua);

	function(L, "_hf_save", save);
	function(L, "_hf_load", load);
}
