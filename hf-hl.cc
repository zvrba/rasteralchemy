/* Heightfield Lab : generate and modify heightfields
 * intended to work in a manner similar to Lee Crocker's "piclab" program
 * Uses 32-bit floating point internally to store pixel values.
 * Saves to TGA, POT, PGM, PNG, GIF, and Matlab formats.
 * Copyright (C) 1996  John P. Beale  <beale@jump.stanford.edu>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * --------------   JPB 1/18/96  --------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-hl.cc,v 1.1.2.3 2003/12/31 15:25:18 zvrba Exp $";

/* Values taken from hf-lab. */
struct HF_PARAMS HF_PARAMS = {
	TRUE,						/* rand_gauss */
	1,							/* rnd_seed_stale */
	0,							/* rnd_seed */
	1000,						/* histbins */
	AUTO,						/* tile_mode */
	0.01,						/* tile_tol */
	4.0							/* gaufac */
};

hfield *h_newr(int xs, int ys)	/* create real HF */
{
	PTYPE *a;
	size_t mem;
	hfield *hf = (hfield*)malloc(sizeof(hfield));

	mem = (size_t)xs*ys*sizeof(PTYPE);
	if(hf && (a = (float*)malloc(mem))) { 
		memset(a, 0, mem);
		hf->a = a;
		hf->xsize = xs;
		hf->ysize = ys;
		hf->c = 0;
		hf->max = hf->min = 0;
	} else {
		perror("ERROR: h_newr: malloc");
		free(hf);
		hf = NULL;
	}
	return hf;
}

hfield *h_newc(int xs, int ys)	/* create complex HF */
{
	PTYPE *a;
	size_t mem;
	hfield *hf = (hfield*)malloc(sizeof(hfield));
 
	mem = (size_t)xs*ys*2*sizeof(PTYPE);
	if(hf && (a = (float*)malloc(mem))) { 
		memset(a, 0, mem);
		hf->a = a;
		hf->xsize = xs;
		hf->ysize = ys;
		hf->c = 1;
		hf->max = hf->min = 0;
	} else {
		perror("ERROR: h_newc: malloc");
		free(hf);
		hf = NULL;
	}
	return hf;
}

void h_delete(hfield *hf)
{
	free(hf->a);
	free(hf);
}

void h_assign_free(hfield *dst, hfield *src)
{
	free(dst->a);
	memcpy(dst, src, sizeof(hfield));
	free(src);
}
