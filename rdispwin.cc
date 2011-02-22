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
#include <fox/fx.h>
#include <fox/fxkeys.h>
#include <fox/FXGLVisual.h>
#include <fox/FXGLCanvas.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <unistd.h>
#include "rdispwin.h"
#include "hf-hl.h"

static char rcsid[] = "$Id: rdispwin.cc,v 1.1.2.12 2004/09/24 17:18:23 zvrba Exp $";

extern FXApp *RasterAlchemyApplication; 

FXDEFMAP(RasterDisplayWindow) RasterDisplayWindowMap[] = {
	FXMAPFUNCS(SEL_COMMAND, RasterDisplayWindow::ID_CONTRAST1, RasterDisplayWindow::ID_POLAR, RasterDisplayWindow::onCmdDisplay),
	FXMAPFUNCS(SEL_UPDATE, RasterDisplayWindow::ID_CONTRAST1, RasterDisplayWindow::ID_POLAR, RasterDisplayWindow::onUpdDisplay),
	FXMAPFUNCS(SEL_COMMAND, RasterDisplayWindow::ID_COLOR, RasterDisplayWindow::ID_COLOR+13, RasterDisplayWindow::onCmdColor),
	FXMAPFUNCS(SEL_UPDATE, RasterDisplayWindow::ID_COLOR, RasterDisplayWindow::ID_COLOR+13, RasterDisplayWindow::onUpdColor),
	FXMAPFUNCS(SEL_COMMAND, RasterDisplayWindow::ID_HSCROLL, RasterDisplayWindow::ID_VSCROLL, RasterDisplayWindow::onCmdScroll),
	FXMAPFUNC(SEL_CONFIGURE, 0, RasterDisplayWindow::onConfigure),
	FXMAPFUNC(SEL_COMMAND, RasterDisplayWindow::ID_ABOUT, RasterDisplayWindow::onCmdAbout),
	FXMAPFUNC(SEL_COMMAND, RasterDisplayWindow::ID_VALUE, RasterDisplayWindow::onCmdValue),
	FXMAPFUNC(SEL_IO_READ, 0, RasterDisplayWindow::onSocketMsg)
};

FXIMPLEMENT(RasterDisplayWindow, FXTopWindow, RasterDisplayWindowMap, ARRAYNUMBER(RasterDisplayWindowMap));

RasterDisplayWindow::RasterDisplayWindow(FXApp *parent, const char *name) :
	FXTopWindow(parent, name, 0, 0, DECOR_ALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
{
	// menus
	FXMenubar *mb = new FXMenubar(
		this, this, FRAME_RAISED | LAYOUT_SIDE_TOP | LAYOUT_FILL_X);
	
	FXMenuPane *dsp = new FXMenuPane(mb);
	new FXMenuCommand(dsp, "Contrast channel &1", 0, this, ID_CONTRAST1);
	new FXMenuCommand(dsp, "Contrast channel &2", 0, this, ID_CONTRAST2);
	new FXMenuSeparator(dsp);
	new FXMenuCommand(dsp, "&Rectangular", 0, this, ID_RECT);
	new FXMenuCommand(dsp, "&Polar", 0, this, ID_POLAR);
	new FXMenuTitle(mb, "Display", 0, dsp);

	FXMenuPane *cplx = new FXMenuPane(mb);
	new FXMenuCommand(cplx, "Channel &1 (RE/MAG)", 0, this, ID_COLOR+0);
	new FXMenuCommand(cplx, "Channel &2 (IM/PHI)", 0, this, ID_COLOR+1);
	FXMenuPane *rgb = new FXMenuPane(mb);
	FXMenuPane *hsv = new FXMenuPane(mb);
	new FXMenuSeparator(cplx);
	new FXMenuCommand(cplx, "Constant channel value...", 0, this, ID_VALUE);
	new FXMenuCascade(cplx, "RGB", 0, rgb);
	new FXMenuCascade(cplx, "HSV", 0, hsv);
	new FXMenuTitle(mb, "Complex", 0, cplx);

	// The order must be as in DisplayMode enum
	new FXMenuCommand(hsv, "(0,1,2)", 0, this, ID_COLOR+2);
	new FXMenuCommand(hsv, "(1,0,2)", 0, this, ID_COLOR+3);
	new FXMenuCommand(hsv, "(0,2,1)", 0, this, ID_COLOR+4);
	new FXMenuCommand(hsv, "(1,2,0)", 0, this, ID_COLOR+5);
	new FXMenuCommand(hsv, "(2,0,1)", 0, this, ID_COLOR+6);
	new FXMenuCommand(hsv, "(2,1,0)", 0, this, ID_COLOR+7);
	new FXMenuCommand(rgb, "(0,1,2)", 0, this, ID_COLOR+8);
	new FXMenuCommand(rgb, "(1,0,2)", 0, this, ID_COLOR+9);
	new FXMenuCommand(rgb, "(0,2,1)", 0, this, ID_COLOR+10);
	new FXMenuCommand(rgb, "(1,2,0)", 0, this, ID_COLOR+11);
	new FXMenuCommand(rgb, "(2,0,1)", 0, this, ID_COLOR+12);
	new FXMenuCommand(rgb, "(2,1,0)", 0, this, ID_COLOR+13);

	FXMenuPane *help = new FXMenuPane(mb);
	new FXMenuCommand(help, "&About", 0, this, ID_ABOUT);
	new FXMenuTitle(mb, "&Help", 0, help, LAYOUT_RIGHT);

	hscroll_ = new FXScrollbar(
		this, this, ID_HSCROLL,
		SCROLLBAR_HORIZONTAL | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X);
	vscroll_ = new FXScrollbar(
		this, this, ID_VSCROLL,
		SCROLLBAR_VERTICAL | LAYOUT_SIDE_RIGHT | LAYOUT_FILL_Y);
	glc_ = new GLRasterCanvas(
		this, new FXGLVisual(parent, 0), 0, 0,
		LAYOUT_CENTER_X | LAYOUT_CENTER_Y | LAYOUT_FILL_X | LAYOUT_FILL_Y,
		0, 0, 0, 0);
}

long RasterDisplayWindow::onCmdDisplay(FXObject *sender, FXSelector sel, void*)
{
	FXuint mode = glc_->getDisplayMode();

	switch(SELID(sel)) {
	case ID_CONTRAST1:
		mode ^= GLRasterCanvas::DISPLAY_CONTRAST1;
		break;
	case ID_CONTRAST2:
		mode ^= GLRasterCanvas::DISPLAY_CONTRAST2;
		break;
	case ID_RECT:
		mode &= ~GLRasterCanvas::DISPLAY_POLAR;
		break;
	case ID_POLAR:
		mode |= GLRasterCanvas::DISPLAY_POLAR;
		break;
	default:
		return 0;
	}

	glc_->setDisplayMode(mode);
	return 1;
}

long RasterDisplayWindow::onUpdDisplay(FXObject *sender, FXSelector sel, void*)
{
	FXuint mode = glc_->getDisplayMode();
	FXMenuCommand *obj = static_cast<FXMenuCommand*>(sender);
	
	switch(SELID(sel)) {
	case ID_CONTRAST1:
		mode & GLRasterCanvas::DISPLAY_CONTRAST1 ?
			obj->check() : obj->uncheck();
		return 1;
	case ID_CONTRAST2:
		mode & GLRasterCanvas::DISPLAY_CONTRAST2 ?
			obj->check() : obj->uncheck();
		return 1;
	case ID_RECT:
		mode & GLRasterCanvas::DISPLAY_POLAR ?
			obj->uncheck() : obj->check();
		return 1;
	case ID_POLAR:
		mode & GLRasterCanvas::DISPLAY_POLAR ?
			obj->check() : obj->uncheck();
		return 1;
	}
	
	return 0;
}

long RasterDisplayWindow::onCmdColor(FXObject*, FXSelector sel, void*)
{
	FXuint mode = ((SELID(sel) - ID_COLOR) << 4);
	glc_->setDisplayMode(mode, GLRasterCanvas::CPLX_MASK);
	return 1;
}

long RasterDisplayWindow::onUpdColor(FXObject *sender, FXSelector sel, void*)
{
	FXMenuCommand *obj = static_cast<FXMenuCommand*>(sender);
	FXuint mode = glc_->getDisplayMode();
	SELID(sel) - ID_COLOR == mode >> 4 ? obj->check() : obj->uncheck();
	return 1;
}

long RasterDisplayWindow::onCmdScroll(FXObject *sender, FXSelector sel, void*)
{
	FXScrollbar *sb = static_cast<FXScrollbar*>(sender);
	FXint pos = sb->getPosition();
	
	if(SELID(sel) == ID_HSCROLL) {
		glc_->setXOrigin(pos);
		FXTRACE((5, "onCmdScroll(ID_HSCROLL): %d\n", pos));
		return 1;
	}
	
	if(SELID(sel) == ID_VSCROLL) {
		FXTRACE((5, "onCmdScroll(ID_VSCROLL): %d\n", pos));
		glc_->setYOrigin(pos);
		return 1;
	}

	return 0;
}

long RasterDisplayWindow::onConfigure(FXObject *obj, FXSelector sel, void *data)
{
	FXTopWindow::onConfigure(obj, sel, data);

	FXint w = glc_->getWidth(), h = glc_->getHeight();
	hscroll_->setPage(w);
	vscroll_->setPage(h);
	return 1;
}

long RasterDisplayWindow::onCmdAbout(FXObject*, FXSelector, void*)
{
   FXMessageBox about(
	   this, "Raster Alchemy", 
	   "(c) 2003,2004 Zeljko Vrba.\n"
	   "This program is GNU software.",
	   NULL, MBOX_OK | DECOR_TITLE | DECOR_BORDER);
   about.execute();
   return 1;
}

long RasterDisplayWindow::onCmdValue(FXObject*, FXSelector, void*)
{
	FXDialogBox dialog(this, "Constant channel value", DECOR_TITLE | DECOR_BORDER);
	FXVerticalFrame *vf = new FXVerticalFrame(&dialog);
	
	FXHorizontalFrame *hf = new FXHorizontalFrame(vf);
	new FXLabel(hf, "Value");
	FXTextField *text = new FXTextField(
		hf, 8, &dialog, FXDialogBox::ID_ACCEPT,
		TEXTFIELD_ENTER_ONLY | FRAME_SUNKEN | FRAME_THICK | LAYOUT_FILL_X);
	
	hf = new FXHorizontalFrame(vf);
	new FXButton(hf, "&OK", 0, &dialog, FXDialogBox::ID_ACCEPT,
				 BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK);
	new FXButton(hf, "&Cancel", 0, &dialog, FXDialogBox::ID_CANCEL,
				 BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK);

	text->setText(FXStringVal(glc_->getConstant(), 6, false));
	
	dialog.create();
	if(dialog.execute()) {
		FXfloat f = FXFloatVal(text->getText());
		glc_->setConstant(f);
	}
	return 1;
}

// this received hfield* through socket.
long RasterDisplayWindow::onSocketMsg(FXObject*, FXSelector, void*)
{
	extern int GuiSocket[2];
	const void *re, *im;
	hfield *hf;

	if(read(GuiSocket[0], &hf, sizeof(hf)) < sizeof(hf)) {
		fxerror("RasterDisplayWindow::onSocketMsg: incomplete data received.");
	}
	
	// in hfields RE and IM parts are consecutive (NOT interleaved)
	if(hf) {
		re = hf->a;
		im = hf->c ? hf->a + hf->xsize*hf->ysize : 0;
		setImage(GL_FLOAT, hf->xsize, hf->ysize, re, im);
	} else {
		setImage(GL_FLOAT, 0, 0, 0, 0);
	}
	return 1;
}

/**
   Set an image to display. \e pixtype is one of GL_FLOAT, etc. constants.
*/
void RasterDisplayWindow::setImage(
	FXint pixtype, FXuint w, FXuint h,
	const void *re, const void *im)
{
	glc_->setImage(pixtype, w, h, re, im);
	hscroll_->setRange(w-1); hscroll_->setPosition(0);
	vscroll_->setRange(h-1); vscroll_->setPosition(0);
}

