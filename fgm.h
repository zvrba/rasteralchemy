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
#ifndef FGM_H__
#define FGM_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
   @file
   Floating-point raster file format definition.  The format of the
   file is similar to the already existing PNM fileformats. Each of
   the following items is alone itself on the line, in formatted text:
   
   - F1 (1 stands for number of channels)
   - dimensions in pixels (e.g. 1152 864)
   - zero padding until 252nd byte
   - at byte offset 252 a magic number (integer) containing 0x01828384;
     this is used to detect if byte swapping must be done on reading
	 (if the number reads as negative, assuming 2nd complement, then we must do
	 byte swapping)
   - pixel data starting at offset 256 in the file. Pixel data is stored
     row-major, in 32-bit IEEE format with native byte ordering of the
	 platform that wrote the file.

   @todo Implement paging so that large images (larger than physical
   or even virtual available memory (~4GB max on 32bit architectures)
   can be processed).

   @todo Multi-channel images.

   @todo Error-reporting. Something like fgm_errno().
*/

/* Actual FGM structure. */
typedef struct fgm {
	unsigned w, h;				/* width and height */
	float *d;					/* data */
} *FGM;

/* functions */
FGM fgm_new(unsigned width, unsigned height);
void fgm_set_data(FGM f, const void *data);
void fgm_free(FGM f);
FGM fgm_load(const char *fname);
int fgm_save(FGM f, const char *fname);
unsigned fgm_width(FGM f);
unsigned fgm_height(FGM f);
float *fgm_data(FGM f);

#ifdef __cplusplus
}
#endif

#endif /* FGM_H__ */
