// -*- C++ -*-
// $Id: rdispwin.h,v 1.1.2.8 2004/01/06 15:59:11 zvrba Exp $
#ifndef MAINWIN_H__
#define MAINWIN_H__

#include <fox/fx.h>
#include "GLRasterCanvas.h"

/**
   Top-level window to display raster images.
   @todo	Color images.
*/
class RasterDisplayWindow : public FXTopWindow {
	FXDECLARE(RasterDisplayWindow);
public:
	RasterDisplayWindow(FXApp *parent = 0, const char *name = 0);

	void setImage(FXint, FXuint, FXuint, const void*, const void* = 0);

	enum {
		ID_EXIT = FXMainWindow::ID_LAST,
		ID_CONTRAST1,
		ID_CONTRAST2,
		ID_RECT,
		ID_POLAR,
		ID_COLOR,
		ID_HSCROLL = ID_COLOR+14,
		ID_VSCROLL,
		ID_VALUE,
		ID_ABOUT,
		ID_LAST
	};

	long onCmdAbout(FXObject*, FXSelector, void*);
	long onCmdDisplay(FXObject*, FXSelector, void*);
	long onUpdDisplay(FXObject*, FXSelector, void*);
	long onCmdColor(FXObject*, FXSelector, void*);
	long onUpdColor(FXObject*, FXSelector, void*);
	long onCmdValue(FXObject*, FXSelector, void*);
	long onCmdScroll(FXObject*, FXSelector, void*);
	long onConfigure(FXObject*, FXSelector, void*);
	long onSocketMsg(FXObject*, FXSelector, void*);

private:
	GLRasterCanvas *glc_;
	FXScrollbar *hscroll_, *vscroll_;
};

#endif // MAINWIN_H__
