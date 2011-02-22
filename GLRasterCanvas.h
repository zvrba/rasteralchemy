// -*- C++ -*-
// $Id: GLRasterCanvas.h,v 1.1.2.3 2004/09/24 17:18:23 zvrba Exp $
/*
  GLRasterCanvas.h - OpenGL complex image display widget
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
#ifndef GLRASTERCANVAS_H__
#define GLRASTERCANVAS_H__

#include <fox/fx.h>
#include <fox/FXGLVisual.h>
#include <fox/FXGLCanvas.h>

/**
   A canvas widget to display raster images with OpenGL. It features:
   - Images larger than display area.
   - Display of monochrome images 8-32 bit (un)signed integer and 32-bit
     floating-point images.
   - Display of complex images. Real and imaginary parts of complex image are
     laid out in memory consecutively: first real part, then imaginary part.
	 There are several modes:
	 - real or imaginary part only
	 - magnitude or phase (-PI .. PI) only
	 - color-coded display in HSI mode: one component is constant, other vary
	   according to (mag,phase) or (re,im)
	 - color-coded display in RGB mode

   @todo Color images.
*/
class GLRasterCanvas : public FXGLCanvas {
	FXDECLARE(GLRasterCanvas)
public:
	GLRasterCanvas(FXComposite*, FXGLVisual*,
				   FXObject* = 0, FXSelector = 0, FXuint = 0,
				   FXint = 0, FXint = 0, FXint = 0, FXint = 0);
	GLRasterCanvas(FXComposite*, FXGLVisual*, FXGLCanvas*, 
				   FXObject* = 0, FXSelector = 0, FXuint = 0,
				   FXint = 0, FXint = 0, FXint = 0, FXint = 0);
	virtual ~GLRasterCanvas();

	// FOX
	virtual void create();
	virtual FXint getDefaultWidth();
	virtual FXint getDefaultHeight();

	long onPaint(FXObject*, FXSelector, void*);
	long onConfigure(FXObject*, FXSelector, void*);

	// basic image manipulation: image setting, display origin
	void setImage(FXint, FXuint, FXuint, const void*, const void* = 0);
	void setXOrigin(FXint, bool = true);
	void setYOrigin(FXint, bool = true);
	void setOrigin(FXint, FXint, bool = true);
	void getOrigin(FXint *ox, FXint *oy) const {
		*ox = xo_; *oy = yo_;
	}

	// display modes
	enum DisplayMode {
		// display modes
		DISPLAY_CONTRAST1 = 0x01, // all grayscale images and 1st channel
		DISPLAY_CONTRAST2 = 0x02, // only 2nd channel of color images
		DISPLAY_POLAR = 0x04,	// (mag,phase) instead of (re,im) image
		DISPLAY_MASK = 0x0F,

		// one of; 4 bits
		CPLX_CH1 = 0x00,		// first channel; either RE or MAG
		CPLX_CH2 = 0x10,		// second channel; either IM or PHI
		CPLX_HS = 0x20,			// h=re, s=im, v=const.
		CPLX_HV = 0x30,			// others are analogous..
		CPLX_SV = 0x40,
		CPLX_SH = 0x50,			// s=re, h=im, v=const.
		CPLX_VH = 0x60,
		CPLX_VS = 0x70,
		CPLX_RG = 0x80,
		CPLX_RB = 0x90,
		CPLX_GB = 0xA0,
		CPLX_GR = 0xB0,
		CPLX_BR = 0xC0,
		CPLX_BG = 0xD0,
		CPLX_MASK = 0xF0,
	};

	void setDisplayMode(FXuint val, FXuint mask = -1U);
	FXuint getDisplayMode() const;
	void setConstant(FXfloat c);
	FXfloat getConstant() const;

protected:
	GLRasterCanvas() {}
	GLRasterCanvas(const GLRasterCanvas&) {} 

private:
	const void *re_, *im_;
	unsigned int w_, h_;
	int	pixtype_;

	unsigned int disp_mode_;
	float color_const_;
	int xo_, yo_;

	float *aux_im_;
	float min_[2], max_[2];
	float factor_[2], bias_[2];
	const void *disp_im_;
	bool preprocess_stale_;

	void preprocess();
};

#endif // GLRASTERCANVAS_H__
