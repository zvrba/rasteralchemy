#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-cplx.cc,v 1.1.2.2 2003/12/31 15:25:18 zvrba Exp $";

/* --- cplx.c functions -------------------------------------- */
hfield *c_swap(hfield *hfin) /* swap complex and real parts of matrix */
{
	int ix,iy;
	int xsize, ysize;
	PTYPE *hf;
	hfield *chf;
	D tmp;

	hf = hfin->a;
	xsize = hfin->xsize;
	ysize = hfin->ysize;

	if (hfin->c) {              /* swap real, imag parts of mx */
		for (ix=0;ix<xsize;ix++) {
			for (iy=0;iy<ysize;iy++) {
				tmp = El(hf,ix,iy);
				El(hf,ix,iy) = Im(hf,ix,iy);
				Im(hf,ix,iy) = tmp;
			}
		}
		chf = hfin;
	} else {		  /* matrix is real; must create new complex mx */
		if(!(chf = h_newc(xsize,ysize))) return NULL;

		for (ix=0;ix<xsize;ix++) {   /* old real part -> new imag. part */
			for (iy=0;iy<ysize;iy++) {
				Im(chf->a,ix,iy) = El(hf,ix,iy);
			}
		}
	}  /* end if cflag */

	h_minmax(chf); /* new real extrema */
	return chf;
}

hfield *c_mag(hfield *hfc)		/* take magnitude of complex matrix */
{
	int ix,iy;
	int xsize, ysize;
	hfield *hfr;

	if(!hfc->c) {
		fprintf(stderr, "ERROR: cmag: matrix not complex.\n");
		return NULL;
	}
	xsize = hfc->xsize;
	ysize = hfc->ysize;
	if(!(hfr = h_newr(xsize,ysize))) return NULL;

	/* compute magnitude of matrix */

	for (ix=0;ix<xsize;ix++) {
		for (iy=0;iy<ysize;iy++) {
			El(hfr->a,ix,iy) =
				sqrt(pow(El(hfc->a,ix,iy),2)+pow(Im(hfc->a,ix,iy),2));
		}
	}

	h_minmax(hfr);
	return hfr;
}

hfield *c_convert(hfield *hfc, int dir)	/* convert complex matrix Rect <-> Polar */
{
	int ix,iy;
	int xsize, ysize;
	D re, im;        /* temporary variables */

	if(!hfc->c) {
		fprintf(stderr, "ERROR: convert: matrix not complex.\n");
		return NULL;
	}
	xsize = hfc->xsize;
	ysize = hfc->ysize;

	/* convert matrix */
	if (dir==0) {           /* rectangular -> polar */
		for (ix=0;ix<xsize;ix++) {
			for (iy=0;iy<ysize;iy++) {
				re = El(hfc->a,ix,iy);
				im = Im(hfc->a,ix,iy);
				El(hfc->a,ix,iy) = sqrt(re*re + im*im);
				Im(hfc->a,ix,iy) = ATAN2(im,re);
			}
		}
	} else {                /* polar -> rectangular */
		for (ix=0;ix<xsize;ix++) {
			for (iy=0;iy<ysize;iy++) {
				re = El(hfc->a,ix,iy);
				im = Im(hfc->a,ix,iy);
				El(hfc->a,ix,iy) = re * cos(im);
				Im(hfc->a,ix,iy) = re * sin(im);
			}
		}
	}

	h_minmax(hfc); /* new real extrema */
	return hfc;
}


hfield *c_join(hfield *hfr, hfield *hfi) /* make complex from real & imag parts of matrix */
{
	unsigned int ix, iy, xsize, ysize;
	hfield *hfc;

	xsize = hfr->xsize;
	ysize = hfr->ysize;
	if(xsize != hfi->xsize || ysize != hfi->ysize) {
		fprintf(stderr, "ERROR: c_join: matrices must be of same size\n");
		return NULL;
	}

	if(!(hfc = h_newc(xsize,ysize))) return NULL;

	for (ix=0;ix<xsize;ix++) {   /* old real part -> new imag. part */
		for (iy=0;iy<ysize;iy++) {
			El(hfc->a,ix,iy) = El(hfr->a,ix,iy);
			Im(hfc->a,ix,iy) = El(hfi->a,ix,iy);
		}
	}
  
	h_minmax(hfc);				/* new real extrema */
	return hfc;
}


int c_split(hfield *hfc, hfield **hfr, hfield **hfi)  /* divide complex into real & imag parts */
{
	int ix,iy;
	int xsize, ysize;

	if(hfc->c) {
		fprintf(stderr, "ERROR: csplit: matrix not complex.\n");
		return 0;
	}

	xsize = hfc->xsize;
	ysize = hfc->ysize;
	if(!(*hfr = h_newr(xsize,ysize))) return 0;
	if(!(*hfi = h_newr(xsize,ysize))) return 0;

	for (ix=0;ix<xsize;ix++) { 
		for (iy=0;iy<ysize;iy++) {
			El((*hfr)->a,ix,iy) = El(hfc->a,ix,iy);
			El((*hfi)->a,ix,iy) = Im(hfc->a,ix,iy);
		}
	}

	h_minmax(*hfr);
	h_minmax(*hfi);
	return 1;
}

hfield *c_real(hfield *hf)  /* take real part of array */
{
	int xsize, ysize;
	size_t memsize;

	if(!hf->c) return hf;    /* nothing to do */

	xsize = hf->xsize;
	ysize = hf->ysize;
  
	memsize = (size_t)xsize*ysize*sizeof(PTYPE);
	if(!(hf->a = (float*)realloc(hf->a, memsize))) { /* truncate array to real */
		fprintf(stderr, "FATAL: c_real: realloc shrinking returned NULL\n");
		exit(1);
	}
	hf->c = FALSE;
	return hf;
}

hfield *c_diff(hfield *hf) /* make complex from x & y slope of matrix */
{
	int ix,iy;
	int xsize, ysize;
	hfield *hfc;
	int tile;
	D xdiff, ydiff, tmp;

	xsize = hf->xsize;
	ysize = hf->ysize;
	tile = h_tilable(hf, 0);

	if(!(hfc = h_newc(xsize,ysize))) return NULL;

	for (ix=0;ix<xsize;ix++) {   /* old real part -> new imag. part */
		for (iy=0;iy<ysize;iy++) {
			tmp = El(hf->a,ix,iy);
			if (tile) {
				xdiff = tmp-Elmod(hf->a,ix-1,iy);
				ydiff = tmp-Elmod(hf->a,ix,iy-1);
			} else {
				xdiff = tmp-Elclip(hf->a,ix-1,iy);
				ydiff = tmp-Elclip(hf->a,ix,iy-1);
			}
			El(hfc->a,ix,iy) = xdiff;
			Im(hfc->a,ix,iy) = ydiff;
		}
	}
  
	h_minmax(hfc); /* new real extrema */
	return hfc;
}

hfield *c_int(hfield *hf0) /* generate real mx from complex x&y diff'l */
{
	int xsize,ysize;
	int ix,iy;
	hfield *hf1;

	if(!hf0->c) {
		fprintf(stderr, "ERROR: cint: input must be complex.\n");
		return NULL;
	}
	xsize = hf0->xsize;
	ysize = hf0->ysize;
	if(!(hf1 = h_newr(xsize,ysize))) return NULL;

	El(hf1->a,0,0) = 0;        /* start with corner [0,0] = 0.0 */
	for (ix=1;ix<xsize;ix++)
		El(hf1->a,ix,0) = El(hf1->a,ix-1,0) + El(hf0->a,ix,0);
	for (iy=1;iy<ysize;iy++)
		El(hf1->a,0,iy) = El(hf1->a,0,iy-1) + Im(hf0->a,0,iy);

	for (ix=1;ix<xsize;ix++) {
		for (iy=1;iy<ysize;iy++) {
			El(hf1->a,ix,iy) = 0.5 * (El(hf1->a,ix-1,iy)+El(hf0->a,ix,iy) +
									 El(hf1->a,ix,iy-1)+Im(hf0->a,ix,iy)   );
		}
	}

	h_minmax(hf1);				/* find min,max values */
	return hf1;
}
