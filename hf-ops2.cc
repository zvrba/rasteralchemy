/* ops2.c   --  (more) heightfield operations
 *              Jan 22 1996   John P. Beale
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-ops2.cc,v 1.1.2.2 2003/12/31 15:25:18 zvrba Exp $";


#define ISQRT2 0.707106781188
#define PID2   1.5707963268

hfield *h_join(hfield *h1, hfield *h2, int f) /* join two HFs joined together 0=vert 1=horiz */
{
	int x,y;
	int xsize1, ysize1;   /* old1 HF */
	int xsize2, ysize2;   /* old2 HF */
	int xsize3, ysize3;   /* new HF */
	hfield *h3;

	if(h1->c || h2->c) {
		fprintf(stderr, "ERROR: join: matrix complex.\n");
		return NULL;
	}

	xsize1 = h1->xsize; ysize1 = h1->ysize;
	xsize2 = h2->xsize; ysize2 = h2->ysize;

	if (f) {
		xsize3 = xsize1 + xsize2;
		ysize3 = MAX(ysize1,ysize2);
	}
	else {
		xsize3 = MAX(xsize1,xsize2);
		ysize3 = ysize1 + ysize2;
	}
	if(!(h3 = h_newr(xsize3,ysize3))) return NULL;

	for (y=0;y<ysize1;y++) {
		for (x=0;x<xsize1;x++) {
			El3(h3->a,x,y) = El1(h1->a,x,y);
		}
	}
	if (f) {
		for (y=0;y<ysize2;y++) {
			for (x=0;x<xsize2;x++) {
				El3(h3->a,x+xsize1,y) = El2(h2->a,x,y);
			}
		}
	} else {
		for (y=0;y<ysize2;y++) {
			for (x=0;x<xsize2;x++) {
				El3(h3->a,x,y+ysize1) = El2(h2->a,x,y);
			}
		}
	}

	h_minmax(h3);
	return h3;
}

/*
#define X(x)  ((x)<0?0:(((x)>=xsize)?(xsize-1):(x)))
#define Y(Y)  ((y)<0?0:(((y)>=ysize)?(ysize-1):(y)))
*/

/* double() -- double resolution using modified-midpoint-displacement
	       algorithm; different versions for tiling and non-tiling
	       matrices.  <f> is local fractal scaling, <f2> is overall
	       fractal scaling.
 */
hfield *h_double(hfield *h1, D f, D f2) /* double res. with midpoint-disp interpolation */
{
	int x,y;
	int xsize,ysize;
	int xsize1, ysize1;   /* old1 HF */
	int xsize2, ysize2;   /* old2 HF */
	hfield *h2;
	PTYPE *hf1, *hf2;
	int tile;
	D sfac;
	D a,b,c,d;

	tile = is_tilable(h1, 0);

	hf1 = h1->a;
	xsize1 = h1->xsize;
	ysize1 = h1->ysize;
	if(h1->c) {
		fprintf(stderr, "ERROR: double: matrix is complex.\n");
		return NULL;
	}

	xsize2 = xsize1*2;
	ysize2 = ysize1*2;
	if (!tile) {          /* non-tiling: don't wrap at edges */
		xsize2--;
		ysize2--;
	}
	f2 /= sqrt((D)xsize2*ysize2);  /* normalize to matrix size */
  
	if(!(h2 = h_newr(xsize2,ysize2))) return NULL;
	hf2 = h2->a; 

	for (y=0;y<ysize1;y++) {      /* copy over initial values */
		for (x=0;x<xsize1;x++) {
			El2(hf2,x*2,y*2) = El1(hf1,x,y);
		}
	}
	if (!tile) {                  /* fill in edges */
		for (y=1;y<ysize2;y+=2) {
			El2(hf2,0,y)=El2(hf2,0,y-1);
			El2(hf2,xsize2-1,y)=El2(hf2,xsize2-1,y-1);
		}
		for (x=1;x<xsize2;x+=2) {
			El2(hf2,x,0) = El2(hf2,x-1,0);
			El2(hf2,x,ysize2-1) = El2(hf2,x-1,ysize2-1);
		}
	}

	initgauss();  /* seed rnd#gen, gauss routine */
	xsize = xsize2;
	ysize = ysize2;

	for (y=1;y<ysize2;y+=2) {      /* tiling and non-tiling interp. */
		for (x=1;x<xsize2;x+=2) {
			if (tile) {
				a = Elmod(hf2,x+1,y+1); b = Elmod(hf2,x-1,y+1);
				c = Elmod(hf2,x+1,y-1); d = Elmod(hf2,x-1,y-1);
			} else {
				a = Elclip(hf2,x+1,y+1); b = Elclip(hf2,x-1,y+1);
				c = Elclip(hf2,x+1,y-1); d = Elclip(hf2,x-1,y-1);
			}
			sfac = f2+f*(MAX(MAX(a,b),MAX(c,d)) - MIN(MIN(a,b),MIN(c,d)));
			El2(hf2,x,y) = (a+b+c+d)/4.0 + sfac*(gaussn()-0.5);
		}
	}

	for (y=0;y<ysize2;y+=2) {      
		for (x=1;x<xsize2;x+=2) {
			if (tile) {
				a = Elmod(hf2,x+1,y); b = Elmod(hf2,x-1,y);
				c = Elmod(hf2,x,y+1); d = Elmod(hf2,x,y-1);
			} else {
				a = Elclip(hf2,x+1,y); b = Elclip(hf2,x-1,y);
				c = Elclip(hf2,x,y+1); d = Elclip(hf2,x,y-1);
			}
			sfac = f2+f*(MAX(MAX(a,b),MAX(c,d)) - MIN(MIN(a,b),MIN(c,d)));
			El2(hf2,x,y) = (a+b+c+d)/4.0 + sfac*(gaussn()-0.5);
		}
	}

	for (y=1;y<ysize2;y+=2) {      
		for (x=0;x<xsize2;x+=2) {
			if (tile) {
				a = Elmod(hf2,x+1,y); b = Elmod(hf2,x-1,y);
				c = Elmod(hf2,x,y+1); d = Elmod(hf2,x,y-1);
			} else {
				a = Elclip(hf2,x+1,y); b = Elclip(hf2,x-1,y);
				c = Elclip(hf2,x,y+1); d = Elclip(hf2,x,y-1);
			}
			sfac = f2+f*(MAX(MAX(a,b),MAX(c,d)) - MIN(MIN(a,b),MIN(c,d)));
			El2(hf2,x,y) = (a+b+c+d)/4.0 + sfac*(gaussn()-0.5);
		}
	}
  
	for (y=0;y<ysize2;y+=2) {      /* backtrack and adjust original data */
		for (x=0;x<xsize2;x+=2) {
			if (tile) {
				a = Elmod(hf2,x+1,y+1); b = Elmod(hf2,x-1,y+1);
				c = Elmod(hf2,x+1,y-1); d = Elmod(hf2,x-1,y-1);
			} else {
				a = Elclip(hf2,x+1,y+1); b = Elclip(hf2,x-1,y+1);
				c = Elclip(hf2,x+1,y-1); d = Elclip(hf2,x-1,y-1);
			}
			sfac = 0.5*(f2+f*(MAX(MAX(a,b),MAX(c,d)) - MIN(MIN(a,b),MIN(c,d))));
			El2(hf2,x,y) = (a+b+c+d)/4.0 + sfac*(gaussn()-0.5);
		}
	}
  
	h_minmax(h2);
	return h2;
}



hfield *h_half(hfield *h1) /* halve resolution by averaging groups of four pixels */
{
	int x,y,xx,yy;
	int xsize1, ysize1;   /* old1 HF */
	int xsize2, ysize2;   /* old2 HF */
	hfield *h2;
	PTYPE *hf1, *hf2;

	hf1 = h1->a;
	xsize1 = h1->xsize;
	ysize1 = h1->ysize;
	if(h1->c) {
		fprintf(stderr, "ERROR: half: matrix is complex.\n");
		return NULL;
	}

	xsize2 = xsize1/2;
	ysize2 = ysize1/2;
  
	if(!(h2 = h_newr(xsize2,ysize2))) return NULL;
	hf2 = h2->a; 

	for (y=0;y<ysize2;y++) {      /* copy over initial values */
		yy = y * 2;
		for (x=0;x<xsize2;x++) {
			xx = x * 2;
			El2(hf2,x,y) = (El1(hf1,xx,yy)+El1(hf1,xx+1,yy)+
							El1(hf1,xx,yy+1)+El1(hf1,xx+1,yy+1))/4.0;
		}
	}
  
	h_minmax(h2);
	return h2;
}


hfield *rescale(hfield *h0, int xsize2, int ysize2) /* rescale heightfield */
{
	int x,y;
	int xa,ya,yaa;
	int xsize, ysize;
	size_t mem;
	hfield *h1;
	PTYPE *lbuf;                   /* one-line buffer */
	PTYPE t1,t2;                   /* HF values */
	double xsf,ysf;
	double ifrac;                  /* interpolation fraction */

	xsize = h0->xsize;             /* get X and Y dimensions of this HF */
	ysize = h0->ysize;

	mem = (size_t) (xsize+1) * sizeof(PTYPE);
	lbuf = (PTYPE *)malloc(mem);           /* alloc interpolation line buffer */

	if (h0->c) {
		if(!(h1 = h_newc(xsize2,ysize2))) return NULL;
	} else {
		if(!(h1 = h_newr(xsize2,ysize2))) return NULL;
	}

	xsf = (double) xsize / (xsize2);    /* X and Y scale factors */
	ysf = (double) ysize / (ysize2);    /* MOD +1 1/15/96 */

	for (y=0;y<ysize2;y++) {       /* y = [0  ... ysize2-1 ] */
		ya = (int)(y * ysf);                /* ya = [0 ... ysize-1 ]  */
		ifrac = ((double)y * ysf) - ya;     /* ifrac = [0..1] */
		if (ya < (ysize-1)) yaa = ya+1;
		else yaa = 0;
		for (xa=0;xa<xsize;xa++) {
			t1 = El(h0->a,xa,ya);      /* linear interpolation from t1 to t2 */
			t2 = El(h0->a,xa,yaa);
			lbuf[xa] = INTERPOLATE(t1,t2,ifrac);  /* lbuf[i]  i:0..xsize-1 */
		}
		lbuf[xsize] = lbuf[0];      /* last element */

		for (x=0;x<xsize2;x++) {
			xa = (int)(x * xsf); /* xa = [0 ... xsize-1] */
			ifrac = ((double)x * xsf) - xa;
			t1 = lbuf[xa];
			t2 = lbuf[xa+1];
			El2(h1->a,x,y) = INTERPOLATE(t1,t2,ifrac);
		}

	}  /* for (y... ) */
 
	if (h0->c) {   /* do everything again for complex side */
		for (y=0;y<ysize2;y++) {       /* y = [0  ... ysize2-1 ] */
			ya = (int)(y * ysf);                /* ya = [0 ... ysize-1 ]  */
			ifrac = ((double)y * ysf) - ya;     /* ifrac = [0..1] */
			if (ya < (ysize-1)) yaa = ya+1;
			else yaa = 0;
			for (xa=0;xa<xsize;xa++) {
				t1 = Im(h0->a,xa,ya);
				t2 = Im(h0->a,xa,yaa);
				lbuf[xa] = INTERPOLATE(t1,t2,ifrac);  /* lbuf[i]  i:0..xsize-1 */
			}
			lbuf[xsize] = lbuf[0];      /* last element */

			for (x=0;x<xsize2;x++) {
				xa = (int)(x * xsf); /* xa = [0 ... xsize-1] */
				ifrac = ((double)x * xsf) - xa;
				t1 = lbuf[xa];
				t2 = lbuf[xa+1];
				Im2(h1->a,x,y) = INTERPOLATE(t1,t2,ifrac);
			} /* end for x */
		} /* end for y */
	} /* end if h0->c */

	free(lbuf);  /* free line buffer */
	h_minmax(h1);
	return h1;
}

/* ============================================================================== */
hfield *h_clip(hfield *h1, int x1, int y1, int x2, int y2)  /* clip out rectangular subarray */
{
	int xsize1,ysize1,xsize2,ysize2;
	int x,y;
	hfield *h2;

	if(h1->c) {
		fprintf(stderr, "ERROR: clip: matrix is complex\n");
		return NULL;
	}
	xsize1 = h1->xsize;
	ysize1 = h1->ysize;

	x2 += x1;  /* convert X0,Y0,xsize,ysize into (x0,x1) (y0,y1) */
	y2 += y1;

	if (x1<0 || y1<0 || x2>xsize1 || y2>ysize1) {
		fprintf(stderr, "ERROR: clip: point falls outside of array.\n");
		return NULL;
	}
	if (x1>x2 || y1>y2) {
		fprintf(stderr, "ERROR: clip: specified rectangle has zero or negative area.\n");
		return NULL;
	}
	xsize2 = x2-x1;   /* find size of new sub-array */
	ysize2 = y2-y1;
	if(!(h2 = h_newr(xsize2,ysize2))) return NULL;

	for (y=0;y<ysize2;y++) {
		for (x=0;x<xsize2;x++) {
			El2(h2->a,x,y) = El1(h1->a,x+x1,y+y1);
		}
	}

	h_minmax(h2);
	return h2;
}

/* =============================================================== */

hfield *h_warp(hfield *h1, hfield *h2, int opn, D xcent, D ycent, D scalefac,int xsize3,int ysize3)
{
	hfield *h3;
	int xsize1,ysize1,xsize2,ysize2;
	int xsize,ysize;                /* for X_WRAP, Y_WRAP */
	int x,y,xx,yy,xx1,yy1;
	D fx,fy,ax,ay,ay2;
	D r,theta;              /* point in polar coordinates */
	D tmp1, tmp2;           /* interpolation values */
	D xfrac, yfrac;
	D xcent1, ycent1;       /* center of input matrix (hf1) */
	D xcent3, ycent3;       /* center of output matrix (hf3) */
	char name[20];
	int tile;
	PTYPE *hf1, *hf2, *hf3;

	hf2 = h2->a;              /* h2: control HF */
	xsize2 = h2->xsize;
	ysize2 = h2->ysize;

	tile = h_tilable(h1,0);
	hf1 = h1->a;              /* h1: source HF */
	xsize1 = h1->xsize;
	ysize1 = h1->ysize;
	if (opn==0) strcpy(name,"twist"); else strcpy(name,"bloom");
	if (h1->c || h2->c) {
		fprintf(stderr, "ERROR: %s: both input matrices must be real.\n",name);
		return NULL;
	}
	/*
	  if (xsize1!=xsize2 || ysize1!=ysize2) {
	  PE("X and Y HFs must have same dimensions");
	  return(1);
	  }
	*/
	xsize3 = xsize2;   /* output dim = control mx. dim */
	ysize3 = ysize2;
	/*
	  if (xsize3==0 || ysize3==0) {
	  xsize3 = xsize1;
	  ysize3 = ysize1;
	  }
	*/
	/* printf("%s output: [%dx%d]\n",name,xsize3,ysize3); */
 
	if(!(h3 = h_newr(xsize3,ysize3))) return NULL;
	hf3 = h3->a;
	xcent1 = (D)xcent*xsize1;
	ycent1 = (D)ycent*ysize1;
	xcent3 = (D)xcent*xsize3;  /* change to hf3 units */
	ycent3 = (D)ycent*ysize3;
 
	xsize = xsize1;
	ysize = ysize1;
	if (opn!=0) scalefac *= xsize2;

	for (y=0;y<ysize3;y++) {
		ay = ((D)y-ycent3);
		ay2 = ay*ay;
		for (x=0;x<xsize3;x++) {
			ax = ((D)x - xcent3);
			r = sqrt(ax*ax + ay2);
			theta = ATAN2(ay,ax);
			if (opn==0) theta += scalefac * El2(hf2,x,y); /* add control mx. factor */
			else r += scalefac * El2(hf2,x,y);
			fx = r * cos(theta);
			fy = r * sin(theta);
			/* printf("\t%5.2f %5.2f\n",ax,ay); */
			xx = (int)(fx); 
			yy = (int)(fy); 
			xfrac = ABS(fx-xx);
			yfrac = ABS(fy-yy);
			if (fx>0) xx1 = xx+1; else xx1 = xx-1; 
			if (fy>0) yy1 = yy+1; else yy1 = yy-1;
			xx  += (int)xcent1; yy  += (int)ycent1;
			xx1 += (int)xcent1; yy1 += (int)ycent1;
    
			if (tile) { X_WRAP(xx); Y_WRAP(yy); X_WRAP(xx1); Y_WRAP(yy1);}
			else { X_CLIP(xx); Y_CLIP(yy); X_CLIP(xx1); Y_CLIP(yy1);}
			/* printf("x:%d,%d\txx:%d,%d\t",x,y,xx,yy);  */
			tmp1 = INTERPOLATE(El1(hf1,xx,yy),El1(hf1,xx,yy1),yfrac);
			tmp2 = INTERPOLATE(El1(hf1,xx1,yy),El1(hf1,xx1,yy1),yfrac);
			El3(hf3,x,y) = INTERPOLATE(tmp1,tmp2,xfrac);
		} /* for x */
	} /* for y */

	h_minmax(h3);
	return h3;
}


/* --------------------------------------------------------------------
   h_cwarp() uses a complex control matrix to move each point in a source
   matrix to a corresponding point in a same-sized destination matrix.
   The amount each point is moved is determined by the control matrix.
   The real part of the matrix is the X displacement, and the imaginary
   part controls the Y displacement.  This is a generalization of the
   better known "twist" operator (circumferential displacement vectors).
 ----------------------------------------------------------------------
*/

hfield *h_cwarp(hfield *h1, hfield *h2, int opn, D scalefac)  /* warp with complex control */
{

	int xsize1,ysize1;      /* dimensions of three HFs */
	int xsize2,ysize2;
	int xsize3,ysize3;
	int xsize,ysize;                /* for X_WRAP, Y_WRAP macros */

	int xa,ya,yaa;          /* interpolation values for control mx. */
	double xsf,ysf;         /* scale factors for interpolation (control mx.) */

	int x,y,xx,yy,xx1,yy1;
	D fx,fy,ax,ay;

	D tmp1, tmp2;           /* interpolation values */
	D ifrac;                /* interpolation fraction for control mx. computation */
	D t1, t2;               /* temp. values for interpolation */

	D xfrac, yfrac;
	D creal, cimag;         /* X,Y values of control matrix for this point */
	int tile;
	int cinterp;            /* if control matrix is interpolated (1) or not (0) */
	hfield *h3;
	PTYPE *hf1, *hf2, *hf3;
	size_t mem;               /* amount of memory to alloc. for interpolation buffers */
	PTYPE *lbufr, *lbufi;     /* one-line control mx. interpolation buffers */

	hf2 = h2->a;                 /* h2: control HF (complex) */
	xsize2 = h2->xsize;
	ysize2 = h2->ysize;
	if(h2->c != TRUE) {
		fprintf(stderr, "ERROR: cwarp: control matrix must be complex\n");
		return NULL;
	}

	hf1 = h1->a;              /* hf1: source HF */
	xsize1 = h1->xsize;
	ysize1 = h1->ysize;
	tile = h_tilable(h1,0);
	scalefac *= xsize2;     /* in units of horizontal size of source HF */

	if (xsize1!=xsize2 || ysize1!=ysize2) {
		cinterp=TRUE;    /* yes, we must interpolate control matrix */
		/* since it's not of the same size as working mx. */
	} else {           
		cinterp=FALSE;
	}
 
	xsize3 = xsize1;   /* 3: output dim = source mx. dim */
	ysize3 = ysize1;

	if (cinterp) {
		mem = (size_t) (xsize2+1) * sizeof(PTYPE);
		lbufr = (PTYPE *)malloc(mem);           /* alloc interpolation line buffer: real */
		lbufi = (PTYPE *)malloc(mem);           /* alloc interpolation line buffer: imag */
		if ((lbufr==NULL) || (lbufi==NULL)) {
			perror("ERROR: cwarp: malloc");
			return NULL;
		}
	}


	/* -------- Generate new HF to store output ----------------- */
	if(!(h3 = h_newr(xsize3,ysize3))) return NULL;
	hf3 = h3->a;
 
	xsize = xsize1;
	ysize = ysize1;
	if (opn!=0) scalefac *= xsize2;

	xsf = (double) xsize2 / (xsize3);    /* X and Y scale factors */
	ysf = (double) ysize2 / (ysize3);

	creal = cimag = 0;     /* pointless, but removes a warning from compiler */

	for (y=0;y<ysize3;y++) {
		ay = (D)y;     /* ax,ay: real valued */

		if (cinterp) {                  /* generate Real, Imag. interpolation arrays */

			ya = (int)(y * ysf);                /* ya = [0 ... (ysize2 - 1) ] */
			ifrac = ((double)y * ysf) - ya;     /* ifrac = [0..1] */
			if (ya < (ysize2-1)) yaa = ya+1;
			else yaa = 0;                     /* wrap around at edge (hmm...good idea?) */

			for (xa=0;xa<xsize2;xa++) {    /* generate interpolation buffer */
				t1 = El2(hf2,xa,ya);      /* linear interpolation from t1 to t2 for real part */
				t2 = El2(hf2,xa,yaa);
				lbufr[xa] = INTERPOLATE(t1,t2,ifrac);  /* Real: lbufr[i]  i:0..xsize2-1 */

				t1 = Im2(hf2,xa,ya);      /* linear interpolation from t1 to t2 for imag. part */
				t2 = Im2(hf2,xa,yaa);
				lbufi[xa] = INTERPOLATE(t1,t2,ifrac);  /* Imag: lbufi[i]  i:0..xsize2-1 */

			}
			lbufr[xsize2] = lbufr[0];
			lbufi[xsize2] = lbufi[0];

		} /* if (cinterp) */

		for (x=0;x<xsize3;x++) {

			if (cinterp) {   /* control matrix interpolated */
				xa = (int)(x * xsf); /* xa = [0 ... xsize2-1] */
				ifrac = ((double)x * xsf) - xa;
				t1 = lbufr[xa];
				t2 = lbufr[xa+1];
				creal = INTERPOLATE(t1,t2,ifrac);
				t1 = lbufi[xa];
				t2 = lbufi[xa+1];
				cimag = INTERPOLATE(t1,t2,ifrac);

			} else {         /* control matrix not interpolated */
				creal = El(hf2,x,y);
				cimag = Im(hf2,x,y);
			}

			ax = (D)x;
			fx = ax + scalefac * creal;  /* x,y control factor */
			fy = ay + scalefac * cimag;

			xx = (int)(fx); 
			yy = (int)(fy); 
			xfrac = ABS(fx-xx);
			yfrac = ABS(fy-yy);
			if (fx>0) xx1 = xx+1; else xx1 = xx-1; 
			if (fy>0) yy1 = yy+1; else yy1 = yy-1;
    
			if (tile) { X_WRAP(xx); Y_WRAP(yy); X_WRAP(xx1); Y_WRAP(yy1);}
			else { X_CLIP(xx); Y_CLIP(yy); X_CLIP(xx1); Y_CLIP(yy1);}
			/* printf("x:%d,%d\txx:%d,%d\t",x,y,xx,yy);  */
			tmp1 = INTERPOLATE(El1(hf1,xx,yy),El1(hf1,xx,yy1),yfrac);
			tmp2 = INTERPOLATE(El1(hf1,xx1,yy),El1(hf1,xx1,yy1),yfrac);
			El3(hf3,x,y) = INTERPOLATE(tmp1,tmp2,xfrac);
		} /* for x */
	} /* for y */
 
	h_minmax(h3);
	return h3;
}

hfield *h_zedge(hfield *hf, D frac, D pwr)        /* set edges of HF to zero */
{
	int ix,iy;
	D fx,fy;
	int xsize,ysize;
	D xcent,ycent;
	D xsfac, ysfac;

	xsize = hf->xsize; xcent = xsize/2.0;
	ysize = hf->ysize; ycent = ysize/2.0;

	ysfac = M_PI/(1.0-frac);
	xsfac = M_PI/(1.0-frac);
	for (iy=0;iy<ysize;iy++) {
		fy = ABS((iy-ycent)/ycent);  /* fy on range 0..1 */
		if (fy > frac) {        /* in active edge region */
			fy = ((fy-frac)*ysfac);  /* now fy in range 0..pi */
			fy = 1.0-((1+sin(fy-PID2))/2.0);
		} else fy = 1.0;
		for (ix=0;ix<xsize;ix++) {
			fx = ABS((ix-xcent)/xcent);
			if (fx > frac) {        /* in active edge region */
				fx = ((fx-frac)*xsfac);  /* now fx in range 0..pi */
				fx = 1.0-((1+sin(fx-PID2))/2.0);
			} else fx = 1.0;
			El(hf->a,ix,iy) *= pow(fx*fy,pwr); 
		} /* for ix */
	} /* for iy */
 
	h_minmax(hf);
	return hf;
}

#if 0
D h_sample(hfield *h0, D fx, D fy)
{
	int xsize,ysize;
	int ix,iy;
	D fnx,fny;
	   
	xsize = h0->xsize;
	ysize = h0->ysize;

	if (fx > xsize-1) fx=xsize-1;
	if (fx < 0) fx=0;
	if (fy > ysize-1) fy=ysize-1;
	if (fy < 0) fy=0;

	fnx=fx;
	fny=fy;
	if (fnx<1.0) {
		fnx *= xsize; 
		if (fnx > (xsize-1)) fnx=xsize-1;
	} else fx /= xsize;
	if (fny<1.0) {
		fny *= ysize; 
		if (fny > (ysize-1)) fny=ysize-1;
	} else fy /= ysize;

	ix = (int)(fnx+0.5);
	iy = (int)(fny+0.5);
	fx = (D)ix/xsize;
	fy = (D)iy/ysize;

	printf("%s at [%d,%d] (%4.3f,%4.3f) is %1.3e",
		   h0->name,ix,iy,fx,fy,El(h0->a,ix,iy));
	if (h0->c) printf(" + %1.3e i",Im(h0->a,ix,iy));
	printf("\n");

	return(0);
}
#endif

/* smooth input HF h0, returning as h1: don't touch if h0(x,y) > thresh */
static void h_smoo2(hfield *h1, hfield *h0, int tile, D th1, D th2)
{
	int xsize, ysize;
	int ix, iy;
	D ht0, ht1=0;
	D htx,hty,htx1,hty1;   /* values of HF elements on either side of this one */
  
	xsize = h0->xsize;
	ysize = h0->ysize;
  
	for (iy = 0;iy<ysize;iy++) {
		for (ix = 0;ix<xsize;ix++) {
			ht0 = El(h0->a,ix,iy);
			if (tile) {
				htx =  Elmod(h0->a,ix-1,iy); hty= Elmod(h0->a,ix,iy-1);
				htx1 =  Elmod(h0->a,ix+1,iy); hty1= Elmod(h0->a,ix,iy+1);
			} else {
				htx = Elclip(h0->a,ix-1,iy); hty=Elclip(h0->a,ix,iy-1);
				htx1 = Elclip(h0->a,ix+1,iy); hty1=Elclip(h0->a,ix,iy+1);
			}
			if ((ht0 > th1)&&(ht0 < th2)) {  
				/* smooth those between elevation limits */
				ht1 = (htx + htx1 + hty + hty1)/4.0;
			} else ht1 = ht0;
			El(h1->a,ix,iy) = ht1;
		} /* for ix */
	} /* for iy */
}

hfield *h_nsmooth(hfield *h0, int iter, D th1, D th2)  /* slope-dependent smoothing */
{
	int tile;
	int xsize, ysize;
	hfield *h1;
	int repcount=0;

	xsize = h0->xsize;
	ysize = h0->ysize;
	if(h0->c) {
		fprintf(stderr, "ERROR: nsmooth: matrix is complex.\n");
		return NULL;
	}
	tile = h_tilable(h0, 0);  

	if(!(h1 = h_newr(xsize,ysize))) return NULL;

	do {
		h_smoo2(h1,h0,tile,th1,th2);
		h_smoo2(h0,h1,tile,th1,th2);
	} while (++repcount<iter);
	
	h_minmax(h1);
	return h1;
}
