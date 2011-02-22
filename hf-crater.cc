/* Add craters to a heightfield array 
 * Copyright (C) 1995 by Heiko Eissfeldt
 * heiko@colossus.escape.de
 * modified JAN 1996 John Beale for use with HF-Lab  

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

 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-crater.cc,v 1.1.2.3 2003/12/31 15:25:18 zvrba Exp $";

#define alpha   (35.25/180.0*M_PI)      /* sphere segment cutoff angle */
#define crater_depth 0.85  /* 0.9 */

static double a1 = crater_depth, b, d;

/* function to determine the z-elevation dependent on normalized squared
   radial coordinate */
static double crater_profile(double nsq_rad)
{
	static double c = 50.0; /*100.0;*/   /* controls the wall thickeness */
	double radialcoord;
	radialcoord = sqrt(fabs(nsq_rad));

	if (radialcoord > b) {
		/* outer region gauss distribution */
		return d*exp(-c*(radialcoord-b)*(radialcoord-b));
	} else {
		/* inner region sphere segment */
		return a1 - sqrt(1.0 - nsq_rad);
	}
}

#define pow4(x) ((x)*(x)*(x)*(x))
#define border 0.63
#define inner_scale 1
#define outer_scale (1.0-inner_scale*pow4(border))/pow4(1.0-border)

/* function to control the weight of the crater profile as opposed
   to the underlying terrain elevation */
static double dissolve(double nsq_rad)
{
#if 0
	if (nsq_rad > border*border) {
		double temp = (1-sqrt(nsq_rad));
		return 1.0-outer_scale*pow4(temp);
	} else {
		return inner_scale*nsq_rad*nsq_rad;
	}
#else
	if (nsq_rad > 0.6*0.6) {
		return 1.0-0.9*exp(-30.0*(sqrt(nsq_rad) - 0.6)*(sqrt(nsq_rad) - 0.6));
	} else {
		return 0.1*exp(-25.0*(sqrt(nsq_rad) - 0.6)*(sqrt(nsq_rad) - 0.6));
	}
#if 0
	if (nsq_rad > border*border) {
		double temp = (1-sqrt(nsq_rad));
		return 1.0-outer_scale*pow4(temp);
	} else {
/*      return 0.2+ 0.12*cos(sqrt(nsq_rad)*7);*/
		return 1.0- 4.0*(sqrt(nsq_rad) - 0.17)*(sqrt(nsq_rad) - 0.17);
		return 0.04+1.7*(sqrt(nsq_rad) - 0.38)*(sqrt(nsq_rad) - 0.38);
	}
#endif
#endif
}

#define CRATER_COVERAGE 0.15

/*  DISTRIBUTE_CRATERS  --  doing some damage to the surface */

int distribute_craters(PTYPE *real, PTYPE *imag, int xsize, int ysize, 
					   unsigned int how_many, int wrap, D ch_scale, D cr_scale, D b1)
{
    int i, j, k, xloc, yloc, cratersize;
    int sq_radius;
    double nsq_rad, weight;
    int lev_samples, samples;
    double level_with, level_pure;
    double craterscale;                 /* vertical crater scaling factor */
    double c,d2, shift;
    double b2,b3;                       /* radius scaling params */
    int meshsize;                       /* mean of xsize and ysize ? */

    /* init constants */
    
    if (b1 < 1.0) {
		printf("Crater: distribution factor must be >= 1\n");
		return(1);     /* better not be less than that */
    }
    b2 = 1.0/b1/b1/b1/b1; 
    b3 = 1.0/b1;
    meshsize = (int)(sqrt(xsize * ysize));
    b = sin(alpha);
    d = a1 - cos(alpha);

    /* build a copy of the terrain */
    for (i = 0 ; i < xsize; i++)
		for (j = 0 ; j < ysize; j++)
			El(imag,i,j) = El(real,i,j);

    k = how_many;
    while (k > 0) {
		int ii, jj;

		/* pick a cratersize according to a power law distribution */
		/* greatest first. So great craters never eliminate small ones */
		/* c = (double)(how_many-k+1)/how_many + b3; */
		/* craters appear in random order */

		c = ran1() + b3;       /* c is in the range b3 ... b3+1 */
		d2 = b2/c/c/c/c;       /* d2 is in the range 0 ... 1 */

		if (how_many == 1) d2 = 1.0;   /* single craters set to max. size */

		xloc = (int)(ran1() * xsize); /* pick a random location for the crater */
		yloc = (int)(ran1() * ysize);

		cratersize = 3 + (int)(d2 * CRATER_COVERAGE * meshsize * cr_scale);


/* macro to determine the height dependent on crater size */
#define CRATER_SCALE (((ch_scale*pow((cratersize/(3+CRATER_COVERAGE*meshsize)),0.9)) \
		       /256*pow(meshsize/256.0,0.1))/CRATER_COVERAGE*80)

		craterscale = CRATER_SCALE;     /* vertical crater scaling factor */
	
		/* what is the mean height of this plot */
		samples = lev_samples = MAX((cratersize*cratersize)/5, 1);
		level_with = level_pure = 0.0;
		while (samples) {
			/* */
			/*  i = rand()%(2*cratersize+1) - cratersize;
				j = rand()%(2*cratersize+1) - cratersize; */
			i = (int)(ran1()*(2*cratersize) - cratersize);
			j = (int)(ran1()*(2*cratersize) - cratersize);

			if (i*i+j*j > cratersize*cratersize) continue;
			ii = xloc + i; jj = yloc + j;

			/* handle wrapping details... */

			if ((wrap) || 
				((ii >= 0) && (ii < xsize) && (jj >= 0) && (jj < ysize))) {
				X_WRAP(ii);
				Y_WRAP(jj);
				level_with += El(real, ii, jj);
				level_pure += El(imag, ii, jj);
				samples--;
			}

		}
		level_with /= lev_samples;
		level_pure /= lev_samples;

		/* Now lets create the crater. */


		/* In order to do it efficiently, we calculate for one octant
		 * only and use eightfold symmetry, if possible.
		 * Main diagonals and axes have a four fold symmetry only.
		 * The center point has to be treated single.
		 */

#define SQUEEZE 1.3

/* this macro calculates the coordinates, does clipping and modifies
 * the elevation at this spot due to shift and weight.
 * Imag() contains the crater-free underground. Real() contains the result.
 * level_with: average altitude of cratered surface in region
 * level_pure: average altitude of uncratered surface
 */

#define SHIFT(x,y) { \
    ii = xloc + (x); jj = yloc + (y); \
    if (wrap || ((ii >= 0) && (ii < xsize) && \
		 (jj >= 0) && (jj < ysize))) {\
      X_WRAP(ii);  Y_WRAP(jj); \
      El(real,ii,jj) = (shift  + (El(real,ii,jj))*weight + \
      (level_with + (El(imag,ii,jj)-level_pure)/SQUEEZE) * (1.0-weight)); \
    } \
  }

/* macro to do four points at once. Points are rotated by 90 degrees. */
#define FOURFOLD(i,j)   SHIFT(i,j) SHIFT(-j,i) SHIFT(-i,-j) SHIFT(j,-i)

/* get eightfold symmetry by mirroring fourfold symmetry along the x==y axe */
#define EIGHTFOLD       FOURFOLD(i,j) FOURFOLD(j,i)


		/* The loop covers a triangle (except the center point)
		 * Eg cratersize is 3, coordinates are shown as i,j
		 *
		 *              3,3 j
		 *         2,2  3,2 |
		 *    1,1  2,1  3,1 v
		 * x  1,0  2,0  3,0
		 * ^          <-i
		 * |
		 * center point
		 *
		 * 2,1 , 3,2 and 3,1 have eightfold symmetry.
		 * 1,0 , 2,0 , 3,0 , 1,1 , 2,2 and 3,3 have fourfold symmetry.
		 */

		for (i = cratersize; i > 0; i--) {
			for (j = i; j >= 0; j--) {

				/* check if outside */
				sq_radius = i*i+j*j;
				nsq_rad = (double)sq_radius/cratersize/cratersize;
				if (nsq_rad > 1) continue;

				/* inside the crater area */
				shift = craterscale*crater_profile(nsq_rad);
				weight = dissolve(nsq_rad);

				if (i==j || j==0) {
					FOURFOLD(i,j)
						} else {
							EIGHTFOLD
								}
			}
		}
		/* the center point */
		shift =  craterscale*crater_profile(0.0);
		weight = dissolve(0.0);
		SHIFT(0,0)

			/* one crater added */
			k--;
		/* printf("%d craters of %d created\n", how_many - k, how_many); */
    }
	return(0);
} /* end distribute_craters() */

