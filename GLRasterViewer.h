// -*- C++ -*-
// $Id: GLRasterViewer.h,v 1.3.2.2 2004/09/24 17:18:23 zvrba Exp $
/*
  GLRasterViewer.h - a window hosting the GLRasterCanvas widget
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
#ifndef GLRASTERVIEWER_H__
#define GLRASTERVIEWER_H__

#include <fox/fx.h>
#include <fox/FXGLVisual.h>
#include <fox/FXGLCanvas.h>

/**
   A widget to display raster images with OpenGL. It features two scrollbars
   which enable scrolling the image.

   @todo
   - rectangle selections
   - display of current cursor coordinate

   @note We can't sublcass from FXScrollArea because GLCanvas widget always
   has the same size and never grows larger than displayed window.
*/
class GLRasterViewer : public FXPacker {
	FXDECLARE(GLRasterViewer)
public:
	GLRasterViewer(FXComposite*, FXGLVisual*,
				   FXObject* = 0, FXSelector = 0, FXuint = 0,
				   FXint = 0, FXint = 0, FXint = 0, FXint = 0);
	virtual ~GLRasterViewer() { }

	virtual void create();

	void setImage(const void*, int, FXuint, FXuint);
	void setXOrigin(FXint, bool = true);
	void setYOrigin(FXint, bool = true);

	void setOrigin(FXint x, FXint y, bool do_update = true) {
		setXOrigin(x, false);
		setYOrigin(y, false);
		if(do_update) update();
	}

	void getOrigin(FXint *ox, FXint *oy) const {
		*ox = xo_; *oy = yo_;
	}

	FXGLCanvas *getCanvas() const {
		return glc_;
	}

	bool getContrastAdjustment() const {
		return adjust_contrast_;
	}

	void setContrastAdjustment(bool a) {
		adjust_contrast_ = a;
		setImage(image_, pixtype_, w_, h_);
	}

	enum {
		ID_HSCROLL = FXPacker::ID_LAST,
		ID_VSCROLL,
		ID_LAST
	};

	long onCmdScroll(FXObject*, FXSelector, void*);
	long onPaint(FXObject*, FXSelector, void*);
	long onConfigure(FXObject*, FXSelector, void*);

protected:
	GLRasterViewer() {}
	GLRasterViewer(const GLRasterViewer&) {} 

private:
	FXGLCanvas	*glc_;
	FXScrollbar	*hscroll_, *vscroll_;

	void calcExtrema();

	const void *image_;
	int		pixtype_;
	FXuint	w_, h_;
	FXfloat	min_, max_, type_max_;
	FXint	xo_, yo_;

	bool adjust_contrast_;
};

#endif // GLRASTERVIEWER_H__
