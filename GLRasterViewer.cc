// -*- C++ -*-
/*
  GLRasterViewer.cc - a window hosting the GLRasterCanvas widget
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
#include <GL/gl.h>
#include <GL/glu.h>
#include <algorithm>
#include "GLRasterViewer.h"

static char rcsid[] = "$Id: GLRasterViewer.cc,v 1.4.2.5 2004/09/24 17:18:23 zvrba Exp $";

FXDEFMAP(GLRasterViewer) GLRasterViewerMap[] = {
	FXMAPFUNCS(SEL_COMMAND,
			   GLRasterViewer::ID_HSCROLL, GLRasterViewer::ID_VSCROLL,
			   GLRasterViewer::onCmdScroll),
	FXMAPFUNC(SEL_PAINT, 0, GLRasterViewer::onPaint),
	FXMAPFUNC(SEL_CONFIGURE, 0, GLRasterViewer::onConfigure)
};

FXIMPLEMENT(GLRasterViewer, FXPacker, GLRasterViewerMap, ARRAYNUMBER(GLRasterViewerMap))

/**
   Create a GLRasterViewer.

   @param	p	parent
   @param	vis	OpenGL visual
*/
GLRasterViewer::GLRasterViewer(
	FXComposite *p, FXGLVisual *vis,
	FXObject *tgt, FXSelector sel, FXuint opts,
	FXint x, FXint y, FXint w, FXint h) :
	FXPacker(p, opts, x, y, w, h, 0, 0, 0, 0),
	image_(0), xo_(0), yo_(0),
	adjust_contrast_(true)
{
	hscroll_ = new FXScrollbar(
		this, this, ID_HSCROLL,
		SCROLLBAR_HORIZONTAL | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X);
	vscroll_ = new FXScrollbar(
		this, this, ID_VSCROLL,
		SCROLLBAR_VERTICAL | LAYOUT_SIDE_RIGHT | LAYOUT_FILL_Y);
	glc_ = new FXGLCanvas(
		this, vis, 0, 0,
		LAYOUT_CENTER_X | LAYOUT_CENTER_Y | LAYOUT_FILL_X | LAYOUT_FILL_Y,
		0, 0, 0, 0);
}

void GLRasterViewer::create()
{
	FXPacker::create();

	// set up the canvas
	if(!glc_->makeCurrent()) {
		fxerror("GLRasterViewer::create: can't make OpenGL current canvas");
	}
	glDrawBuffer(GL_FRONT_LEFT);
	glClearColor(0, 0, 0, 0);
	glColor3f(0, 1, 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glShadeModel(GL_FLAT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/** @todo: optimize with 3dnow/sse instructions */
template<class Pixel>
static void calculateExtrema(
	const void *data_, unsigned int n,
	float *min, float *max)
{
	Pixel pmin, pmax;
	const Pixel *data = static_cast<const Pixel*>(data_);
	unsigned i;
	
	pmin = pmax = data[0];
	for(i = 1; i < n; i++) {
		if(data[i] < pmin) pmin = data[i];
		if(data[i] > pmax) pmax = data[i];
	}
	*min = pmin; *max = pmax;
}

/**
   Set an image to display. Depending on contrast adjustment setting, it will
   also calculate image brightness extrema.
   
   @param	data	pointer to pixel data
   @param	pixtype	pixel type (one of GL_BYTE, GL_SHORT, etc. constants)
   @param	w,h		image width and height
*/
void GLRasterViewer::setImage(
	const void *data, int pixtype,
	FXuint w, FXuint h)
{
	switch(pixtype) {
	case GL_UNSIGNED_BYTE:
		type_max_ = 255.f;
		if(adjust_contrast_) {
			calculateExtrema<unsigned char>(data, w*h, &min_, &max_);
		}
		else {
			min_ = 0.f; max_ = 255.f;
		}
		break;
	case GL_BYTE:
		type_max_ = 127.f;
		if(adjust_contrast_) {
			calculateExtrema<char>(data, w*h, &min_, &max_);
		}
		else {
			min_ = -128.f; max_ = 127.f;
		}
		break;
	case GL_UNSIGNED_SHORT:
		type_max_ = 65535.f;
		if(adjust_contrast_) {
			calculateExtrema<unsigned short>(data, w*h, &min_, &max_);
		}
		else {
			min_ = 0.f; max_ = 65535.f;
		}
		break;
	case GL_SHORT:
		type_max_ = 32767.f;
		if(adjust_contrast_) {
			calculateExtrema<short>(data, w*h, &min_, &max_);
		}
		else {
			min_ = -32768.f; max_ = 32767.f;
		}
		break;
	case GL_UNSIGNED_INT:
		type_max_ = 4294967295.f;
		if(adjust_contrast_) {
			calculateExtrema<unsigned int>(data, w*h, &min_, &max_);
		}
		else {
			min_ = 0.f; max_ = 4294967295.f;
		}
		break;
	case GL_INT:
		type_max_ = 2147483647.f;
		if(adjust_contrast_) {
			calculateExtrema<int>(data, w*h, &min_, &max_);
		}
		else {
			min_ = -2147483648.f; max_ = 2147483647.f;
		}
		break;
	case GL_FLOAT:
		type_max_ = 1.f;
		if(adjust_contrast_) {
			calculateExtrema<float>(data, w*h, &min_, &max_);
		}
		else {
			min_ = 0.f; max_ = 1.f;
		}
		break;
	default:
		fxerror("GLRasterViewer::setImage: invalid OpenGL image pixel type");
	}

	image_ = data; pixtype_ = pixtype; w_ = w; h_ = h;
	xo_ = yo_ = 0;
	
	hscroll_->setRange(w_-1);
	hscroll_->setPage(glc_->getWidth());
	hscroll_->setPosition(0);
	
	vscroll_->setRange(h_-1);
	vscroll_->setPage(glc_->getHeight());
	vscroll_->setPosition(0);

	update();
}

/** Set X origin. Makes sure not to exceed image boundaries when displayed. */
void GLRasterViewer::setXOrigin(FXint x, bool do_update)
{
	if(image_) {
		FXuint delta = x + glc_->getWidth();
		if(delta > w_) x -= delta - w_;
		xo_ = std::max(x, 0);
		hscroll_->setPosition(xo_);
		if(do_update) update();
	}
}

/** Set Y origin. Makes sure not to exceed image boundaries when displayed. */
void GLRasterViewer::setYOrigin(FXint y, bool do_update)
{
	if(image_) {
		FXuint delta = y + glc_->getHeight();
		if(delta > h_) y -= delta - h_;
		yo_ = std::max(y, 0);
		vscroll_->setPosition(yo_);
		if(do_update) update();
	}
}

long GLRasterViewer::onCmdScroll(FXObject *sender, FXSelector sel, void*)
{
	FXScrollbar *sb = static_cast<FXScrollbar*>(sender);
	FXint pos = sb->getPosition();
	
	if(SELID(sel) == ID_HSCROLL) {
		FXTRACE((5, "onCmdScroll(ID_HSCROLL): %d\n", pos));
		setXOrigin(pos);
		return 1;
	}
	
	if(SELID(sel) == ID_VSCROLL) {
		FXTRACE((5, "onCmdScroll(ID_VSCROLL): %d\n", pos));
		setYOrigin(pos);
		return 1;
	}

	return 0;
}

long GLRasterViewer::onPaint(FXObject*, FXSelector, void*)
{
	int vw = std::min((int)glc_->getWidth(), (int)w_);
	int vh = std::min((int)glc_->getHeight(), (int)h_);
	float factor, bias;

	if(!image_) {
		glClear(GL_COLOR_BUFFER_BIT);
		return 0;
	}
	
	// calculate subimage to render
	glPixelStoref(GL_UNPACK_ROW_LENGTH, w_);
	glPixelStoref(GL_UNPACK_SKIP_ROWS, yo_);
	glPixelStoref(GL_UNPACK_SKIP_PIXELS, xo_);
	
	// adjust transfer function
	if(max_ == min_) {
		fxmessage("GLRasterViewer::onPaint: constant image %f; no adjustment done\n", max_);
		factor = 1; bias = 0;
	} else {
		factor = type_max_ / (max_ - min_); bias = -min_ / (max_ - min_);
	}
	glPixelTransferf(GL_RED_SCALE, factor); glPixelTransferf(GL_RED_BIAS, bias);
	glPixelTransferf(GL_GREEN_SCALE, factor); glPixelTransferf(GL_GREEN_BIAS, bias);
	glPixelTransferf(GL_BLUE_SCALE, factor); glPixelTransferf(GL_BLUE_BIAS, bias);
	glDrawPixels(vw, vh, GL_LUMINANCE, pixtype_, image_);
	
	return 1;
}

long GLRasterViewer::onConfigure(FXObject*, FXSelector, void*)
{
	int w = glc_->getWidth(), h = glc_->getHeight();
		
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w-1, 0, h-1);
	glRasterPos2f(0, 0);

	FXTRACE((5, "onConfigure: %d %d\n", w, h));
	hscroll_->setPage(w);
	vscroll_->setPage(h);

	setOrigin(0, 0);

	return 1;
}
