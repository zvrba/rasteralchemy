 /* ops.c   --  heightfield operations
 *              Jan 15 1996   John P. Beale
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "hf-hl.h"
#include "hf-fftn.h"

#define BANDPASS 1		 /* frequency-domain (fourier) filter types */
#define BANDREJECT -1
#define LOWPASS 2
#define HIGHPASS -2
#define Nrand 4				 /* number of rand samples for gaussian */

static double arand, gaussadd, gaussfac; /* Gaussian random parameters */
static char rcsid[] UNUSED = "$Id: hf-ops.cc,v 1.1.2.3 2004/01/08 16:30:44 zvrba Exp $";

/* --------------------------------------------------- */

/* smooth(f) -- smooth matrix by replacing each HF element with average of
	    its neighbors.  f=0: no smoothing. f=1.0 : full smoothing.
*/

hfield *smooth(hfield *h0, D frac)
{
	int x,y;
	int xsize,ysize;
	double tmp, orig;
	hfield *h1;
	int wrap;

	if(h0->c) fprintf(stderr, "WARNING: smooth: smoothing real part only.\n");
	xsize = h0->xsize;
	ysize = h0->ysize;
	wrap = h_tilable(h0,0);           /* if image is tilable or not */
  
	if(h0->c) {
		if(!(h1 = h_newc(xsize,ysize))) return NULL;
	} else {
		if(!(h1 = h_newr(xsize,ysize))) return NULL;
	}

	for(y=0;y<ysize;y++) {
		for (x=0;x<xsize;x++) {
			orig = El(h0->a,x,y);
			if (wrap) tmp = Elmod(h0->a,x-1,y) + Elmod(h0->a,x,y-1)+
						  Elmod(h0->a,x+1,y) + Elmod(h0->a,x,y+1);
			else      tmp = Elclip(h0->a,x-1,y) + Elclip(h0->a,x,y-1)+
						  Elclip(h0->a,x+1,y) + Elclip(h0->a,x,y+1);

			tmp /= 4;         /* compute average of neighbors */
			tmp = orig + frac*(tmp-orig);
			El(h1->a,x,y) = tmp;
		}
	}

	h_minmax(h1);
	return h1;
}

hfield *gen_rand(int xsize, int ysize)
{
	int x,y;
	hfield *h1;
	register PTYPE *hf;
	D hmin,hmax,tmp;
 
	if(!(h1 = h_newr(xsize,ysize))) return NULL;
	hf = h1->a;

	initgauss(); 
	hmin = FLT_MAX;
	hmax = FLT_MIN;
	for (y=0;y<ysize;y++) {
		for (x=0;x<xsize;x++) {
			if (HF_PARAMS.rand_gauss) tmp = gaussn();
			else tmp = ran1();
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
			El(hf,x,y) = tmp;
		}
	}
	h1->min = hmin;
	h1->max = hmax;

	return h1;
}

/* h_fft(dir, scal)  -- do forward or inverse fourier transform
 *                 dir = +1  forward FFT
 *                       -1  inverse FFT
 *                 scal =  1.0  no renormalization
 *                 scal = -1.0  renorm by mx. dimension
 *                 scal < -1.0  renorm by sqrt(mx dim)
 */

hfield *h_fft(hfield *hf, int dir, D scaling)
{
	int dim[2];           /* fft dimensions */
	PTYPE *hf0, *hf1;
 
	if(!hf->c) {
		fprintf(stderr, "ERROR: fft: complex matrix is required.\n");
		return NULL;
	}

	hf0 = hf->a;
	hf1 = &(hf->a[hf->xsize * hf->ysize]); /* starting point of imag. matrix */
	dim[0] = hf->xsize;
	dim[1] = hf->ysize;

	/* #dims dim_array, *real, *imag, fwd/rev, scaling */
	/* if scaling = -1 norm by dimension, scaling < -1 norm by sqrt(dim) */

	fftnf(2, dim, hf0, hf1, dir, scaling); 
	h_minmax(hf);
	return hf;
}

hfield *gforge(int size, float h)
{
	hfield *ret;

	if(size<3) {
		fprintf(stderr, "ERROR: gforge: minimum array size is 3.\n");
		return NULL;
	}
	if((h<0) || (h>4)) {
		fprintf(stderr, "ERROR: gforge: dimension must be in range [0..4]\n");
		return NULL;
	}

	if(!(ret = fillarray(size,size,3.0-h))) { /* generate complex array */
		return NULL;
	}

	h_fft(ret, -1, 1.0);		/* take inverse fft */
	c_real(ret);				/* separate real & imag. parts */
	norm(ret, 0, 1);			/* normalize to 0..1 */
	return ret;
}

/* fill array with 1/f gaussian noise */
hfield *fillarray(int xsize, int ysize, float h)
{
	hfield *h1;
	int x,y, k, i0, j0, rank, nx, ny, xcent, ycent, rankmax;
	double rad, phase, rcos, rsin, scale;
	PTYPE *real, *imag;

	if(!(h1 = h_newc(xsize,ysize))) return NULL;

	nx = xsize; ny = ysize;
	real = h1->a;
	imag = &(h1->a[xsize*ysize]);

	/* printf("filling %d x %d HF\n",xsize,ysize); */

	xcent = (int)(xsize / 2.0 - 0.5);  /* center dimensions of array */
	ycent = (int)(ysize / 2.0 - 0.5);
	rankmax = MIN(xcent,ycent);

    /* fill in mx. in order of radius, so we can generate higher resolutions
       with the same overall aspect (if seed is held fixed) */
 
	/* initialize gaussian/random # with time */
    initgauss();
    scale = 1.0;

    for (rank = 0; rank <= rankmax; rank++) {
		/* fill quadrants 2 and 4  */
		for (k=0;k<=rank;k++) {
			x = k; y = rank;
			phase = 2 * M_PI * ((ran1() * 0x7FFF) / arand);
			if ((x == 0) && (y == 0)) rad = 0; 
			else rad = pow((double) (x*x + y*y), -(h+1) / 2) * gauss();
			rcos = rad * cos(phase)*scale; rsin = rad * sin(phase)*scale;
			El(real, x, y) = rcos; 
			El(imag, x, y) = rsin;
			if (!((x == 0) && (y == 0))) { 
				i0 = nx-x-1; j0 = ny-y-1;
				El(real, i0,j0) = rcos;
				El(imag, i0,j0) = rsin;
			}

			x = rank; y = k;
			phase = 2 * M_PI * ((ran1() * 0x7FFF) / arand);
			if ((x == 0) && (y == 0)) rad = 0; 
			else rad = pow((double) (x*x + y*y), -(h+1) / 2) * gauss();
			rcos = rad * cos(phase)*scale; rsin = rad * sin(phase)*scale;
			El(real, x, y) = rcos;
			El(imag, x, y) = rsin;
			if (!((x == 0) && (y == 0))) { 
				i0 = nx-x-1; j0 = ny-y-1;
				El(real, i0,j0) = rcos;
				El(imag, i0,j0) = rsin;
			}
		} /* end for k */
    
		/* now handle quadrants 1 and 3 */
		for (k=0;k<=rank;k++) {
			x = k; y = rank;
			phase = 2 * M_PI * ((ran1() * 0x7FFF) / arand);
			if ((x == 0) && (y == 0)) rad = 0; 
			else rad = pow((double) (x*x + y*y), -(h+1) / 2) * gauss();
			rcos = rad * cos(phase)*scale; 
			rsin = rad * sin(phase)*scale;
			El(real, x, ny-y-1) = rcos; 
			El(imag, x, ny-y-1) = rsin;
			El(real, nx-x-1, y) = rcos;
			El(imag, nx-x-1, y) = rsin;
       
			x = rank; y = k;
			phase = 2 * M_PI * ((ran1() * 0x7FFF) / arand);
			if ((x == 0) && (y == 0)) rad = 0; 
			else rad = pow((double) (x*x + y*y), -(h+1) / 2) * gauss();
			rcos = rad * cos(phase)*scale; rsin = rad * sin(phase)*scale;
			El(real, x, ny-y-1) = rcos; 
			El(imag, x, ny-y-1) = rsin;
			El(real, nx-x-1, y) = rcos;
			El(imag, nx-x-1, y) = rsin;
      
		} /* end for k */
    } /* end for rank */
    
    El(imag, nx / 2, 0) = 0;
    El(imag, 0, ny / 2) = 0;
    El(imag, nx / 2, ny / 2) = 0;
 
	h_minmax(h1);				/* find limits of real array */
	return h1;
} /* end fillarray() */

/*  INITGAUSS  --  Initialise random number generators.  As given in
		   Peitgen & Saupe, page 77. */
void initgauss(void)
{
    /* Range of random generator */
    arand = pow(2.0, 15.0) - 1.0;
    gaussadd = sqrt(3.0 * Nrand);
    gaussfac = 2 * gaussadd / (Nrand * arand);

    if (HF_PARAMS.rnd_seed_stale)
		HF_PARAMS.rnd_seed = (int) (time(NULL) ^ 0xF37C)%1000000;
    seed_ran1((int)HF_PARAMS.rnd_seed);        /* seed the generator */
    HF_PARAMS.rnd_seed_stale = TRUE;
}

/*  GAUSS  --  Return a Gaussian random number.  As given in Peitgen
	& Saupe, page 77. */
double gauss()
{
	int i;
	D sum = 0.0;

	for (i=1; i<=Nrand; i++) sum += (ran1() * 0x7FFF);
	return(gaussfac * sum - gaussadd);
}

double gaussn()   /* returns gaussian dist. rand # between 0 and 1 */
{
	int i;
	D sum = 0.0;

	for (i=0; i<Nrand; i++) sum += ran1();
	return(sum/Nrand);
}

hfield *yslope(hfield *hfin, D yfrac, D y_sfac, D slope_exp)
{
	int xsize,ysize;
	int backedge = TRUE;	  /* if slope is max at back edge of hf */
	int i,j;
	D yf, z;
	D yscale;			   /* num to multiply yf by so range = 0..1 */
	PTYPE *hf;
	PTYPE hmin, hmax;

	if((yfrac < 0)||(yfrac > 1)) {
		fprintf(stderr, "ERROR: yslope: frac must be within [0..1].\n");
		return NULL;
	}
	if(slope_exp<0) {
		fprintf(stderr, "ERROR: yslope: exponent must be positive.\n");
		return NULL;
	}

	xsize = hfin->xsize;
	ysize = hfin->ysize;
	hf = hfin->a;
	hmin = FLT_MAX;
	hmax = FLT_MIN;

	yfrac = 1-yfrac;
	if (yfrac == 0) yscale = 1;
	else yscale = 1/yfrac;

	for (j=0;j<ysize;j++) {
		for (i=0;i<xsize;i++) {
			yf = ((double)j)/ysize;	/* fractional distance along y coord */
			if (backedge) yf = 1-yf;
			yf = (yf - yfrac) * yscale;
			if (yf < 0) yf = 0;
			z = (y_sfac * pow(yf,slope_exp)) + El(hf,i,j);
			El(hf,i,j) = (PTYPE)z;
			if (z > hmax) hmax = z;
			if (z < hmin) hmin = z;
		} /* end for i */
	} /* end for j */

	hfin->min = hmin;
	hfin->max = hmax;

	return hfin;
}

/* add gaussian hill */
hfield *h_gauss(hfield *hfin, D xfrac, D yfrac, D radfac, D hscale)
{
	long int xsize,ysize;
	long int xmin, xmax, ymin, ymax;
	long int x,y;
	D xa,ya,tmp,rad,xcent,ycent;
	PTYPE *hf;
	PTYPE hmin, hmax;
	int tile;

	if((yfrac < 0) || (yfrac > 1) || (xfrac<0) || (yfrac>1)) {
		fprintf(stderr, "ERROR: gauss: xfrac, yfrac must be within [0..1].\n");
		return NULL;
	}
	xsize = hfin->xsize;
	ysize = hfin->ysize;
	hf = hfin->a;
	hmin = FLT_MAX;
	hmax = FLT_MIN;
	xcent = xsize/2.0;  ycent = ysize/2.0;
	tile = is_tilable(hfin,0);
	if (tile) {
		xmin = (int)(xcent-(HF_PARAMS.gaufac*radfac*xsize)); 
		xmax = (int)(xcent+(HF_PARAMS.gaufac*radfac*xsize));
		ymin = (int)(ycent-(HF_PARAMS.gaufac*radfac*ysize)); 
		ymax = (int)(ycent+(HF_PARAMS.gaufac*radfac*ysize));
	} else {
		xmin = 0; xmax = xsize;
		ymin = 0; ymax = ysize;
	}
	radfac = radfac * radfac;
	for (y=ymin;y<ymax;y++) {
		ya = (((D)y+0.5)/ysize)-yfrac;
		ya = ya*ya;
		for (x=xmin;x<xmax;x++) {
/*    xx = (x>=0)?(x%xsize):(((x%xsize)+xsize))%xsize;
	  if ( ((x+10*xsize)%xsize==0) && ((y+10*ysize)%ysize==0) ) 
      printf("%ldx%ld ",xx,yy); */
			if (tile) tmp = Elmod(hf,x,y); else tmp = El(hf,x,y); 
/*    if (tile) tmp = El(hf,xx,yy); else tmp = El(hf,x,y); */
			xa = (((D)x+0.5)/xsize)-xfrac;
			xa = xa * xa;
			rad = xa+ya;
			tmp += hscale * exp(-rad/radfac);
			if (tile) Elmod(hf,x,y) = tmp; else El(hf,x,y) = tmp; 
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		} /* end for x */
	} /* end for y */
	if (tile) h_minmax(hfin);
	else {
		hfin->min = hmin;
		hfin->max = hmax;
	}

	return hfin;
}

/* add a ring */
hfield *h_ring(hfield *hfin, D xfrac, D yfrac, D radfac, D width, D hscale)
{
	long int xsize,ysize;
	long int xmin, xmax, ymin, ymax;
	long int x,y;
	D xa,ya,tmp,rad,xcent,ycent;
	PTYPE *hf;
	PTYPE hmin, hmax;
	int tile;

	if((yfrac < 0) || (yfrac > 1) || (xfrac<0) || (yfrac>1)) {
		fprintf(stderr, "ERROR: gauss: xfrac, yfrac must be within [0..1].\n");
		return NULL;
	}

	xsize = hfin->xsize;
	ysize = hfin->ysize;
	hf = hfin->a;
	hmin = FLT_MAX;
	hmax = FLT_MIN;
	xcent = xsize/2.0;  ycent = ysize/2.0;
	tile = is_tilable(hfin,0);
	if (tile) {
		xmin = (long)(xcent-((HF_PARAMS.gaufac*width+radfac)*xsize)); 
		xmax = (long)(xcent+((HF_PARAMS.gaufac*width+radfac)*xsize));
		ymin = (long)(ycent-((HF_PARAMS.gaufac*width+radfac)*ysize)); 
		ymax = (long)(ycent+((HF_PARAMS.gaufac*width+radfac)*ysize));
	} else {
		xmin = 0; xmax = xsize;
		ymin = 0; ymax = ysize;
	}
	width = width * width;  /* gaussian sigma, squared */
	for (y=ymin;y<ymax;y++) {   /* --- main loop over (x,y) ---- */
		ya = (((D)y+0.5)/ysize)-yfrac;
		ya *= ya;
		for (x=xmin;x<xmax;x++) {
			if (tile) tmp = Elmod(hf,x,y); else tmp = El(hf,x,y); 
			xa = (((D)x+0.5)/xsize)-xfrac;
			xa *= xa;
			rad = sqrt(xa + ya);
			tmp += hscale * exp(-pow((rad-radfac)/width,2));  /* gaussian */
			if (tile) Elmod(hf,x,y) = tmp; else El(hf,x,y) = tmp; 
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		} /* end for x */
	} /* end for y */
	if (tile) h_minmax(hfin);
	else {
		hfin->min = hmin;
		hfin->max = hmax;
	}

	return hfin;
}

/* -  h_crater() ---- add craters to a heightfield ----------- */
hfield *h_crater(hfield *h0, int how_many, D ch_scale, D radius, D dfac)
{
	PTYPE *real, *imag;
	hfield *h1;				  /* real and (temporary) imaginary HFs */
	int xsize, ysize;
	int wrap;

	xsize = h0->xsize;
	ysize = h0->ysize;
	real = h0->a;

	if(!(h1 = h_newr(xsize,ysize))) return NULL;

	imag = h1->a;
	wrap = h_tilable(h0, 0);
	initgauss();    /* seed or re-seed random # generator */

	distribute_craters(real, imag, xsize, ysize, (U) how_many, 
					   wrap, ch_scale, radius, dfac);

	h_delete(h1);
	h_minmax(h0);				/* update min, max values */
	return h0;
}


/* f_filter()  -- filter array of noise  with peak or notch filter */
/*                a frequency-domain filter: acts on real & imag arrays */
/*                f_type: 1 bpass, -1 breject, 2 lopass, -2 hipass */

void f_filter(PTYPE *real, PTYPE *imag, int xsize, int ysize, 
			  double center, double Q, int f_type)
{
	int i, j, i0, j0;
	double rad, fac, p, sfac;

    if (center == 0.0) center = -0.00001;  /* avoid a singularity */

	/* sfac is radius scaling factor */
    sfac = 1.0/sqrt((double)(xsize*xsize/4 + ysize*ysize/4));  
    for (i = 0; i <= xsize / 2; i++) {          /* do quadrants 2 and 4 */
		for (j = 0; j <= ysize / 2; j++) {
			if (i != 0 || j != 0) {
				rad = sqrt((double) (i * i + j * j)) * sfac;
			} else {
				rad = 0;
			}
			p = 1.0 / pow(Q * center, 2);
			if (abs(f_type)==1)
				fac = p / (p + pow((1.0-rad/center),2)) ; /* bandpass/rej. */
			else
				fac = 1.0 / (1.0 + pow((rad/center),Q) ); /* lo/hi-pass */
			if (f_type < 0) fac = (1.0 - fac);  /* invert filter */
			El(real, i, j) *= fac;
			El(imag, i, j) *= fac;
			i0 = (i == 0) ? 0 : xsize - i;
			j0 = (j == 0) ? 0 : ysize - j;
			El(real, i0, j0) *= fac;
			El(imag, i0, j0) *= fac;
		}
    }
    El(imag, xsize / 2, 0) = 0;
    El(imag, 0, ysize / 2) = 0;
    El(imag, xsize / 2, ysize / 2) = 0;
    for (i = 1; i <= xsize / 2 - 1; i++) {     /* do quadrants 1 and 3 */
		for (j = 1; j <= ysize / 2 - 1; j++) {
			rad = sfac * sqrt((double) (i * i + j * j));
			p = 1.0 / pow(Q * center, 2);
			if (abs(f_type)==1)
				fac = p / (p + pow((1.0-rad/center),2)) ; /* bandpass/rej. */
			else
				fac = 1.0 / (1.0 + pow((rad/center),Q) ); /* lo/hi-pass */
			if (f_type < 0) fac = (1.0 - fac);  /* invert filter */
			El(real, i, ysize - j) *= fac;
			El(imag, i, ysize - j) *= fac;
			El(real, xsize - i, j) *= fac;
			El(imag, xsize - i, j) *= fac;
		}
    }

}

/* h_fourfilt()  -- do filtering on fourier-domain data in real and
 *              imaginary arrays on stack
 *
 *  calls: f_filter(PTYPE *real, PTYPE *imag, int xsize, int ysize, 
 *             double center, double Q, int f_type)
 *
 */
hfield *h_fourfilt(hfield *hf, D center, D Q, const char *f_type)
{
	int type;					/* filter type */
	int xsize,ysize;
	PTYPE *imag, *real;
 
	if(!hf->c) {
		fprintf(stderr, "ERROR: fourfilt: matrix must be complex.\n");
		return NULL;
	}

	xsize = hf->xsize;
	ysize = hf->ysize;
	real = hf->a;
	imag = &(hf->a[xsize*ysize]);

	if     (strncmp(f_type, "ffbp", 4)) type = BANDPASS;
	else if(strncmp(f_type, "ffbr", 4)) type = BANDREJECT;
	else if(strncmp(f_type, "fflp", 4)) type = LOWPASS;
	else if(strncmp(f_type, "ffhp", 4)) type = HIGHPASS;
	else {
		fprintf(stderr, "ERROR: h_fourfilt: unknown filter type %s\n",f_type);
		return NULL;
	}
 
	f_filter(real, imag, xsize, ysize, center, Q, type); /* do filtering */
	h_minmax(hf);				/* and those of real array */
	return hf;
}

/* h_realfilt() -- Fourier filter a real-valued matrix: fft->filter->ifft */
hfield *h_realfilt(hfield *h0, D a1, D a2, const char *f_type)
{
	hfield *h1;
	char fs[6];
 
	if      (strncmp(f_type,"bp",2)) {
		strcpy(fs,"ffbp");
		if (a2==0) a2=4;
	} else if (strncmp(f_type,"br",2)) {
		strcpy(fs,"ffbr");
		if (a1==-1) a1=0.0;
		if (a2==0) a2=50;
	} else if (strncmp(f_type,"lp",2)) {
		strcpy(fs,"fflp");
	} else if (strncmp(f_type,"hp",2)) {
		if (a1==-1) a1 = 0.05;
		strcpy(fs,"ffhp");
	} else {
		fprintf(stderr, "ERROR: h_realfilt: unknown filter type %s\n", f_type);
		return NULL;
	}
	if (a1==-1) a1=0.1;
	if (a2==0) a2=1;

	if(!(h1 = c_swap(h0))) return NULL;	/* make matrix complex */
	c_swap(h1);

	h_fft(h1, 1, 1.0);			/* forward FFT, no rescaling */
	h_fourfilt(h1, a1, a2, fs);
	h_fft(h1, -1, -1.0);		/* inverse FFT, rescale by mx. size */
	c_real(h1);					/* leave just real part on stack */

	return h1;
} /* h_realfilt */
