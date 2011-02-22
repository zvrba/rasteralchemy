//-*- C++ -*-
/* definitions and macros for HF-Lab     January 1996  John P. Beale */
/* Cleaned-up and adapted for LUA in 2004 by Zeljko Vrba
 * <mordor@fly.srk.fer.hr>
 */
#ifndef HF_HL_H__
#define HF_HL_H__

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define FALSE 0
#define TRUE 1

/* tiling modes */
#define AUTO 0
#define TOFF 1
#define TON  2

#define D double
#define U unsigned int
#define PTYPE float   /* data-type of heightfield (pixel) values */ 

struct hfield {					/* Heightfield structure type */
	PTYPE *a;					/* 2-D array of values */
	U xsize;
	U ysize;					/* x- and y-dimensions of array */
	PTYPE min;
	PTYPE max;					/* max and min values in array */
	int c;				/* TRUE if matrix is complex, FALSE if real */

#ifdef __cplusplus				// Lua scripting
	bool operator==(const hfield &hf) const {
		return a == hf.a;
	}

	const char *_type() const {
		return "hfield";
	}

	unsigned long _hkey() const {
		return (unsigned long)a;
	}
#endif
};

extern struct HF_PARAMS {
	int rand_gauss;				/* if gaussian rand# should be used */
	int rnd_seed_stale;			/* no random seed yet? */
	int rnd_seed;			   /* random seed to use (if not stale) */
	int histbins;				/* histogram bins */
	int tile_mode;				/* tiling mode: on/off/auto */
	D   tile_tol;				/* tiling edge threshold tolerance */
	D   gaufac;					/* sigmas along gaussian */
	
#ifdef __cplusplus				// Lua scripting
	const char *_type() const {
		return "hfield";
	}
#endif
} HF_PARAMS;

#define ABS(a)          (((a)<0) ? -(a) : (a))
#define ROUND(a)        ((a)>0 ? (int)((a)+0.5) : -(int)(0.5-(a)))
#define ZSGN(a)         (((a)<0) ? -1 : (a)>0 ? 1 : 0)  
#define SGN(a)          (((a)<0) ? -1 : 1)
#define MIN(x,y)        (((x)<(y)) ? (x) : (y) )
#define MAX(x,y)        (((x)>(y)) ? (x) : (y) )
#define ATAN2(y,x)      ((x==0 && y==0)?0:atan2(y,x))
#define INTERPOLATE(x,y,frac)  (1-(frac))*(x) + (frac)*(y)

/* ------------------- hl.c -------------------------- */
hfield *h_newr(int xs, int ys);
hfield *h_newc(int xs, int ys);
void h_delete(hfield*);
//void h_assign_free(hfield *dst, hfield *src);

/* --- hcomp.c -------------------------------------- */
hfield *h_oneop(hfield *h1, char *opn, D fac, D fac2); /* single- HF ops */
hfield *h_composit(hfield *h1, hfield *h2, char *opn, int xo, int yo); /* HF comb. taking two operands */
void h_minmax(hfield *hfin);                       /* set min,max values of array */
void i_minmax(hfield *hf,D *mx,D *mn);  /* find min,max of imag.vals */
void r_minmax(hfield *hf,D *mx,D *mn);  /* find min,max of real.vals */
hfield *histeq(hfield *h1, PTYPE frac); /* histogram equalization */
void h_hist(hfield *hfin, int bins);                  /* display a histogram */
hfield *h_hshift(hfield *hf, PTYPE shift); /* offset HF so hist. peak at SHIFT */
hfield *norm(hfield *hfin, PTYPE min, PTYPE max); /* normalize to [min..max] */
hfield *negate(hfield *hf); /* invert values in matrix */
hfield *h_slopelim(hfield *h0, const char *opn,D thresh, int iter);  /* slope-dependent smoothing */
hfield *h_diff(hfield *h0, const char *opn); /* 1st or 2nd differential of HF */

hfield *h_rotate(hfield *hf1, int deg);              /* rotate 90,180,270 degrees */
hfield *h_peak(hfield *hf0, D xpos, D ypos);             /* slew HF to locate peak at x,y */
int is_tilable(hfield*, int flag);             /* true if image tiles ok */
int h_tilable(hfield *h, int f); /* 1 if HF tiles seamlessly */
hfield *h_zero(int xsize, int ysize);    /* create a new blank HF */
hfield *h_const(int xsize, int ysize, D value);  /* create a new constant HF */

/* ----- cplx.c functions --------------------------------- */
hfield *c_swap(hfield *hfin); /* swap complex and real parts of matrix */
hfield *c_join(hfield *hfr, hfield *hfi); /* make complex from real & imag parts of matrix */
int c_split(hfield *hfc, hfield **hfr, hfield **hfi);  /* divide complex into real & imag parts */
hfield *c_real(hfield *hf);  /* take real part of array */
hfield *c_mag(hfield *hfc);		/* take magnitude of complex matrix */
hfield *c_diff(hfield *hf); /* make complex from x & y slope of matrix */
hfield *c_convert(hfield *hfc, int dir);	/* convert complex matrix Rect <-> Polar */


/* --- rand.c --------------------------------------------------- */
float ran1(void);		   /* return a single [0..1] float random # */
void seed_ran1(int seed);		/* use a single integer seed value */

/* --- ops.c --------------------------------------------------- */
hfield *gforge(int size, float h); /* generate terrain */
hfield *fillarray(int xsize, int ysize, float h); /* gen 1/f noise */
hfield *h_fft(hfield *hf, int dir, D scaling); /* complex FFT / IFFT routine */
hfield *gen_rand(int xsize, int ysize); /* generate random array */
void initgauss(void); /* initialize rand #/gauss gen */
double gauss(void); /* return gaussian rand# */
double gaussn(void); /* returns gaussian dist. rand # between 0 and 1 */
hfield *h_fourfilt(hfield *hf, D center, D Q, const char *f_type); /* filter real & imag. matrix */
void f_filter(PTYPE *real, PTYPE *imag, int xsize, int ysize, 
	      double center, double Q, int f_type);
hfield *h_realfilt(hfield *hf, D a1, D a2, const char *f_type); /* filter a real matrix */

hfield *yslope(hfield *hfin, D yfrac, D y_sfac, D slope_exp);
hfield *h_gauss(hfield *hfin, D xfrac, D yfrac, D radfac, D hscale); /* add gaussian hill */
hfield *h_ring(hfield *hfin, D xfrac, D yfrac, D radfac, D width, D hscale); /* add a ring */
hfield *h_crater(hfield *h0, int how_many, D ch_scale, D radius, D dfac);
hfield *smooth(hfield *h0, D frac); /* smooth heightfield */

/* ---------- ops2.c ------------------------------------------ */
hfield *h_join(hfield *h1, hfield *h2, int f); /* join two HFs joined together 0=vert 1=horiz */
hfield *h_double(hfield *h1, D f, D f2); /* double res. with midpoint-disp interpolation */
hfield *h_half(hfield *h1); /* halve resolution by averaging groups of four pixels */
hfield *rescale(hfield *h0, int xsize2, int ysize2); /* rescale heightfield */
hfield *h_clip(hfield *h1, int x1, int y1, int x2, int y2);  /* clip out rectangular subarray */
hfield *h_warp(hfield *h1, hfield *h2, int opn, D xcent, D ycent, D scalefac,int xsize3,int ysize3); /* radial map xform */
hfield *h_cwarp(hfield *h1, hfield *h2, int opn, D scalefac);  /* warp with complex control */
hfield *h_zedge(hfield *hf, D frac, D pwr);        /* set edges of HF to zero */
//int h_sample(int av, char ac[MAXTOK][TOKLEN]);
            /* print value of matrix at x,y */
hfield *h_nsmooth(hfield *h0, int iter, D th1, D th2);  /* slope-dependent smoothing */

/* ---------- crater.c --------------------------------------- */

int distribute_craters(PTYPE *real, PTYPE *imag, int xsize, int ysize,
 unsigned int how_many, int wrap, double ch_scale, double cr_scale,
 double dfac);

/* ------- erode.c --------------------------------- */
hfield *h_fillb(hfield *h1, int imax, D rate);  /* fill basin imax times */
hfield *h_find_ua(hfield *h1);                /* find uphill area */

/* ------ hcon.h -----------------------------------   jpb 7/15/95 */

#define PTYPE float     /* internal representation of float numbers */
#define BYTE unsigned char
#define U unsigned int

#define Boolean int
#define FALSE 0
#define TRUE 1

#define El(vv, xq, yq)  vv[((yq) * xsize) + xq]  /* elmt of array */
#define El1(vv, xq, yq) vv[((yq) * xsize1)+ xq]  /* elmt of 1st array */
#define El2(vv, xq, yq) vv[((yq) * xsize2)+ xq]  /* elmt of 2nd array */
#define El3(vv, xq, yq) vv[((yq) * xsize3)+ xq]  /* elmt of 3rd array */
#define Im(vv, xq, yq)  vv[xsize*ysize + ((yq) * xsize) + xq] /* imag.part */
#define Im2(vv, xq, yq) vv[xsize2*ysize2 +((yq)*xsize2) + xq] /* imag.part */

#define Elmod(v,x,y) v[(((y)>=0)?(y)%ysize:(((y)%ysize)+ysize)%ysize)*xsize \
		     + (((x)>=0)?(x)%xsize:(((x)%xsize)+xsize)%xsize) ]
#define Elclip(v,x,y) v[(((y)<0?0:(((y)>=ysize)?(ysize-1):(y))) * xsize)\
		      + ((x)<0?0:(((x)>=xsize)?(xsize-1):(x)))]

#define X_WRAP(x)  x=(((x)>=0)?(x)%xsize:(((x)%xsize)+xsize)%xsize)
#define Y_WRAP(y)  y=(((y)>=0)?(y)%ysize:(((y)%ysize)+ysize)%ysize)

#define X_CLIP(x)  x=((x)<0?0:(((x)>=xsize)?(xsize-1):(x)))
#define Y_CLIP(y)  y=((y)<0?0:(((y)>=ysize)?(ysize-1):(y)))

#endif
