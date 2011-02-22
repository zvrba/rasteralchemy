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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fgm.h"

static char rcsid[] = "$Id: fgm.c,v 1.1.1.1.2.4 2004/09/24 17:18:23 zvrba Exp $";

static const unsigned int MAGIC = 0x01828384;

/**
   Create a new fgm with uninitialized image data.
   
   @param	width	Image width.
   @param	height	Image height.
   @return	Image, or NULL on failure.
*/
FGM fgm_new(unsigned width, unsigned height)
{
	FGM f = malloc(sizeof(struct fgm));
	
	if(f) {
		f->w = width; f->h = height;
		if(!(f->d = malloc(width * height * sizeof(float)))) {
			free(f);
			f = NULL;
		}
	}
	return f;
}

/**
   Set image data from an external buffer.

   @param	f		Image to copy data to.
   @param	data	External buffer.
*/
void fgm_set_data(FGM f, const void *data)
{
	memcpy(f->d, data, f->w * f->h * sizeof(float));
}

/** Free all of image data. */
void fgm_free(FGM f)
{
	free(f->d);
	free(f);
}

/** Return image width. */
unsigned fgm_width(FGM f)
{
	return f->w;
}

/** Return image height. */
unsigned fgm_height(FGM f)
{
	return f->h;
}

/** Return raw image data stored in row-major order. */
float *fgm_data(FGM f)
{
	return f->d;
}

/**
   Load a fgm image from file.
   
   @param	fname	File name to load from.
   @return	Image or NULL on failure (either because of invalid file format or
   insufficient memory).

   @todo	Byte swapping.
*/
FGM fgm_load(const char *fname)
{
	char buf[16];
	unsigned w, h;
	FGM result = NULL;
	FILE *f = fopen(fname, "rb");
	
	if(!f) return NULL;
	if((fscanf(f, "%15s%u%u", buf, &w, &h) < 3) ||
	   (strcmp(buf, "F1") != 0)) {
		goto end;
	}

	if(fseek(f, 256, SEEK_SET) < 0) goto end;
	if(!(result = fgm_new(w, h))) goto end;
	if(fread(fgm_data(result), sizeof(float), w*h, f) < w*h) {
		fgm_free(result);
		result = NULL;
	}
	
 end:
	fclose(f);
	return result;
}

/**
   Save fgm to a file.

   @param	f		Image to save.
   @param	fname	File name to save to.
   @return	0 on success, -1 on failure.
*/
int fgm_save(FGM f, const char *fname)
{
	int result = -1;
	unsigned int w = fgm_width(f), h = fgm_height(f);
	FILE *out = fopen(fname, "wb");
	
	if(!out) return -1;
	fprintf(out, "F1\n%u %u\n", w, h);
	if(fseek(out, 252, SEEK_SET) < 0) goto end;
	if(fwrite(&MAGIC, 1, sizeof(MAGIC), out) < sizeof(MAGIC)) goto end;
	if(fwrite(fgm_data(f), sizeof(float), w*h, out) < w*h) goto end;

	result = 0;
 end:
	fclose(out);
	return result;
}
