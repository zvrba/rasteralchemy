#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-hcomp.cc,v 1.1.2.3 2003/12/31 15:25:18 zvrba Exp $";

#define ADD  0           /* keytypes for image operation */
#define SUB  1
#define MUL  2
#define DIV  3
#define POW  4
#define EXP  5
#define SIN  6
#define ASIN 7
#define TAN  8
#define ATAN 9
#define LOG  10
#define INVT 11
#define ABSV 12
#define INTG 13
#define DIFF 14
#define DIF2 15
#define COM  16
#define COMN 17
#define RMAG 18
#define FLOO 19
#define CEIL 20
#define MODU 21
#define DISC 22

/* -------------------------------------------------------------------
 *  histeq() -- Generate histogram of data array im1, size xsize, ysize.
 *  PTYPE frac         equalization strength fraction (0 = no eq, 1=full)
 *  global: histbins   number of histogram bins
 * -------------------------------------------------------------------
 */
hfield *histeq(hfield *h1, PTYPE frac)
{
	hfield *h2;
	D tmp, range, fpart, linpart;
	D hmin, hmax;                  /* extrema of HF */
	int xsize, ysize;                /* dimension of this hf */
	int ix, iy, in;
	size_t memsize;
	PTYPE *trans;
	PTYPE *hf;                     /* working heightfield array */
	long * hist;                   /* histogram array */
	int bins;                      /* number of bins */

	if (h1->c) {
		fprintf(stderr, "ERROR: histeq: matrix is complex.\n");
		return NULL;
	}
	xsize = h1->xsize;
	ysize = h1->ysize;
	hf = h1->a;              /* pointer to HF array */
	hmin = h1->min;
	hmax = h1->max;
	range = hmax - hmin;
	if(!(h2 = h_newr(xsize,ysize))) return NULL;

	bins = HF_PARAMS.histbins;	/* global histbins variable */
	memsize = (bins+1) * sizeof(long);   /* allocate histogram array */
	hist = (long *) malloc(memsize);
	memset(hist,0,memsize);          /* clear hist[x] */
	memsize = (bins+1) * sizeof(PTYPE);
	trans = (PTYPE *) malloc( memsize);
	if(!hist || !trans) {
		perror("ERROR: histeq: malloc");
		h_delete(h2);
		return NULL;
	}

	/* ------------ Normalize data, generate histogram -------- */

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			tmp = (double)El(hf,ix,iy);
			if((tmp > hmax) || (tmp < hmin)) {
				fprintf(stderr, "ERROR: histeq: HF min/max inaccurate.\n");
				return NULL;
			}
			tmp = (tmp-hmin)/range;          /* normalize data 0..1 */
			El(hf,ix,iy) = tmp;
			hist[(int)(bins*tmp)]++;   /* increment this histogram bin */
		}
	}
   
	tmp = 0;
	for (ix = 0; ix < bins; ix++) {  /* integrate histogram for xfer func */
		trans[ix] = tmp;
		tmp += hist[ix];
	}
	trans[bins] = tmp;
	for (ix = 0; ix <= bins; ix++) {  /* normalize xfer function: range 0..1 */
		trans[ix] /= tmp;              
		linpart = (D)ix/bins;           /* linear transfer function */
		trans[ix] = linpart + frac*(trans[ix]-linpart);
	}

	/* print out transfer function */
	/*
	  if (bins < 21) {              
	  printf("Transfer function:\n");
	  for (ix = 0; ix <= bins; ix++)
      printf("%d: %1.3f\n",ix,trans[ix]);
	  }
	*/
	fpart = 0;
	for (iy = 0; iy<ysize; iy++) {     /* do the equalization */
		for (ix = 0; ix<xsize; ix++) {
			tmp = (D)El(hf,ix,iy);
			in = (int)(tmp * (D) bins);	/* integer index */
			if (in > (bins-1)) in = bins-1;
			fpart = (tmp * (D)bins) - (D)in;       /* fractional part */
			tmp = INTERPOLATE(trans[in],trans[in+1],fpart);
			El(h2->a,ix,iy) = (range * tmp) + hmin;
		}
	}

	free(trans);
	free(hist);

	h_minmax(h2);
	return h2;
}

/* generate histogram: pass in # of bins desired and pointer to array
   with result. hh_hist allocates memory for the array itself, it's up
   to the calling procedure to free() it.
 */
static int hh_hist(hfield *hfin, int bins, long **hist_p)  
{
	int ix,iy,idx;
	int xsize, ysize;
	PTYPE *hf, tmp;
	PTYPE hmin,hmax,scalefac;
	size_t memsize;
	long *hist;

	memsize = bins * sizeof(long);   /* allocate histogram array */
	if(!(hist = (long *) malloc(memsize))) {
		perror("ERROR: hh_hist: malloc");
		return 0;
	}
	memset(hist,0,memsize);   /* clear hist[x] */

	xsize = hfin->xsize;
	ysize = hfin->ysize;
	hf = hfin->a;

	hmin = hfin->min;
	hmax = hfin->max;
	if(hmin == hmax) {
		fprintf(stderr, "ERROR: hh_hist: constant matrix (%3.5f)\n", hmin);
		return 0;
	}
	scalefac = 0.999999/(hmax - hmin);

	/* -------- generate histogram -------- */

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			tmp = (El(hf,ix,iy) - hmin)*scalefac; /* normalize 0..1 */
			if ((tmp > 1.0)||(tmp < 0)) {
				fprintf(stderr, "ERROR: hh_hist: min/max inaccurate\n");
				return 0;
			}
			idx = (int)(bins*tmp);
			if (idx >= bins) {
				fprintf(stderr, "INTERNAL ERROR: hh_hist: index=%d out of range\n",idx);
				abort();
			}
			hist[idx]++;   /* increment this histogram bin */
		}
	}

	hist_p[0] = hist;
	return 1;
}

void h_hist(hfield *hfin, int bins)                  /* display a histogram */
{
	int i,x,y;
	int screenx,screeny;
	long *hist = (long*)NULL;  /* histogram array */
	long hf_max;
	D sfac;
 
	screenx = 79;
	screeny = 20;
	if(!hh_hist(hfin, screenx, &hist)) return;   /* error in hist. generation? */
	hf_max = 0;
	for (i=0;i<screenx;i++) {
		if (hist[i]>hf_max) hf_max = hist[i];
	}
	if(hf_max == 0) {
		fprintf(stderr, "ERROR: h_hist: histogram has no elements!\n");
		free(hist);
		return;
	}

	sfac = (float)screeny / (float) hf_max;
	printf("%ld ---------------------------------"
		   "-------------------------------------\n",
		   hf_max);
	/* hokey (but portable) text-mode graphics display */
	for (y=screeny;y>0;y--) {
		for (x=0;x<screenx;x++) {
			if (hist[x]*sfac >= ((float)y-0.5)) printf("*");
			else printf(" ");
		}
		printf("\n");
	}
	printf("%1.2e -----------------------------------------------------------"
		   " %1.2e\n",hfin->min,hfin->max);
	free(hist);
}

/* add a constant to current HF such that the most-represented elevation value
 * is the SHIFT value, +/- half a bin width.
 */
hfield *h_hshift(hfield *hf, PTYPE shift)
{
	int i,idx,bins;
	long *hist = (long*)NULL;  /* histogram array */
	long hf_max;
	PTYPE hpeak; /* HF elevation value (+/-.5 bin size) of greatest population */

	bins = HF_PARAMS.histbins;
	if(!hh_hist(hf, bins, &hist)) {
		fprintf(stderr, "ERROR: h_hshift: histogram function failed.\n");
		return NULL;
	}

	hf_max = 0;
	idx = 0;            /* unnecessary but GCC complains about uninitialized...*/
	for (i=0;i<bins;i++) {
		if (hist[i]>hf_max) {
			hf_max = hist[i];
			idx = i;
		}
	}
	free(hist);           /* now done with the histogram array */
	if(hf_max == 0) {
		fprintf(stderr, "ERROR: h_hshift: histogram has no elements!\n");
		return NULL;
	}
	hpeak = ((PTYPE)idx * ((hf->max - hf->min)/bins)) + hf->min;
	//printf("Peak value is %f.\n",hpeak);
	return h_oneop(hf, "+",shift-hpeak,0);
}

hfield *negate(hfield *hf)
{ 
	return norm(hf, hf->max, hf->min);
}

hfield *norm(hfield *hfin, PTYPE min, PTYPE max)
{
	int ix,iy;
	int xsize, ysize;
	PTYPE scalefac, offset;
	PTYPE *hf;
	PTYPE hmin, hmax, tmp;

	hf = hfin->a;
	xsize = hfin->xsize;
	ysize = hfin->ysize;
	if(hfin->max == hfin->min) {
		fprintf(stderr, "ERROR: norm: cannot normalize constant matrix (min = max).\n");
		return NULL;
	}
	if(hfin->c) fprintf(stderr, "WARNING: norm: normalizing real part only.\n");
	hmin = FLT_MAX;
	hmax = FLT_MIN;
	offset = -hfin->min;
	scalefac = (max-min) / (hfin->max - hfin->min);
	for (ix=0;ix<xsize;ix++) {
		for (iy=0;iy<ysize;iy++) {
			tmp = (El(hf,ix,iy) + offset) * scalefac + min;
			El(hf,ix,iy) = tmp;
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		}
	}

	hfin->max = hmax;
	hfin->min = hmin;
	return hfin;
}

/* combine two HFs with an offset xo,yo of the (smaller?) X into Y */
hfield *h_composit(hfield *h1, hfield *h2, char *opn, int xo, int yo) /* HF comb. taking two operands */
{
	int op;
	int cflag;
	int xsize,ysize;      /* for Im() operator */
	int xsize1, ysize1;   /* HF X dimensions */
	int xsize2, ysize2;   /* HF Y dimensions */
	hfield *h3;
	int ix, iy;
	int xx, yy;
	int tile;               /* whether to do a tiling operation */
	D ht1,ht2,ht3;      /* temp HF values */
	D sval;             /* horizontal line integral */

	xsize = xsize1 = h1->xsize;      /* X hf */
	xsize = ysize1 = h1->ysize;
	xsize2 = h2->xsize;      /* Y hf */
	ysize2 = h2->ysize;
	if((xsize1 > xsize2) || (ysize1 > ysize2)) {
		fprintf(stderr, "ERROR: h_composit: %s: X matrix dim's must equal to or less than Y dim.\n",opn);
		return NULL;
	}
	if(h1->c && h2->c) {
		fprintf(stderr, "ERROR: h_composit: %s: can't handle two complex operands\n",opn);
		return NULL;
	}
	cflag = (h1->c || h2->c);  /* if new matrix will be complex */
	if (cflag)
		if(xsize1!=xsize2 || ysize1!=ysize2) {
			fprintf(stderr, "ERROR: h_composit: %s: real & complex matrices of different size.\n",opn);
			return NULL;
		}

	tile = h_tilable(h2,0);          /* TRUE if Y matrix is tilable */
  
	if (cflag) {
		if(!(h3 = h_newc(xsize2,ysize2))) return NULL;
	} else {
		if(!(h3 = h_newr(xsize2,ysize2))) return NULL;
	}

	if ((xo >= xsize2) || (yo >= ysize2)) {
		fprintf(stderr, "ERROR: h_composit: %s: offset is larger than HF dimension.\n",opn);
		return NULL;
	}
  
	if (strcmp(opn,"add")==0) op = ADD;
	else if (strcmp(opn,"sub")==0) op = SUB;
	else if (strcmp(opn,"mul")==0) op = MUL;
	else if (strcmp(opn,"div")==0) op = DIV;
	else if (strcmp(opn,"exp")==0) op = EXP;
	else if (strcmp(opn,"rmag")==0) op = RMAG;   /* complex magnitude */
	else if (strncmp(opn,"comneg",4)) op = COMN;
	else if (strncmp(opn,"com",3)) op = COM;
	else {
		fprintf(stderr,"ERROR: h_composit: unknown operation type %s.\n",opn);
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
		sval = El1(h1->a,0,iy);              /* initial value for integration */
		for (ix = 0; ix < xsize1; ix++) {
			xx = ix+xo;                    /* xo = x offset */
			if (tile && xx>=xsize2) xx -= xsize2;
			ht1 = El1(h1->a,ix,iy);
			if (xx<xsize2 && yy<ysize2) {
				ht2 = El2(h2->a,xx,yy);
				if      (op==ADD) ht3 = ht2 + ht1;
				else if (op==SUB) ht3 = ht2 - ht1;
				else if (op==MUL) ht3 = ht2 * ht1;
				else if (op==RMAG) ht3 = sqrt(ht1*ht1 + ht2*ht2);
				else if (op==DIV) { 
					if (ht1 == 0) ht3=0;      /* arb. handling of div by 0 problem */
					else ht3 = ht2 / ht1;
				} else if (op==EXP) {
					if (ht1 < 0) ht1 = 0;
					if (ht2 < 0) ht3 = 0; 
					else ht3 = pow(ht2,ht1);
				} else if (op==COM) {
					if (ht1 > ht2) {
						ht3 = ht1;
					} else ht3 = ht2;
				} else if (op==COMN) {
					if (ht1 < ht2) {
						ht3 = ht1;
					} else ht3 = ht2;
				} else {
					printf("ERROR: h_composit: unknown opcode %d\n",op);
					return NULL;
				} /* end opn if-else  */
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

hfield *h_oneop(hfield *h1, char *opn, D fac, D fac2) /* single- HF ops */
{
	int op;
	int cflag;
	int tile;
	U xsize, ysize, ix, iy;
	D ht1, ht3=0;
	D sf2,sf3;              /* floor/ceiling added threshold scale factor */

	xsize = h1->xsize;
	ysize = h1->ysize;
	cflag = h1->c;
	tile = h_tilable(h1,0);  
	sf2 = (h1->max-fac); if (sf2 != 0) sf2 = fac2/sf2;
	sf3 = (fac-h1->min); if (sf3 != 0) sf3 = fac2/sf3;

	if (strcmp(opn,"+")==0) op = ADD;
	else if (strcmp(opn,"-")==0) op = SUB;
	else if (strcmp(opn,"*")==0) op = MUL;
	else if (strcmp(opn,"/")==0) op = DIV;
	else if (strcmp(opn,"^")==0) op = POW;
	else if (strcmp(opn,"pow")==0) op = POW;
	else if (strcmp(opn,"abs")==0) op = ABSV;  /* absolute value */
	else if (strcmp(opn,"sin")==0) op = SIN;
	else if (strcmp(opn,"log")==0) op = LOG;
	else if (strcmp(opn,"asin")==0) op = ASIN;
	else if (strcmp(opn,"tan")==0) op  = TAN;
	else if (strcmp(opn,"atan")==0) op = ATAN;
	else if (strncmp(opn,"invert",3)) op = INVT;
	else if (strncmp(opn,"floor",4)) op = FLOO;
	else if (strncmp(opn,"ceiling",4)) op = CEIL;
	else if (strncmp(opn,"modulus",3)) op = MODU;
	else if (strncmp(opn,"discrete",3)) op = DISC;
	else {
		fprintf(stderr, "ERROR: h_oneop: %s: unknown operation type.\n",opn);
		return NULL;
	}
	/* printf("operation is %s\n",opn);
	   printf("opcode is %d\n",op);
	*/

	if (0==strcmp(opn,"/") && fac==0) { 
		fprintf(stderr, "ERROR: h_oneop: /: division by zero is prohibited.\n");
		return NULL;
	}

	for (iy = 0; iy < ysize; iy++) {
		for (ix = 0; ix < xsize; ix++) {
			ht1 = El(h1->a,ix,iy);
			if (op==SIN) ht3 = sin(ht1);
			else if (op==ASIN) {
				if (ht1 < -1) ht1 = -1;
				if (ht1 > 1) ht1 = 1;
				ht3 = asin(ht1);
			}
			else if (op==TAN) { 
				if (cos(ht1)==0) ht3=0;
				else ht3 = tan(ht1);
			}
			else if (op==ATAN) ht3 = atan(ht1);
			else if (op==ABSV) ht3 = ABS(ht1);
			else if (op==POW) ht3 = SGN(ht1) * pow(ABS(ht1),fac);
			else if (op==ADD) ht3 = ht1+fac;
			else if (op==SUB) ht3 = ht1-fac;
			else if (op==MUL) ht3 = ht1*fac;
			else if (op==DIV) ht3 = ht1/fac;
			else if (op==DISC) ht3 = ((int)(ht1*fac-0.000001))/fac;
			else if (op==MODU) ht3 = ht1-SGN(ht1)*ABS(((int)(ht1/fac))*fac);
			else if (op==FLOO) { 
				if (ht1 < fac) {
					if (fac2 > 0) {
						ht3 = fac + sf3*(1-1/(1+ht1-fac));
					} else ht3 = fac;
				} else ht3 = ht1;
			} else if (op==CEIL) { 
				if (ht1 > fac) {
					if (fac2 > 0) {
						ht3 = fac + sf2*(1-1/(1+ht1-fac));
					} else ht3 = fac;
				} else ht3 = ht1;
			} else if (op==INVT) { 
				if (ht1 == 0) ht3 = 0;
				else ht3 = 1.0/ht1;
			} else if (op==LOG) {
				if (ht1 <= 0.0) ht1 = 1.0;   /* arbitrary handling of (-inf) */
				ht3 = log(ht1);
			} else {
				fprintf(stderr, "ERROR: h_oneop: unrecognized opcode.\n");
				return NULL;
			}

			El(h1->a,ix,iy) = ht3;
		}  /* end for ix */
	}  /* end for iy */

	h_minmax(h1);
	return h1;
}

hfield *h_diff(hfield *h0, const char *opn)
{
	int op;
	int tile;
	int xsize, ysize;
	hfield *h1;
	int ix, iy;
	D ht0, ht1=0;
	D tmp1,tmp2;

	xsize = h0->xsize;
	ysize = h0->ysize;
	if(h0->c) {
		fprintf(stderr, "ERROR: h_diff: matrix is complex.\n");
		return NULL;
	}
	tile = h_tilable(h0,0);  

	if (strcmp(opn,"diff")==0) op = DIFF;
	else if (strcmp(opn,"dif2")==0) op = DIF2;
	else {
		fprintf(stderr, "ERROR: h_diff: %s: unknown operation type.\n",opn);
		return NULL;
	}
	if(!(h1 = h_newr(xsize,ysize))) return NULL;
	for (iy = 0;iy<ysize;iy++) {
		for (ix=0;ix<xsize;ix++) {
			ht0 = El(h0->a,ix,iy);
			if (op==DIFF) {
				if (tile)
					ht1 = sqrt(pow(ht0-Elmod(h0->a,ix-1,iy),2)
							   + pow(ht0-Elmod(h0->a,ix,iy-1),2));
				else
					ht1 = sqrt(pow(ht0-Elclip(h0->a,ix-1,iy),2)
							   + pow(ht0-Elclip(h0->a,ix,iy-1),2));
			}
			else if (op==DIF2) {
				if (tile) {
					tmp1 = (Elmod(h0->a,ix-1,iy)+Elmod(h0->a,ix+1,iy))/2;
					tmp2 = (Elmod(h0->a,ix,iy-1)+Elmod(h0->a,ix,iy+1))/2;
				} else {
					tmp1 = (Elclip(h0->a,ix-1,iy)+Elclip(h0->a,ix+1,iy))/2;
					tmp2 = (Elclip(h0->a,ix,iy-1)+Elclip(h0->a,ix,iy+1))/2;
				}
				ht1 = sqrt(pow(ht0-tmp1,2)+pow(ht0-tmp2,2));
			}
			El(h1->a,ix,iy) = ht1;
		} /* for ix */
	} /* for iy */
  
	h_minmax(h1);
	return h1;
}
		 
static int h_slope2(hfield *h1, hfield *h0,int op,int tile, D thresh, int iter)
{
	int xsize, ysize;
	int ix, iy;
	int changed;
	D ht0, ht1=0;
	D htx,hty,htx1,hty1;   /* values of HF elements on either side of this one */
  
	xsize = h0->xsize;
	ysize = h0->ysize;
	changed = 0;   /* number of elements averaged this pass */
  
	for (iy = 0;iy<ysize;iy++) {
		for (ix = 0;ix<xsize;ix++) {
			ht0 = El(h0->a,ix,iy);
			if (op==DIFF) {       /* calculate mag(x,y) slope at ix,iy */
				if (tile) {htx =  Elmod(h0->a,ix-1,iy); hty= Elmod(h0->a,ix,iy-1);}
				else    {htx = Elclip(h0->a,ix-1,iy); hty=Elclip(h0->a,ix,iy-1);}
				ht1 = sqrt(pow((ht0-htx),2) + pow((ht0-hty),2) );
				if (ht1 > thresh) {  /* yes, slope exceeds threshold */
					if (tile) {htx1 =  Elmod(h0->a,ix+1,iy); hty1= Elmod(h0->a,ix,iy+1);}
					else    {htx1 = Elclip(h0->a,ix+1,iy); hty1=Elclip(h0->a,ix,iy+1);}
					ht1 = (htx + htx1 + hty + hty1)/4.0;
					changed++;
				} else ht1 = ht0;
			} else if (op==DIF2) {
				if (tile) {
					htx =  Elmod(h0->a,ix-1,iy); hty= Elmod(h0->a,ix,iy-1);
					htx1 =  Elmod(h0->a,ix+1,iy); hty1= Elmod(h0->a,ix,iy+1);
				} else {
					htx = Elclip(h0->a,ix-1,iy); hty=Elclip(h0->a,ix,iy-1);
					htx1 = Elclip(h0->a,ix+1,iy); hty1=Elclip(h0->a,ix,iy+1);
				}
				ht1 = sqrt(pow(ht0-(htx+htx1)/2,2)+pow(ht0-(hty+hty1)/2,2));
				if (ht1 > thresh) {
					ht1 = (htx + htx1 + hty + hty1)/4.0;
					changed++;
				} else ht1 = ht0;
			}
			El(h1->a,ix,iy) = ht1;
		} /* for ix */
	} /* for iy */
	return(changed);
}

hfield *h_slopelim(hfield *h0, const char *opn,D thresh, int iter)  /* slope-dependent smoothing */
{
	int op;
	int tile;
	int xsize, ysize;
	hfield *h1;
	int repcount=0;
	int changed;

	xsize = h0->xsize;
	ysize = h0->ysize;
	if(h0->c) {
		fprintf(stderr, "ERROR: h_slopelim: matrix is complex.\n");
		return NULL;
	}
	tile = h_tilable(h0,0);  

	if (strncmp(opn,"lslope",3)) op = DIFF;
	else if (strncmp(opn,"lcurve",3)) op = DIF2;
	else {
		fprintf(stderr, "ERROR: h_slopelim: %s: unknown operation type.\n",opn);
		return NULL;
	}
	if(!(h1 = h_newr(xsize, ysize))) return NULL;

	do {
		h_slope2(h1,h0,op,tile,thresh,iter);
		changed = h_slope2(h0,h1,op,tile,thresh,iter);
	} while (changed > 0  &&  ++repcount<iter);

	h_minmax(h1);
	return h1;
}

		 /* Find minimum and maximum values in current array. */
void h_minmax(hfield *hfin)
{
	PTYPE hmin, hmax, tmp;
	int ix,iy,xsize,ysize;
	PTYPE *hf = hfin->a;

	hmax = El(hf,0,0);    /* initialize extrema to first data value */
	hmin = El(hf,0,0);
	xsize = hfin->xsize;
	ysize = hfin->ysize;

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			tmp = (double)El(hf,ix,iy);
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		}
	}
	hfin->min = hmin;
	hfin->max = hmax;
}

void i_minmax(hfield *hf,D *min,D *max)
{
	D hmin, hmax, tmp;
	int ix,iy,xsize,ysize;
	PTYPE *h = hf->a;

	if(!hf->c) {
		*min=0; *max=0; return;
	}
	xsize = hf->xsize;
	ysize = hf->ysize;
	hmax = Im(h,0,0);    /* initialize extrema to first data value */
	hmin = Im(h,0,0);

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			tmp = (double)Im(h,ix,iy);
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		}
	}
	*min = hmin;
	*max = hmax;
	return;
}

void r_minmax(hfield *hf,D *min,D *max)
{
	D hmin, hmax, tmp;
	int ix,iy,xsize,ysize;
	PTYPE *h = hf->a;

	xsize = hf->xsize;
	ysize = hf->ysize;
	hmax = El(h,0,0);    /* initialize extrema to first data value */
	hmin = El(h,0,0);

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			tmp = (double)El(h,ix,iy);
			if (tmp > hmax) hmax = tmp;
			if (tmp < hmin) hmin = tmp;
		}
	}
	*min = hmin;
	*max = hmax;
}

/* is_tilable()  --  Returns 1 if current matrix will tile seamlessly */
/*                   Returns 0 if it won't  */
int is_tilable(hfield *hfin, int pflag)
{
	D v1,v2,v3,d1,d2;
	D xdiff, ydiff, range;
	int ix,iy,xsize,ysize;
	PTYPE *hf;
	int retval;

	if (pflag==0) {
		if (HF_PARAMS.tile_mode == TON) return(1);
		if (HF_PARAMS.tile_mode == TOFF) return(0);
	}

	hf = hfin->a;
	xsize = hfin->xsize;
	ysize = hfin->ysize;
	range = (hfin->max - hfin->min);
	if (range==0) range=1;

	xdiff = ydiff = 0;
	for (iy = 0; iy<ysize; iy++) { /* horizontal edge */
		v1 = El(hf,xsize-1,iy);
		v2 = El(hf,0,iy);
		v3 = El(hf,1,iy);
		d1 = sqrt(pow(v1-v2,2));   /* difference across edge */
		d2 = sqrt(pow(v3-v2,2));   /* difference near edge */
		ydiff += d1-d2;
	}
	ydiff /= (D)ysize * range;
	if (pflag != 0) printf("horizontal difference = %5.3e\n",(float)ydiff);

	for (ix = 0; ix<xsize; ix++) {
		v1 = El(hf,ix,ysize-1);
		v2 = El(hf,ix,0);
		v3 = El(hf,ix,1);
		d1 = sqrt(pow(v1-v2,2));   /* difference across edge */
		d2 = sqrt(pow(v3-v2,2));   /* difference near edge */
		xdiff += d1-d2;
	}
	xdiff /= (D) xsize*range;
	if ((ABS(xdiff) < HF_PARAMS.tile_tol) && 
		(ABS(ydiff) < HF_PARAMS.tile_tol)) retval=1;
    else retval = 0;
	if (pflag != 0) {
		printf("vertical difference = %5.3e\n",(float)xdiff);
		printf("tile tolerance %5.3e\n",(float)HF_PARAMS.tile_tol);
		if (retval) printf("HF should be tilable.\n");
		else printf("HF appears not to tile.\n");
	}
	return(retval);
}  /* is_tilable() */


/* h_tilable()  --  Returns 1 if current matrix will tile seamlessly */
/*                   Returns 0 if it won't  */
int h_tilable(hfield *hfin, int pflag)
{
	D v1,v2,v3,d1,d2;
	D xdiff, ydiff, range;
	int ix,iy,xsize,ysize;
	int retval;
	PTYPE *hf = hfin->a;

	if (pflag==0) {
		if (HF_PARAMS.tile_mode == TON) return(1);
		if (HF_PARAMS.tile_mode == TOFF) return(0);
	}

	xsize = hfin->xsize;
	ysize = hfin->ysize;
	range = (hfin->max - hfin->min);
	if (range==0) range=1;

	xdiff = ydiff = 0;
	for (iy = 0; iy<ysize; iy++) { /* horizontal edge */
		v1 = El(hf,xsize-1,iy);
		v2 = El(hf,0,iy);
		v3 = El(hf,1,iy);
		d1 = sqrt(pow(v1-v2,2));   /* difference across edge */
		d2 = sqrt(pow(v3-v2,2));   /* difference near edge */
		ydiff += d1-d2;
	}
	ydiff /= (D)ysize * range;
	if (pflag != 0) printf("horizontal difference = %5.3e\n",(float)ydiff);

	for (ix = 0; ix<xsize; ix++) {
		v1 = El(hf,ix,ysize-1);
		v2 = El(hf,ix,0);
		v3 = El(hf,ix,1);
		d1 = sqrt(pow(v1-v2,2));   /* difference across edge */
		d2 = sqrt(pow(v3-v2,2));   /* difference near edge */
		xdiff += d1-d2;
	}
	xdiff /= (D) xsize*range;
	if ((ABS(xdiff) < HF_PARAMS.tile_tol) &&
		(ABS(ydiff) < HF_PARAMS.tile_tol)) retval=1;
    else retval = 0;
	if (pflag != 0) {
		printf("vertical difference = %5.3e\n",(float)xdiff);
		printf("tile tolerance %5.3e\n",(float)HF_PARAMS.tile_tol);
		if (retval) printf("HF should be tilable.\n");
		else printf("HF appears not to tile.\n");
	}
	return(retval);
}


hfield *h_peak(hfield *hf0, D xpos, D ypos)             /* slew HF to locate peak at x,y */
{
	int xsize, ysize;
	int xpeak, ypeak, xstart, ystart;
	int x,y,xx,yy;
	D tmp,hmax;
	hfield *hf1;

	xsize = hf0->xsize;
	ysize = hf0->ysize;
	if(hf0->c) {
		fprintf(stderr, "ERROR: peak: real matrices only.\n");
		return NULL;
	}

	if(!(hf1 = h_newr(xsize,ysize))) return NULL;
	hf1->min = hf0->min;  
	hf1->max = hf0->max;

	xpeak = xsize/2;          /* should not be necessary, i hope */
	ypeak = ysize/2;
	hmax = FLT_MIN;

	for (y=0;y<ysize;y++) {       /* find coordinate of peak */
		for (x=0;x<xsize;x++) {
			tmp = El(hf0->a,x,y);
			if (tmp > hmax) {
				hmax = tmp;
				xpeak = x;
				ypeak = y;
			}
		}
	}
	xstart = xpeak - (int)((D)xsize * xpos);
	ystart = ypeak - (int)((D)ysize * ypos);

	/* copy new array into new position */
	for (y=0;y<ysize;y++) {       
		yy = (ysize + y + ystart) % ysize;
		for (x=0;x<xsize;x++) {
			xx = (xsize + x + xstart) % xsize;
			El(hf1->a,x,y) = El(hf0->a,xx,yy);
		}
	}

	h_minmax(hf1);
	return hf1;
}

hfield *h_rotate(hfield *hf1, int deg)              /* rotate 90,180,270 degrees */
{
	int xsize, ysize;
	int xsize2, ysize2;
	int x,y;
	hfield *hf2;

	if((deg!=90)&&(deg!=180)&&(deg!=270)) {
		fprintf(stderr, "ERROR: rotate: only supports 90, 180, or 270 degree rotations.\n");
		return NULL;
	}
	xsize = hf1->xsize;
	ysize = hf1->ysize;
	if (hf1->c) {
		printf("rotate only works on real matrices\n");
		return NULL;
	}

	if (deg==180) {
		xsize2 = xsize;
		ysize2 = ysize;
	} else {
		xsize2 = ysize;
		ysize2 = xsize;
	}
	if(!(hf2 = h_newr(xsize2,ysize2))) return NULL;
	hf2->min = hf1->min;
	hf2->max = hf1->max;

	if (deg==90) {
		for (y=0;y<ysize;y++) {
			for (x=0;x<xsize;x++) {
				El2(hf2->a,ysize-1-y,x) = El(hf1->a,x,y);
			}
		}
	} else if (deg==270) {
		for (y=0;y<ysize;y++) {
			for (x=0;x<xsize;x++) {
				El2(hf2->a,y,xsize-1-x) = El(hf1->a,x,y);
			}
		}
	} else if (deg==180) {
		for (y=0;y<ysize;y++) {
			for (x=0;x<xsize;x++) {
				El2(hf2->a,xsize-1-x,ysize-1-y) = El(hf1->a,x,y);
			}
		}
	} else {
		fprintf(stderr, "ERROR: rotate: only supports 90, 180, or 270 degree rotations.\n");
		return NULL;
	}

	h_minmax(hf2);
	return hf2;
}

hfield *h_zero(int xsize, int ysize)    /* create a new blank HF */
{
	hfield *hf;
	if(!(hf = h_newr(xsize,ysize))) return NULL;
	memset(hf->a, 0, xsize*ysize*sizeof(PTYPE));
	return hf;
}


hfield *h_const(int xsize, int ysize, D value)  /* create a new constant HF */
{        
	hfield *hf;
	int x,y;
 
	if(!(hf = h_newr(xsize,ysize))) return NULL;
	for (x=0;x<xsize;x++) for (y=0;y<ysize;y++) El(hf->a,x,y)=value;
	hf->min = hf->max = value;
	return hf;
}

