/* --- erode.c  heightfields and water flow -------- */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hf-hl.h"

static char rcsid[] UNUSED = "$Id: hf-erode.cc,v 1.1.2.2 2003/12/31 15:25:18 zvrba Exp $";

/*  1 2 3   | -- +x ->
 *  8 0 4   |
 *  7 6 5   +y
 *          \/ 
 */
static int xo[9] = { 0,-1, 0, 1,1,1,0,-1,-1 };       /* 8-dir index offset arrays */
static int yo[9] = { 0,-1,-1,-1,0,1,1, 1, 0 };
//static int x4[4] = { -1,0,1,0 };                     /* 4-dir index offset arr */
//static int y4[4] = { 0,-1,0,1 };
//static int dhash[9] = { 1,2,3,8,0,4,7,6,5 };         /* 8-direction hash mx */

/* ------------------------------------------------------------ */
/* lowestn()    --      returns direction of steepest descent from */
/*                      element x,y in hf1[] array                 */
/* ------------------------------------------------------------ */
static int lowestn(hfield *hf, int x, int y, D *r, int tile)
{ 
	PTYPE *hf1;
	int xsize, ysize;
	int i, mini;
	float here, slope, min;

	hf1 = hf->a;
	xsize = hf->xsize;
	ysize = hf->ysize;
	mini = 0;  
	min = FLT_MAX;
	here = El(hf1,x,y);
	if (tile) {
		for (i=1;i<9;i++) {
			slope = Elmod(hf1,x+xo[i],y+yo[i]) - here;
			if ( (i==1) || (i==3) || (i==5) || (i==7) )
				slope = slope / 1.414;
			if (slope < min) {
				mini = i;
				min = slope;
			}
		}
	} else {  /* tile == FALSE */
		for (i=1;i<9;i++) {
			slope = Elclip(hf1,x+xo[i],y+yo[i]) - here;
			if ( (i==1) || (i==3) || (i==5) || (i==7) )
				slope = slope / 1.414;
			if (slope < min) {
				mini = i;
				min = slope;
			}
		}
	} /* end if(tile) */
	*r = min;
	return mini;
}

static int is_lowest(hfield *hf, int x, int y, int tile) /* 1 if x,y is min */
{
	int i;
	int xsize, ysize;
	PTYPE here;
	D diff;

	xsize = hf->xsize;
	ysize = hf->ysize;
	if (tile) {
		here = Elmod(hf->a,x,y);
		for (i=1;i<9;i++) {
			diff = Elmod(hf->a,x+xo[i],y+yo[i]) - here;
			if (diff < 0) return(FALSE);
		}
		return(TRUE);
	} else {
		here = Elclip(hf->a,x,y);
		for (i=1;i<9;i++) {
			diff = Elclip(hf->a,x+xo[i],y+yo[i]) - here;
			if (diff < 0) return(FALSE);
		}
	} /* end else (not h_tilable) */
	return(TRUE);
} /* is_lowest */

/* ------------------------------------------------------------ */
/*  fill_bn  --  fill in basins in heightfield area             */
/*               input is hf1, output is in hf2                 */
/* ------------------------------------------------------------ */
static int fill_bn(hfield *h2, hfield *h1)
{
	PTYPE *hf1, *hf2;
	int xsize, ysize;
	int count;              /* number of minima changed */
	int tile;
	int ix,iy,i;
	float sum, neb, wavg;

	hf1 = h1->a;              /* array part of heightfield 1 */
	hf2 = h2->a;
	xsize = h1->xsize;
	ysize = h1->ysize;
	tile = h_tilable(h1, 0);  /* 1 if tilable, 0 if not */
	count = 0;
  
	/* start with exact copy */
	/*
	  for (iy=0;iy<ysize;iy++) {       
	  for (ix=0;ix<xsize;ix++) {
	  El(hf2,ix,iy) = El(hf1,ix,iy);
	  }
	  }
	*/

	for(iy=0;iy<ysize;iy++)  {
		for (ix=0;ix<xsize;ix++)  {
			if (is_lowest(h1,ix,iy,tile))  {           /* local depression */
				count++;
				sum = 0; 
				if (tile) {
					for (i=1;i<9;i++) {
						neb = Elmod(hf1,ix+xo[i],iy+yo[i]);      /* neighbor elev. */
						sum += neb;                   /* get sum of neighbor elevs */
					}
				} else {
					for (i=1;i<9;i++) {
						neb = Elclip(hf1,ix+xo[i],iy+yo[i]);      /* neighbor elev. */
						sum += neb;                   /* get sum of neighbor elevs */
					}
				}
				wavg = sum / 8;               /* simple average */
				El(hf2,ix,iy) = wavg;       /* hf2(x,y) <- weighted local avg */
			} /* end if is_lowest()  */
			else {    /* if not depression, just copy old mx to new */
				El(hf2,ix,iy) = El(hf1,ix,iy);
			}
		} /* end for ix */
		/* printf("\n"); */
	} /* end for iy */

	return(count);
}

/* ------------------------------------------------------------ */
/* find_upflow -- find direction of stream flow at each lattice */
/*                point, and set element of flag (uphill).      */
/*                Must be called after find_flow2() so fl set.  */
/* ------------------------------------------------------------ */
static void find_upflow(hfield *h1, BYTE *fl, BYTE *flag, int tile)
{
	PTYPE *hf;
	int xsize,ysize;
	int x, y,i;
	int xn, yn;
	int tcounter;      /* counter to print 'wait dots' */
	int dir;
	int inflows;                    /* #nodes flowing in */
	D mval;            /* current minimum elevation value */
	D maxv;            /* current maximum */
	D tval;            /* temporary value */

	hf = h1->a;
	xsize = h1->xsize;
	ysize = h1->ysize;
	tcounter = 0;
	for (y = 0; y < ysize; y++) {
		if (++tcounter>100) {
			tcounter=0;
		}
		for (x = 0; x < xsize; x++)  {
			mval = El(hf,x,y);          /* start with current point */
			maxv = mval;
			tval = El(hf,x,y);
			El(flag,x,y) = 0;       /* 0 means point is a peak */
			inflows = 0;            /* # neighbors pointing downhill to me */

			for (i=1;i<9;i++) {
				if (tile) tval = Elmod(hf,x+xo[i],y+yo[i]); /* neighbor higher? */
				else tval = Elclip(hf,x+xo[i],y+yo[i]);
	  
				if (tval > maxv) {
					maxv = tval;                /* yes, set uphill->this one */
					El(flag,x,y) = i;
				}
				xn = x + xo[i];
				yn = y + yo[i];
				/* make sure not off edge of mx */
				if ((xn >= 0) && (xn < xsize) && (yn >= 0) && (yn < ysize)) {
					dir = El(fl,xn,yn);       /* inflow from here? */
					if ((xo[i]+xo[dir]==0) && (yo[i]+yo[dir]==0)) { /* inflow! */
						inflows++;
					}
				}  /* end if (inbounds) */
			} /* end for i */
	
			if (inflows == 0) {     /* no inflows: find_ua can start here */
				El(flag,x,y) = (El(flag,x,y) | 0x10);  /* set b4: summed */
				/* printf("uflow %d,%d summed\n",x,y); */
			}
			/* and if we still think it's a peak...*/

			if (El(flag,x,y)==0) {       /* supposed to be peak */
				for(i=1;i<9;i++) {
					if (tile) dir = Elmod(fl,x+xo[i],y+yo[i]);
					else dir = Elclip(fl,x+xo[i],y+yo[i]);
					if ((xo[i]+xo[dir]==0) && (yo[i]+yo[dir]==0)) { /* inflow! */
						El(flag,x,y)=1;  /* arbitrary direction */
					}
				} /* end for i */
			} /* end if d==0 */

		} /* end for x*/
	} /* end for  y */
}

/* ----------------------------------------------------------   */
/* fillb   --   call fill_bn() (fill basins) up to imax times   */
/*              in order to fill in basins, returns # of calls  */
/* ----------------------------------------------------------   */
hfield *h_fillb(hfield *h1, int imax, D rate)  /* fill basin imax times */
{
	hfield *h2;
	int xsize, ysize;
	int i,count;

	if(h1->c) {
		fprintf(stderr, "ERROR: fillb: matrix is complex.\n");
		return NULL;
	}
	xsize = h1->xsize;
	ysize = h1->ysize;
	if(!(h2 = h_newr(xsize,ysize))) return NULL;

	for (i=0;i<imax;i++) {
		fill_bn(h2,h1);
		count=fill_bn(h1,h2);
		if (count==0) break;
	}

	h_delete(h2);
	h_minmax(h1);
	return h1;
}

#define BYTE unsigned char
/* ------------------------------------------------------------ */
/*    find_ua()  --  find uphill area for each mx element       */
/*    that is, area or # of pixels which flow into this one     */
/* ------------------------------------------------------------ */
hfield *h_find_ua(hfield *h1)                /* find uphill area */
{
	hfield *h2;
	int xsize, ysize;
	int tile;
	PTYPE *ua;            /* uphill area */
	int ix,iy;
	int i;
	int tcounter;
	int dir;              /* direction this element points in */
	BYTE ff;               /* flag variable for this loc */
	int o_summed;         /* flag indicating all neighbors summed */
	long added;            /* # found unsummed nodes this pass */
	float area;           /* sum of uphill area for this element */
	size_t msize;         /* memory needed to alloc */
	BYTE *fl;             /* flow direction array */
	BYTE *flag;
	D slope;

/* flag  matrix, b4 indicates this node summed 
   ua    matrix, uphill area (pixel count) 
   fl    matrix, direction of flow (downhill index)
*/

	xsize = h1->xsize;
	ysize = h1->ysize;
	tile = h_tilable(h1, 0);

	msize = ((size_t) xsize * ysize * sizeof(char));
	fl = (BYTE *) malloc( msize);
	flag = (BYTE *) malloc( msize);
	if ((fl == NULL) || (flag == NULL ))   {
		perror("ERROR: find_ua: malloc");
		if (fl!=NULL) free(fl);
		if (flag!=NULL) free(flag);
		return NULL;
	}
	memset(fl,(BYTE) 0, msize); /* ------ fl[]   now allocated ------- */
	memset(flag,(BYTE) 0, msize); /* ---- flag[] now allocated ------- */

	if(!(h2 = h_newr(xsize, ysize))) return NULL;
	ua = h2->a;

	for (iy = 0; iy < ysize; iy++) { for (ix = 0; ix < xsize; ix++)  {
		El(fl,ix,iy) = lowestn(h1,ix,iy,&slope,tile);
	}  /* for x */ }  /* for y */     /* ------ fl[] now set ------- */

	find_upflow(h1, fl, flag, tile);   /* ------- flag[] now set ---- */
/* first pass: set local highpoints to "summed", area to 1 */

	for (iy = 0; iy<ysize; iy++) {
		for (ix = 0; ix<xsize; ix++) {
			if ( (El(flag,ix,iy) & 0x10) == 0x10) { /* pre-summed (no inflows) */
				El(ua,ix,iy) = 1;
				/* printf("presummed %d %d\n",ix,iy); */
			}
			if ( (El(flag,ix,iy) & 0x0f) == 0) {   /* this point is a peak */
				El(ua,ix,iy) = 1;                     /* peak: unit area */
				El(flag,ix,iy) = (El(flag,ix,iy) | 0x10);  /* set b4: node summed */
			} /* end if */
		} /* end for ix */
	} /* end for iy */

/* 2nd through n passes: cumulative add areas in a downhill-flow hierarchy */
	tcounter = 0;
	added = 1;
	while ( added > 0 ) {
		added = 0;
		for (iy = 1; iy<(ysize-1); iy++) {
			for (ix = 1; ix<(xsize-1); ix++) {
				ff = El(flag,ix,iy);
				if (!(ff & 0x10)) {      /* flag bit b4: set = ua for this node done */
					/* if all neighbors flowing into this node done, can sum */
					o_summed = 1;           /* start assuming we're ok... 0=false*/
					for(i=1;i<9;i++) {      /* check if relevant neighbors finished */
						dir = El(fl,ix+xo[i],iy+yo[i]);
						if ( ((xo[dir]+xo[i])==0) && ((yo[dir]+yo[i])==0))
							/* ie,that node points here */
							if (!(El(flag,ix+xo[i],iy+yo[i]) & 0x10))
								o_summed = 0;
					} /* end for i=1..8 */
					if (o_summed) {
						area = 1;
						for (i=1;i<9;i++) {
							dir = El(fl,ix+xo[i],iy+yo[i]);
							if ( ((xo[dir]+xo[i])==0) && ((yo[dir]+yo[i])==0))
								area += El(ua,ix+xo[i],iy+yo[i]);
						}  /* end for i=1..9 */
						El(ua,ix,iy) = area;
						El(flag,ix,iy) = El(flag,ix,iy) | 0x10;  /* set b4: node summed */
						added++;  /* increment count of nodes summed */
					}  /* end if o_summed */

				} /* end if b4 set */
			}  /* end for x=1..xsize-1 */
		} /* end for y=1..ysize-1 */

		/* printf("before edges, added = %d ",added); */
		/* aaand now... do the edges */
/*   
	 for (iy = 0; iy<ysize; iy++) {
	 added += ua_iter(0,iy);
	 added += ua_iter(xsize-1,iy);
	 }
	 for (ix = 1; ix<xsize-1; ix++) {
	 added += ua_iter(ix,0);
	 added += ua_iter(ix,ysize-1);
	 }
*/
		/* printf("after %d, hit enter...\n",added); */
		if (++tcounter > 10) {
			tcounter=0;
		}
	} /* end while (added > 0) */
	/*
	  for(ix=0;ix<xsize;ix++) {
	  for(iy=0;iy<ysize;iy++) {
      dir = (int) El(fl,ix,iy);
      switch (dir) {
	  case 1: Im(ua,ix,iy) = 3*M_PI/2; break;
	  case 2: Im(ua,ix,iy) = M_PI/2; break;
	  case 3: Im(ua,ix,iy) = M_PI/4; break;
	  case 4: Im(ua,ix,iy) = 0; break;
	  case 5: Im(ua,ix,iy) = -M_PI/4; break;
	  case 6: Im(ua,ix,iy) = -M_PI/2; break;
	  case 7: Im(ua,ix,iy) = -3*M_PI/2; break;
	  case 8: Im(ua,ix,iy) = M_PI; break;
	  case 0: Im(ua,ix,iy) = 0; break;
	  default:
	  printf("Find_ua internal error: fl[] illegal value\n");
	  return(1);
      } 
	  } 
	  } 
	*/
	free(flag);
	free(fl);
	h_minmax(h2);
	h_oneop(h2, "pow", 0.5, 0);   /* take square root */
	norm(h2, 0, 1);
	return h2;
}
