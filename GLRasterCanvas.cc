// -*- C++ -*-
/*
  GLRasterCanvas.cc - OpenGL complex image display widget
  Copyright (C) 2004 Zeljko Vrba

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

  You can reach me at the following e-mail addresses:

  zvrba@globalnet.hr
  mordor@fly.srk.fer.hr
*/
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "GLRasterCanvas.h"

#include <algorithm>

static char rcsid[] = "$Id: GLRasterCanvas.cc,v 1.1.2.10 2004/09/24 17:18:23 zvrba Exp $";

FXDEFMAP(GLRasterCanvas) GLRasterCanvasMap[] = {
	FXMAPFUNC(SEL_PAINT, 0, GLRasterCanvas::onPaint),
	FXMAPFUNC(SEL_CONFIGURE, 0, GLRasterCanvas::onConfigure)
};

FXIMPLEMENT(GLRasterCanvas, FXGLCanvas, GLRasterCanvasMap,
			ARRAYNUMBER(GLRasterCanvasMap))

/******************************************************************************
 * FOX
 *****************************************************************************/

/** Constructor. All parameters are the same as in FXGLCanvas. */
GLRasterCanvas::GLRasterCanvas(
	FXComposite *p, FXGLVisual *vis, FXObject *tgt, FXSelector sel,
	FXuint opts, FXint x, FXint y, FXint w, FXint h)
	: FXGLCanvas(p, vis, tgt, sel, opts, x, y, w, h),
	  re_(0), im_(0), w_(0), h_(0), pixtype_(-1),
	  disp_mode_(0), color_const_(0), xo_(0), yo_(0),
	  aux_im_(0), disp_im_(0), preprocess_stale_(true)
{
}

GLRasterCanvas::GLRasterCanvas(
	FXComposite *p, FXGLVisual *vis, FXGLCanvas *sharegroup,
	FXObject *tgt, FXSelector sel, FXuint opts,
	FXint x, FXint y, FXint w, FXint h)
	: FXGLCanvas(p, vis, sharegroup, tgt, sel, opts, x, y, w, h),
	  re_(0), im_(0), w_(0), h_(0), pixtype_(-1),
	  disp_mode_(0), color_const_(0), xo_(0), yo_(0),
	  aux_im_(0), disp_im_(0), preprocess_stale_(true)
{
}

GLRasterCanvas::~GLRasterCanvas()
{
	delete[] aux_im_;
	aux_im_ = 0;
}

void GLRasterCanvas::create()
{
	FXGLCanvas::create();

	if(makeCurrent()) {
		glDrawBuffer(GL_FRONT_LEFT);
		glClearColor(0, 0, 0, 0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		makeNonCurrent();
	}
}

// OpenGL has specific types so we can't use std::numeric_limits which may
// vary according to platform and should not be expected to match OpenGL's.
static void getGLTypeRange(int pixtype, float *min, float *max)
{
	switch(pixtype) {
	case GL_UNSIGNED_BYTE: *min = 0.f; *max = 255.f; break;
	case GL_BYTE: *min = -128.f; *max = 127.f; break;
	case GL_UNSIGNED_SHORT: *min = 0.f; *max = 65535.f; break;
	case GL_SHORT: *min = -32768.f; *max = 32767.f; break;
	case GL_UNSIGNED_INT: *min = 0.f; *max = 4294967295.f; break;
	case GL_INT: *min = -2147483648.f; *max = 2147483647.f; break;
	case GL_FLOAT: *min = 0.f; *max = 1.f; break;
	default:
		fxerror("GLRasterCanvas::getGLTypeRange: invalid OpenGL image pixel type.");
	}
}

static void setGLTransfer(float factor, float bias)
{
	glPixelTransferf(GL_RED_SCALE, factor);
	glPixelTransferf(GL_RED_BIAS, bias);
	
	glPixelTransferf(GL_GREEN_SCALE, factor);
	glPixelTransferf(GL_GREEN_BIAS, bias);
	
	glPixelTransferf(GL_BLUE_SCALE, factor);
	glPixelTransferf(GL_BLUE_BIAS, bias);
}

long GLRasterCanvas::onPaint(FXObject*, FXSelector, void*)
{
	int vw = std::min(getWidth(), (int)w_);
	int vh = std::min(getHeight(), (int)h_);
	int format, pixtype;

	if(!makeCurrent()) return 0;
	preprocess();

	// calculate subimage to render
	glPixelStoref(GL_UNPACK_ROW_LENGTH, w_);
	glPixelStoref(GL_UNPACK_SKIP_ROWS, yo_);
	glPixelStoref(GL_UNPACK_SKIP_PIXELS, xo_);

	if(im_) {
		format = (disp_mode_ & CPLX_MASK) > CPLX_CH2 ? GL_RGB : GL_LUMINANCE;
		if(((disp_mode_ & CPLX_MASK) > CPLX_CH2) ||
		   (disp_mode_ & DISPLAY_POLAR)) {
			pixtype = GL_FLOAT;
			// preprocessed images are already scaled as necessary
			setGLTransfer(1, 0);
		} else {
			pixtype = pixtype_;
			if(disp_mode_ & (DISPLAY_CONTRAST1 | CPLX_CH1) == 
			   DISPLAY_CONTRAST1 | CPLX_CH1) setGLTransfer(factor_[0], bias_[0]);
			else if(disp_mode_ & (DISPLAY_CONTRAST2 | CPLX_CH2) ==
					DISPLAY_CONTRAST2 | CPLX_CH2) setGLTransfer(factor_[1], bias_[1]);
			else setGLTransfer(1, 0);
		}
	} else if(re_) {
		format = GL_LUMINANCE;
		pixtype = pixtype_;
		if(disp_mode_ & (DISPLAY_CONTRAST1 | DISPLAY_CONTRAST2)) setGLTransfer(factor_[0], bias_[0]);
		else setGLTransfer(1, 0);
	} else {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glDrawPixels(vw, vh, format, pixtype, disp_im_);
	makeNonCurrent();
	return 1;
}

long GLRasterCanvas::onConfigure(FXObject*, FXSelector, void*)
{
	FXint w = getWidth(), h = getHeight();

	if(makeCurrent()) {
		glViewport(0, 0, w, h);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, w-1, 0, h-1);
		glRasterPos2f(0, 0);
		setOrigin(0, 0);
		makeNonCurrent();
	}
	return 1;
}

FXint GLRasterCanvas::getDefaultWidth() {
	return w_;
}

FXint GLRasterCanvas::getDefaultHeight() {
	return h_;
}

/******************************************************************************
 * IMAGE OPS
 *****************************************************************************/

/**
   Set an image to display. Depending on contrast adjustment setting, it will
   also calculate image brightness extrema. This function forces redraw.
   
   @param	pixtype	pixel type (one of GL_BYTE, GL_SHORT, etc. constants)
   @param	w,h		image width and height
   @param	re,im	pointer to real and imaginary part of pixel data
*/
void GLRasterCanvas::setImage(
	FXint pixtype, FXuint w, FXuint h,
	const void *re, const void *im)
{
	if(im_ && !re_) fxerror("GLRasterCanvas::setImage: can't set only imaginary part.");
	pixtype_ = pixtype; w_ = w; h_ = h; re_ = re; im_ = im;
	xo_ = yo_ = 0;

	preprocess_stale_ = true;
	handle(this, MKUINT(0, SEL_PAINT), 0);
}

/** Set X origin. Makes sure not to exceed image boundaries when displayed. */
void GLRasterCanvas::setXOrigin(FXint x, bool do_update)
{
	if(re_) {
		FXuint delta = x + getWidth();
		if(delta > w_) x -= delta - w_;
		xo_ = std::max(x, 0);

		if(do_update) handle(this, MKUINT(0, SEL_PAINT), 0);
	}
}

/** Set Y origin. Makes sure not to exceed image boundaries when displayed. */
void GLRasterCanvas::setYOrigin(FXint y, bool do_update)
{
	if(re_) {
		FXuint delta = y + getHeight();
		if(delta > h_) y -= delta - h_;
		yo_ = std::max(y, 0);

		if(do_update) handle(this, MKUINT(0, SEL_PAINT), 0);
	}
}

void GLRasterCanvas::setOrigin(FXint x, FXint y, bool do_update)
{
	if(re_) {
		setXOrigin(x, false);
		setYOrigin(y, false);

		if(do_update) handle(this, MKUINT(0, SEL_PAINT), 0);
	}
}


void GLRasterCanvas::setDisplayMode(FXuint val, FXuint mask) {
	disp_mode_ = (disp_mode_ & ~mask) | val;
	preprocess_stale_ = true;
}

FXuint GLRasterCanvas::getDisplayMode() const {
	return disp_mode_;
}


void GLRasterCanvas::setConstant(FXfloat c) {
	color_const_ = c;
	preprocess_stale_ = true;
}

FXfloat GLRasterCanvas::getConstant() const {
	return color_const_;
}

/******************************************************************************
 * PREPROCESSING
 *****************************************************************************/

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
// Taken and adapted from Imath
///////////////////////////////////////////////////////////////////////////
static void hsv2rgb(float *rgb, const float *hsv)
{
#define s hsv[1]
#define v hsv[2]

	float h = hsv[0];
    float x = 0.0, y = 0.0, z = 0.0;
    
    if (h == 1) h = 0;
    else h *= 6;

    int i = (int)floor(h);
    float f = h-i;
    float p = v*(1-s);
    float q = v*(1-(s*f));
    float t = v*(1-(s*(1-f)));

    switch (i) 
    {
      case 0: x = v; y = t; z = p; break;
      case 1: x = q; y = v; z = p; break;
      case 2: x = p; y = v; z = t; break;
      case 3: x = p; y = q; z = v; break;
      case 4: x = t; y = p; z = v; break;
      case 5: x = v; y = p; z = q; break;
    }

	rgb[0] = x; rgb[1] = y; rgb[2] = z;
}

/**
   Coordinate transformation class. Takes as input (re,im) of complex image
   and returns transformed coordinates. This is the first operation in chain of
   transformations so it takes two arguments (re,im) and returns a const
   float*.

   This class is identity: leaves rectangular coordinates as is.
*/
class rect {					// first op in chain
	float p_[2];

public:
	const float *operator()(float re, float im) {
		p_[0] = re; p_[1] = im;
		return p_;
	}
};

/**
   Coordinate transformation class. Takes as input (re,im) of complex image
   and returns transformed coordinates. This is the first operation in chain of
   transformations so it takes two arguments (re,im) and returns a const
   float*.

   This class takes rectangular coordinates and returns polar.
*/
class polar { // first op in chain
	float p_[2];
	
public:
	const float *operator()(float re, float im) {
		p_[0] = hypot(re, im);
		p_[1] = atan2(im, re);
		return p_;
	}
};

/**
   Scaling class. Takes as input 2 channels of complex image and returns 3
   channels (scaled/clamped values) and a 3rd constant channel. Constant
   value is specified in constructor.
*/
class scale {
	float factor_[2], bias_[2];
	float ch_[3];
	bool scale_ch_[2];

	void clamp(int);

public:
	scale(float*, float*, float, bool, bool);
	const float *operator()(const float *cplx) {
		ch_[0] = cplx[0]; ch_[1] = cplx[1];
		clamp(0); clamp(1);
		return ch_;
	}
};

scale::scale(
	float *factor, float *bias, float color_const,
	bool scale0, bool scale1)
{
	factor_[0] = factor[0]; factor_[1] = factor[1];
	bias_[0] = bias[0]; bias_[1] = bias[1];
	scale_ch_[0] = scale0; scale_ch_[1] = scale1;
	if(color_const < 0) color_const = 0;
	else if(color_const > 1) color_const = 1;
	ch_[2] = color_const;
}

void scale::clamp(int ch)
{
	if(scale_ch_[ch]) ch_[ch] = ch_[ch] * factor_[ch] + bias_[ch];
	else if(ch_[ch] < 0) ch_[ch] = 0;
	else if(ch_[ch] > 1) ch_[ch] = 1;
}

/** Color transformation class. This is identity, i.e. leaves RGB as is. */
struct rgb {
	const float *operator()(const float *cplx) {
		return cplx;
	}
};

/**
   Color transformation class. Transformes HSV (all components scaled or
   clamped to [0,1]) to RGB values.
*/
class hsv {
	float rgb_[3];
public:
	const float *operator()(const float *cplx) {
		hsv2rgb(rgb_, cplx);
		return rgb_;
	}
};

/**
   Permutation class. Takes as input 3 channels: 2 channels of complex image
   and 3rd constant channel (0: re, 1: im, 2: const.) and stores them along
   3 channels of color space.

   Template argument is the number of output channels. In some cases we have
   only one output channel (e.g. magnitude or phase display only) and in others
   3 output channels (color output).

   This is the last class in chain of pixel transformations. It does the
   actual output to destination pixels. Application operator returns the
   number of pixels written (either 1 or 3).
*/
template<typename CoordXform>
class permute_fun1 {			// last op in chain
	CoordXform coordxf_;
	scale scalexf_;
	int ch_;
public:
	permute_fun1(int ch, CoordXform xf, scale sxf) :
		coordxf_(xf), scalexf_(sxf), ch_(ch) { }
	int operator()(float *dst, float re, float im) {
		dst[0] = scalexf_(coordxf_(re, im))[ch_];
		return 1;
	}
};

template<typename CoordXform, typename ColorXform>
class permute_fun3 {
	CoordXform coordxf_;
	ColorXform colorxf_;
	scale scalexf_;
	float col_[3];
	int perm_[3];
	
public:
	permute_fun3(int ch0, int ch1, int ch2,
				 CoordXform cdxf, scale sxf, ColorXform clxf)
		: coordxf_(cdxf), colorxf_(clxf), scalexf_(sxf)
	{
		perm_[0] = ch0; perm_[1] = ch1; perm_[2] = ch2;
	}
	int operator()(float *dst, float re, float im) {
		// permutation should be done BEFORE color transformation!
		const float *tmp  = scalexf_(coordxf_(re, im));
		col_[0] = tmp[perm_[0]];
		col_[1] = tmp[perm_[1]];
		col_[2] = tmp[perm_[2]];

		const float *result = colorxf_(col_);
		dst[0] = result[0]; dst[1] = result[1]; dst[2] = result[2];
		return 3;
	}
};

template<typename CoordXform>
static inline permute_fun1<CoordXform> permute(
	int ch, CoordXform xf, scale sxf)
{
	return permute_fun1<CoordXform>(ch, xf, sxf);
}

template<typename CoordXform, typename ColorXform>
static inline permute_fun3<CoordXform, ColorXform>
permute(int ch0, int ch1, int ch2, CoordXform cdxf, scale sxf, ColorXform clxf)
{
	return permute_fun3<CoordXform, ColorXform>(ch0, ch1, ch2, cdxf, sxf, clxf);
}

// min/max of image components in TRANSFORMED coordinate system. min/max
// are arrays of at least 2 elements to hold min/max for each component.
template<typename Pixel, typename CoordXform>
static void minmax(
	CoordXform xf,
	const void *re_, const void *im_, unsigned int n,
	float *min, float *max)
{
	const Pixel *re = static_cast<const Pixel*>(re_);
	const Pixel *im = static_cast<const Pixel*>(im_);

	const float *result = xf(re[0], im[0]);
	min[0] = max[0] = result[0];
	min[1] = max[1] = result[0];
	
	for(unsigned int i = 1; i < n; i++) {
		result = xf(re[i], im[i]);
		min[0] = std::min(min[0], result[0]);
		max[0] = std::max(max[0], result[0]);
		min[1] = std::min(min[1], result[1]);
		max[1] = std::max(max[1], result[1]);
	}
}

template<typename CoordXform>
static void minmax(
	int pixtype, const void *re_, const void *im_, unsigned int n,
	float *min, float *max, CoordXform xf)
{
	switch(pixtype) {
	case GL_UNSIGNED_BYTE: return minmax<unsigned char>(xf, re_, im_, n, min, max);
	case GL_BYTE: return minmax<char>(xf, re_, im_, n, min, max);
	case GL_UNSIGNED_SHORT: return minmax<unsigned short>(xf, re_, im_, n, min, max);
	case GL_SHORT: return minmax<short>(xf, re_, im_, n, min, max);
	case GL_UNSIGNED_INT: return minmax<unsigned int>(xf, re_, im_, n, min, max);
	case GL_INT: return minmax<int>(xf, re_, im_, n, min, max);
	case GL_FLOAT: return minmax<float>(xf, re_, im_, n, min, max);
	}
}

// transform source image to destination image. Xform is any composition of
// above transformations. Xform should take TWO (re,im) parameters and should
// return the number of values written (currently, this is either 1 or 3).
template<typename Pixel, typename Xform>
static void calculate(
	Xform xf, const void *re_, const void *im_, unsigned int n, float *dst)
{
	const Pixel *re = static_cast<const Pixel*>(re_);
	const Pixel *im = static_cast<const Pixel*>(im_);

	for(unsigned int i = 0; i < n; i++) dst += xf(dst, re[i], im[i]);
}

template<typename Xform>
static void calculate(
	int pixtype, const void *re_, const void *im_, unsigned int n, float *dst,
	Xform xf)
{
	switch(pixtype) {
	case GL_UNSIGNED_BYTE: return calculate<unsigned char>(xf, re_, im_, n, dst);
	case GL_BYTE: return calculate<char>(xf, re_, im_, n, dst);
	case GL_UNSIGNED_SHORT: return calculate<unsigned short>(xf, re_, im_, n, dst);
	case GL_SHORT: return calculate<short>(xf, re_, im_, n, dst);
	case GL_UNSIGNED_INT: return calculate<unsigned int>(xf, re_, im_, n, dst);
	case GL_INT: return calculate<int>(xf, re_, im_, n, dst);
	case GL_FLOAT: return calculate<float>(xf, re_, im_, n, dst);
	}
}

static void fb(float min, float max, float type_max, float *factor, float *bias)
{
	float d = max - min;
	
	if(d == 0.0) {
		fxmessage("GLRasterCanvas: can't contrast constant channel of value %f; setting value to 0.5\n", max);
		*factor = 0; *bias = 0.5;
	} else {
		*factor = type_max / d; *bias = -min / d;
	}
}

#define HSV(h, s, v)\
  if(is_polar) calculate(pixtype_, re_, im_, sz, aux_im_, permute(h, s, v, polar(), sxf, hsv()));\
  else calculate(pixtype_, re_, im_, sz, aux_im_, permute(h, s, v, rect(), sxf, hsv()));

#define RGB(r, g, b)\
  if(is_polar) calculate(pixtype_, re_, im_, sz, aux_im_, permute(r, g, b, polar(), sxf, rgb()));\
  else calculate(pixtype_, re_, im_, sz, aux_im_, permute(r, g, b, rect(), sxf, rgb()));

/**
   Image preprocessing. For real images just computes min/max, for complex
   images does more extensive preprocessing and stores the result in a
   byte-valued RGB image.
*/
void GLRasterCanvas::preprocess()
{
	float type_min, type_max;
	unsigned int sz = w_ * h_;

	if(!preprocess_stale_) return;
	delete[] aux_im_; aux_im_ = 0;
	
	FXTRACE((4, "GLRasterCanvas::preprocess: display mode=%02x\n", disp_mode_));
	if(im_) {					// complex image
		bool is_polar = disp_mode_ & DISPLAY_POLAR;

		if((disp_mode_ & CPLX_MASK) > CPLX_CH2) {
			FXTRACE((4, "GLRasterCanvas::preprocess: allocating3 %u\n", sz*3));
			aux_im_ = new float[sz*3];
		} else if(disp_mode_ & DISPLAY_POLAR) {
			FXTRACE((4, "GLRasterCanvas::preprocess: allocating1 %u\n", sz));
			aux_im_ = new float[sz];
		}
		disp_im_ = aux_im_;

		FXTRACE((4, "GLRasterCanvas::preprocess: aux_im=%p\n", aux_im_));

		if(is_polar) minmax(pixtype_, re_, im_, sz, min_, max_, polar());
		else minmax(pixtype_, re_, im_, sz, min_, max_, rect());
		FXTRACE((4, "GLRasterCanvas::preprocess: minmax RE=(%f,%f) IM=(%f,%f)\n",
				 min_[0], max_[0], min_[1], max_[1]));

		getGLTypeRange(pixtype_, &type_min, &type_max);
		fb(min_[0], max_[0], type_max, factor_, bias_);
		fb(min_[1], max_[1], type_max, factor_+1, bias_+1);
		scale sxf(factor_, bias_, color_const_,
				  disp_mode_ & DISPLAY_CONTRAST1,
				  disp_mode_ & DISPLAY_CONTRAST2);

		switch(disp_mode_ & CPLX_MASK) {
		case CPLX_CH1:
			if(disp_mode_ & DISPLAY_POLAR) {
				calculate(pixtype_, re_, im_, sz, aux_im_,
						  permute(0, polar(), sxf));
			} else {
				disp_im_ = re_;
			}
			break;
		case CPLX_CH2:
			if(disp_mode_ & DISPLAY_POLAR) {
				calculate(pixtype_, re_, im_, sz, aux_im_,
						  permute(1, polar(), sxf));
			} else {
				disp_im_ = im_;
			}
			break;
		case CPLX_HS: HSV(0, 1, 2); break;
		case CPLX_HV: HSV(1, 0, 2); break;
		case CPLX_SV: HSV(0, 2, 1); break;
		case CPLX_SH: HSV(1, 2, 0); break;
		case CPLX_VH: HSV(2, 0, 1); break;
		case CPLX_VS: HSV(2, 1, 0); break;
		case CPLX_RG: RGB(0, 1, 2); break;
		case CPLX_RB: RGB(1, 0, 2); break;
		case CPLX_GB: RGB(0, 2, 1); break;
		case CPLX_GR: RGB(1, 2, 0); break;
		case CPLX_BR: RGB(2, 0, 1); break;
		case CPLX_BG: RGB(2, 1, 0); break;
		default:
			fxerror("GLRasterCanvas::preprocess: invalid CPLX mode");
		}
	} else if(re_) {			// real image
		if(disp_mode_ & (DISPLAY_CONTRAST1 | DISPLAY_CONTRAST2)) {
			minmax(pixtype_, re_, re_, sz, min_, max_, rect());
			getGLTypeRange(pixtype_, &type_min, &type_max);
			fb(min_[0], max_[0], type_max, factor_, bias_);
		}
		disp_im_ = re_;
	} else {					// no image
		disp_im_ = 0;
	}
	preprocess_stale_ = false;
}
#undef HSV
#undef RGB
