#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hf-hl.h"
#include "hf-sclxform.h"

static char rcsid[] UNUSED = "$Id: hf-sclxform.cc,v 1.1.2.4 2003/12/31 15:25:18 zvrba Exp $";

hfield *h_op1(hfield *h1, const scalar_op1 &op)
{
	int cflag;
	U xsize, ysize, ix, iy;
	D ht3;

	xsize = h1->xsize;
	ysize = h1->ysize;
	cflag = h1->c;

	for (iy = 0; iy < ysize; iy++) {
		for (ix = 0; ix < xsize; ix++) {
			ht3 = op(El(h1->a,ix,iy));
			El(h1->a,ix,iy) = ht3;
		}  /* end for ix */
	}  /* end for iy */

	h_minmax(h1);
	return h1;
}
/**
   Combine two HFs with an offset xo,yo of the (smaller?) X into Y. The
   template argument function takes two double arguments (pixel value from
   first and second heightfield) and returns double (result).
*/
hfield *h_op2(hfield *h1, hfield *h2, int xo, int yo, const scalar_op2 &op)
{
	int cflag;
	int xsize,ysize;      /* for Im() operator */
	int xsize1, ysize1;   /* HF X dimensions */
	int xsize2, ysize2;   /* HF Y dimensions */
	hfield *h3;
	int ix, iy;
	int xx, yy;
	int tile;               /* whether to do a tiling operation */
	D ht1,ht2,ht3;      /* temp HF values */

	xsize = xsize1 = h1->xsize;      /* X hf */
	xsize = ysize1 = h1->ysize;
	xsize2 = h2->xsize;      /* Y hf */
	ysize2 = h2->ysize;
	if((xsize1 > xsize2) || (ysize1 > ysize2)) {
		fprintf(stderr, "ERROR: h_op2: X matrix dim's must equal to or less than Y dim.\n");
		return NULL;
	}
	if(h1->c && h2->c) {
		fprintf(stderr, "ERROR: h_op2: can't handle two complex operands\n");
		return NULL;
	}
	cflag = (h1->c || h2->c);  /* if new matrix will be complex */
	if (cflag)
		if(xsize1!=xsize2 || ysize1!=ysize2) {
			fprintf(stderr, "ERROR: h_op2: real & complex matrices of different size.\n");
			return NULL;
		}

	tile = h_tilable(h2,0);          /* TRUE if Y matrix is tilable */
  
	if (cflag) {
		if(!(h3 = h_newc(xsize2,ysize2))) return NULL;
	} else {
		if(!(h3 = h_newr(xsize2,ysize2))) return NULL;
	}

	if ((xo >= xsize2) || (yo >= ysize2)) {
		fprintf(stderr, "ERROR: h_op2: offset is larger than HF dimension.\n");
		return NULL;
	}
  
	if ( (xsize1!=xsize2 || ysize1!=ysize2) ||
		 (tile==FALSE && (xo!=0 || yo!=0))  )   {
		for (ix=0;ix<xsize2;ix++) {        /* copy over entire Y matrix to new */
			for (iy=0;iy<ysize2;iy++)  El2(h3->a,ix,iy)=El2(h2->a,ix,iy);
		}
	}

	for (iy = 0; iy < ysize1; iy++) {
		yy = iy+yo;     /* yo = y offset */
		if (tile && (yy>=ysize2)) yy -= ysize2;
		for (ix = 0; ix < xsize1; ix++) {
			xx = ix+xo;                    /* xo = x offset */
			if (tile && xx>=xsize2) xx -= xsize2;
			ht1 = El1(h1->a,ix,iy);
			if (xx<xsize2 && yy<ysize2) {
				ht2 = El2(h2->a,xx,yy);
				ht3 = op(ht1, ht2);
				El2(h3->a,xx,yy) = ht3;
			} /* end if xx&&yy > 0 */

		}  /* end for ix */
	}  /* end for iy */
	if (cflag) {
		xsize = xsize1;
		ysize = ysize1;
		if (h1->c == TRUE) {
			for (iy = 0; iy < ysize1; iy++) {
				for (ix = 0; ix < xsize1; ix++) {
					Im2(h3->a,ix,iy) = Im(h1->a,ix,iy);  /* new */
				} /* end for x */
			} /* end for y */
		} else {
			for (iy = 0; iy < ysize1; iy++) {
				for (ix = 0; ix < xsize1; ix++) {
					Im2(h3->a,ix,iy) = Im(h2->a,ix,iy);  /* new */
				} /* end for x */
			} /* end for y */
		}   /* end else */
	} /* end if cflag */
	
	h_minmax(h3);
	return h3;
}

