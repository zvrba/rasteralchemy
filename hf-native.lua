-- This file is a part of the Raster Alchemy package.
-- Copyright (C) 2004  Zeljko Vrba

-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-- You can reach me at the following e-mail addresses:

-- zvrba@globalnet.hr
-- mordor@fly.srk.fer.hr
-- $Id: hf-native.lua,v 1.1.2.14 2004/09/24 17:18:23 zvrba Exp $
-- hf native function mapping.
--
-- hf.PARAMS is a table of global parameters referenced from C implementation
--
-- hf.METHODS is a table of methods available for image operations. each metod
-- is in turn a table with the following fields:
-- [1]: method synopsis (short argument list and defaults). NOTE: all
--      heightfield arguments MUST begin with HF (to aid with automatic
--      parsing of arguments). Synopsis syntax is also very strict and can't
--      deviate from the examples already shown. All variables in synopsis
--      MUST be in uppercase to avoid clashes with internal variables used
--      for interactive processing.
-- [2]: long description text
-- [3]: function that calls, in appropriate way, the underlying native function
--
-- NOTE: all heightfield arguments MUST come first!
local M=hf.METHODS	-- as a short reference

M.rminmax = {
	"HF",
	"Return min,max value of real part.",
	function(hf)
		assert(hf, "nil image")
		return _hf_rminmax(hf)
	end
}

M.iminmax = {
	"HF",
	"Return min,max value of imaginary part.",
	function(hf)
		assert(hf, "nil image")
		return _hf_iminmax(hf)
	end
}

M.histeq={
	"HF [FRAC=1.0]",
	[[
Make a histogram equalized version of the current matrix, such that
there is an equal number of matrix values in each bin. The number
of bins used is specified in the variable HISTBINS, default is 1000.
The optional 'frac' argument is a real number where 1.0 is
complete equalization and 0.0 is no change. Frac can also be
less than zero or greater than one, although the results in that
case can be hard to predict (the default is 1.0).
]],
	function(hf, frac)
		assert(hf, "nil image")
		return _hf_histeq(hf, frac or 1.0)
	end
}

M.hist={
	"HF",
	"Print a text-mode histogram of the elements in the current matrix.",
	function(hf)
		assert(hf, "nil image")
		return _hf_hist(hf, hf.PARAMS.histbins)
	end
}

M.hshift={
	"HF [VALUE=0.0]",
	[[
Adds a constant to the matrix such that the most-represented 
elevation value (usually near 0.5 in a default "gforge" matrix)
is shifted to <value> (default = 0.0). This function involves
a histogram, using HISTBINS bins for the calculation.
]],
	function(hf, value)
		assert(hf, "nil image")
		return _hf_hshift(hf, value or 0.0)
	end
}

M.norm={
	"HF [MIN=0.0] [MAX=1.0]",
	[[
Linearly scale the values in the current matrix so that they
lie in the range <min> .. <max>.  The default range is [0..1].
If (min > max) then the surface will be inverted. Operates on
the current matrix in place, without pushing the stack.
]],
	function(hf, minv, maxv)
		assert(hf, "nil image")
		return _hf_norm(hf, minv or 0.0, maxv or 1.0)
	end
}

M.negate={
	"HF",
	[[
Invert the polarity of a matrix without changing the maximum or
minimum values. Peaks become valleys and vice-versa.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_negate(hf)
	end
}

M.diff={
	"HF",
	[[
Take the differential (local slope) of the matrix. This is called
"emboss" in image-processing programs.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_diff(hf, 'diff')
	end
}

M.dif2={
	"HF",
	[[
Take the second differential (local curvature) of the matrix. The
same result is achieved by taking the differential twice.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_diff(hf, 'dif2')
	end
}

M.rotate={
	"HF ANGLE",
	[[
Rotate the matrix clockwise the specified number of degrees. ANGLE can only be
90, 180 or 270. Hint: you can get fraction rotation (eg 37.5 degrees) with the
"twist" operation; just make the control matrix a constant.
]],
	function(hf, angle)
		assert(hf, "nil image")
		return _hf_rotate(hf, angle)
	end
}

M.peak={
	"HF XFRAC YFRAC",
	[[
Adjust the image such that that highest point is in the place
specified by the arguments (which are fractional horizontal
and vertical distances, ranging from 0..1).  This only works
with tilable images (such as are produced by gforge).
]],
	function(hf, xfrac, yfrac)
		assert(hf, "nil image")
		return _hf_peak(hf, xfrac, yfrac)
	end
}

M.is_tilable={
	"HF",
[[
Report if the current heightfield appears to tile smoothly.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_is_tilable(hf, 1)
	end
}

M.zero={
	"XSIZE [YSIZE=XSIZE]",
	[[
Generate a matrix of all zero values with the specified dimensions.
Useful prior to YSLOPE, GAUSS, or RING commands, for example.
If no argument is given, a zero matrix is created with the same
dimensions as the matrix currently on the top of the stack.
]],
	function(xsize, ysize)
		return _hf_zero(xsize, ysize or xsize)
	end
}

M.const={
	"VAL XSIZE [YSIZE=XSIZE]",
	[[
Generate a constant matrix with each element equal to <val>.
Useful prior to YSLOPE, GAUSS or RING commands, for example.
If no dimension is given, a constant matrix is created with the
same dimensions as the matrix currently on the top of the stack.
]],
	function(val, xsize, ysize)
		return _hf_const(xsize, ysize or xsize, val)
	end
}

M.cswap={
	"HF",
	[[
Exchange real and imaginary portions of current matrix. Matrix must
be complex.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_cswap(hf)
	end
}

M.cjoin={
	"HFX HFY",
	[[
Combine X (real) and Y (imaginary) matrices into a single
complex matrix. Dimensions must be equal.
]],
	function(x, y)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_cjoin(x, y)
	end
}

M.csplit={
	"HF",
	[[
Given a complex matrix, return two real matrices containing
the real and imaginary parts of the original.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_csplit(hf)
	end
}

M.cmag={
	"HF",
	[[
Take the magnitude of a complex matrix:  sqrt(Re*Re + Im*Im)
Result is a real matrix.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_cmag(hf)
	end
}

M.cdiff={
	"HF",
	[[
Take x and y slope (differential) of a real matrix and store
the result as a complex matrix: real = Diff(x), imag = Diff(y).
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_cdiff(hf)
	end
}

M.crect={
	"HF",
	[[
Convert a complex matrix in polar form to a complex matrix in rectangular
form: Re = Re * cos(Im);  Im = Re * sin(Im).  (Im units are radians)
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_cconvert(hf, 1)
	end
}

M.cpolar={
	"HF",
	[[
Convert a complex matrix in rectangular form to a complex matrix in polar
form: Re = sqrt(Re*Re + Im*Im);  Im = atan2(Im,Re)
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_cconvert(hf, 0)
	end
}

M.gforge={
	"SIZE [DIM=2.1]",
	[[
Create a fractal landscape in a square array having <mesh> elements
on each side. The surface roughness is controlled by the <dim>
parameter where 1.5 is fairly smooth and 2.3 is rather rough.
The resulting array is left on the top of the stack for subsequent
operations, display, or saving to a file.  <dim> may be any
number greater than zero. <mesh> is limited only by memory.
Note that a meshsize of 1000 uses just over 8 megabytes.
]],
	function(size, dim)
		return _hf_gforge(size, dim or 2.1)
	end
}

M.cfill={
	"XSIZE [YSIZE=XSIZE] [DIM=2.1]",
	[[
Generates a complex frequency-domain array of 1/f
noise with specified x,y size such that the inverse FFT will
yield a landscape. <dim> controls the (1/f) exponent, where 1.5
is very smooth and 2.5 is very rough. Default <dim> is 2.1
]],
	function(xsize, ysize, dim)
		return _hf_fill(xsize, ysize or xsize, dim or 2.1)
	end
}

M.fft={
	"HF DIR",
	[[
Forward (DIR==1) or inverse (DIR==-1) complex FFT.
]],
	function(hf, dir)
		assert(hf, "nil image")
		if (dir ~= 1) and (dir ~= -1) then
			error("Invalid direction. Must be 1 or -1.")
		end
		return _hf_fft(hf, dir, -2)
	end	
}

M.rand={
	"XSIZE [YSIZE=XSIZE]",
	[[
Generate a matrix of the specified dimensions filled with random
values distributed between 0 and 1. If only one argument is given,
the matrix is square with n x n elements.  Select uniform or
normal distribution with the variable 'randgauss' (see also).
]],
	function(xsize, ysize)
		return _hf_rand(xsize, ysize or xsize)
	end
}

M.yslope={
	"HF [FRAC=1.0] [SCALE=1.0] [POW=1.0]",
	[[
Add a one-dimensional, polynomial slope to the current matrix
starting a fraction <frac> from the edge. <frac> ranges from 0
(none of matrix has added slope) to 1 (all of it). <pow> specifies
the degree of the polynomial: 1 is a plane, 2 is a parabolic sheet.
Scale is the value of the function reached at the edge: a positive
scale adds positive slope to the matrix.
]],
	function(hf, frac, scale, pow)
		assert(hf, "nil image")
		return _hf_yslope(hf, frac or 1.0, scale or 1.0, pow or 1.0)
	end
}

M.ghill={
	"HF [XCENT=0.5] [YCENT=0.5] [WIDTH=0.3] [MAX=0.5]",
	[[
Add a rounded hill (gaussian bell curve) to the current HF.
<xcent> and <ycent> specify the center in fractions of the x and y
dimensions; GAUSS 0.5 0.5 puts the peak in the center of the matrix.
<width> sets the radius of the peak; 0.3 is a good trial value.
The function added goes from (near) zero at an edge to <max> at
the peak.
]],
	function(hf, xcent, ycent, width, maxv)
		assert(hf, "nil image")
		return _hf_ghill(
			hf,
			xcent or 0.5,
			ycent or 0.5,
			width or 0.3,
			maxv or 0.5)
	end
}

M.ring={
	"HF [XCENT=0.5] [YCENT=0.5] [RADIUS=0.25] [WIDTH=0.2] [MAX=0.5]",
	[[
Add a smooth ring shape (gaussian cross section) to the current HF.
<xcent> and <ycent> specify the center in fractions of the x and y
dimensions; RING 0.5 0.5 centers the ring in the matrix.  <radius>
is the radius of the ring and <width> sets how thick it is. The
ring function added goes from (near) zero at an edge to <max> at
the peak.
]],
	function(hf, xcent, ycent, radius, width, maxv)
		assert(hf, "nil image")
		return _hf_ring(
			hf,
			xcent or 0.5,
			ycent or 0.5,
			radius or 0.25,
			width or 0.2,
			maxv or 0.5)
	end
}

M.crater={
	"HF [CRATERS=100] [DEPTH=1.0] [RADIUS=1.0] [DIST=10.0]",
	[[
Add craters to a heightfield. You can specify the number of craters
to add, their depth and maximum radius relative to "normal" values,
and a size distribution factor (controls relative number of larger
craters to smaller ones). The minimum crater size is three pixel
elements across. dist=1.0 means equal numbers of all size craters;
dist=5.0 gives many more small than large craters (more realistic).
]],
	function(hf, ...)
		assert(hf, "nil image")
		return _hf_crater(
			hf,
			arg[1] or 100,
			arg[2] or 1.0,
			arg[3] or 1.0,
			arg[4] or 10.0)
	end
}

M.smooth={
	"HF FRAC",
	[[
Smooth the matrix by averaging each element with nearest neighbors.
<frac> argument specifies amount: 0.0 = no smoothing, 1.0 = element
is replaced by average of neighbors. <frac> may also be less than
zero or greater than one; note that this makes surfaces more rough.
(Often the smoothest effect is achieved by frac=0.5 )

SMOOTH will behave differently at the edges depending on the setting
of the "tile_mode" variable. With ON or AUTO, smoothed heightfields
which were tilable will remain so, and in fact edge discontinuities
will be reduced. If edge artifacts show up in this mode, your
heightfield has a discontinuity across the edge; set tile_mode to OFF.

See also nsmooth for many sequential "smooth" operations all-in-one.
]],
	function(hf, frac)
		assert(hf, "nil image")
		return _hf_smooth(hf, frac)
	end
}

M.nsmooth={
	"HF REPS [MIN=-1E6] [MAX=1E6]",
	[[
Smooth the HF <reps> times in sucession, while leaving those
values less than <hmin> and greater than <hmax> unchanged.  
Good for changing sharp-edged features (eg. imported B/W letters) 
into smooth slopes.
]],
	function(hf, reps, ...)
		assert(hf, "nil image")
		return _hf_nsmooth(hf, reps, arg[1] or -1E6, arg[2] or 1E6)
	end
}

M.joinx={
	"HFX HFY",
	[[
Combine the top two HFs X and Y into a single HF by stacking horizontally.
]],
	function(x, y)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_join(x, y, 1)
	end
}

M.joiny={
	"HFX HFY",
	[[
Combine the top two HFs X and Y into a single HF by stacking vertically.
]],
	function(x, y)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_join(x, y, 0)
	end
}

M.double={
	"HF [LOCAL_FRAC=0.5] [GLOBAL_FRAC=0.0]",
	[[
Double the size of the matrix using a modified-midpoint-displacement
algorithm (aka Fractint 'plasma') to do fractal interpolation
between points. Behavior depends on tiling mode; if tiling is turned
off, the new size will be one less than exactly double. Resolution
may be re-doubled as many times as you like.

<local_frac> specifies the interpolation roughness scale normalized
to each local pixel; the default will leave the flat areas flat.

<global_frac> specifies roughness everywhere: try DOUBLE 0.0 1.0
for added texture even in flat areas.

DOUBLE 0 0 is equivalent to RESCALE 2x 2y followed by SMOOTH.
]],
	function(hf, ...)
		assert(hf, "nil image")
		return _hf_double(hf, arg[1] or 0.5, arg[2] or 0.0)
	end
}

M.half={
	"HF",
	[[
Cut x and y dimensions in half, averaging each block of 4 pixels.
RESCALE is more general, but that is a simple (sub)sampling operation
which is not optimal when decreasing resolution.
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_half(hf)
	end
}

M.rescale={
	"HF XSIZE YSIZE",
	[[
Resize the matrix to the specified x and y dimensions. The scaling
operation uses bilinear interpolation. The SMOOTH operation should
be performed afterwards to remove the interpolation edges.
]],
	function(hf, xsize, ysize)
		assert(hf, "nil image")
		return _hf_rescale(hf, xsize, ysize)
	end
}

M.clip={
	"HF XSIZE [YSIZE=XSIZE] [XSTART=0] [YSTART=0]",
	[[
Cut out a sub-matrix of the specified dimension from the current HF.
<xstart> and <ystart> are the x,y offset from (0,0)
]],
	function(hf, xsize, ...)
		assert(hf, "nil image")
		return _hf_clip(
			xsize, arg[1] or xsize,
			arg[2] or 0, arg[3] or 0)
	end
}

M.cwarp={
	"HFX HFY [SCALE=1.0]",
	[[
Map Y matrix into new matrix using complex control matrix X.
real portion specifies x shift, imaginary = y shift. Matrix
is premultiplied by <scale> value, 1 unit= full width of matrix Y.
]],
	function(x, y, ...)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_cwarp(x, y, 0, arg[1] or 1.0)
	end
}

M.zedge={
	"HF [FRAC=0.5] [POW=1.0]",
	[[
Set the edges of the HF to zero. <frac> specifies the fractional
distance from the center to the edge to start reducing amplitude.
<frac> can be from 0.0 to 1.0. <pow> sets the smoothing function
exponent. Range is anything > 0.0; 1.0 is normally adequate.
]],
	function(hf, ...)
		assert(hf, "nil image")
		return _hf_zedge(hf, arg[1] or 0.5, arg[2] or 1.0)
	end
}

M.fillbasin={
	"HF CYCLES",
	[[
Fill in basins by replacing each local minimum with the average
of the surrounding points. Repeat this procedure <cycles> times.
This is VERY slow on large HFs, and will not fill large basins
in any reasonable amount of time (try FILLB at lower resolution
and then DOUBLE).
]],
	function(hf, cycles)
		assert(hf, "nil image")
		return _hf_fillb(hf, cycles, 1.0)
	end
}

M.flow={
	"HF",
	[[
Calculate flowlines according to slope. Unlike other HF-Lab operators,
the output is an imagemap rather than a heightfield. Pixel values are 
proportional to total uphill area (eg, water in a river). Flow lines 
end at a local minimum (which a typical GForge surface is full of, 
unless 'fillbasin' has been run for many cycles first).
]],
	function(hf)
		assert(hf, "nil image")
		return _hf_find_ua(hf)
	end
}

M.bloom={
	"HFX HFY [SCALE=1.0] [XCENT=0.5] [YCENT=XCENT]",
	[[
Radially expand (or contract) the Y matrix to a degree based
on the value of each point in the X matrix. Positive = radial
expansion, negative = radial contraction. If the control matrix
is nonzero at <xcent>,<ycent> there will be a discontinuity there.
The control value is premultiplied by <scale>.
]],
	function(x, y, scale, xcent, ycent)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_warp(
			x, y, 1,
			xcent, ycent or xcent, scale or 1.0, 0, 0)
	end
}

M.twist={
	"HFX HFY [SCALE=1.0] [XCENT=0.5] [YCENT=XCENT]",
	[[
Rotate each point in the Y matrix about (xcent,ycent) by an angle 
specified by the value (radians) of each corresponding point in the 
X matrix. Positive = counterclockwise. The rotation angle increment 
is multiplied by <scale>. 
]],
	function(x, y, scale, xcent, ycent)
		assert(x, "nil image")
		assert(y, "nil image")
		return _hf_warp(
			x, y, 0,
			xcent, ycent or xcent, scale or 1.0, 0, 0)
	end
}

M.seed={
	"N",
	[[
Set the random number seed to be used for the next 'gforge', 'crater',
'random', or 'fillarray' command. If not set before each 'gforge' etc.
command, the seed is regenerated based on the system clock. Type 'showvar'
to see the values of the internal variables including 'seed'. If 
rnd_seed_stale is 0, you have set that seed value for next use.
If not, that was the seed value used previously.
]],
	function(seed)
		hf.PARAMS.rnd_seed = math.abs(seed)
		hf.PARAMS.rnd_seed_stale = 0
	end
}

M.lslope={
	"HF MAX_SLOPE REPS",
	[[
Average each pixel which has a local slope greater than <max_slope>
with its 4 neighbors. Make <reps> passes through the HF in all.
This will take a very long time on large HFs. Produces cone-shaped
features. To find current maximum HF slope, try:  dup;diff;l;pop
and look at the maximum value of the 'diff' HF on the stack.
]],
	function(hf, maxs, reps)
		return _hf_slopelim(hf, 'lslope', maxs, reps)
	end
}

M.lcurve={
	"HF MAX_CURVE REPS",
	[[
Average each pixel which has a local curvature greater than
<max_curve>. Make <reps> passes through the HF in all. This is
the second-order version of LSLOPE, and converges more quickly.
The effect is to smooth out the rough parts of the HF. To find
the current maximum HF curvature try:    dup;dif2;l;pop
and look at the maximum value of the 'dif2' HF on the stack.
]],
	function(hf, maxs, reps)
		return _hf_slopelim(hf, 'lcurve', maxs, reps)
	end
}

M.hpf={
	"HF [CUTFREQ=0.05] [ORDER=1]",
	[[
range:  <cut-freq>, <cent-freq>  [0..1] (normalized frequency; 1 = max. freq)
        <order>,    <Q>          > 0.  1.0=moderate, 10=strong, 100=brickwall

These filters modify the current (real) array in the Fourier (frequency)
domain. The real data is transformed with a FFT, (becoming complex),
filtered, and then transformed back with an IFFT.

The lowpass filter smooths out sharp edges, the highpass sharpens them. The
bandpass magnifies bumps (wavelengths) of a given size, and the bandreject
removes such features.  The "SMOOTH" operation is faster for very slight
smoothing, but the more powerful LPFILTER can smooth to any degree desired
in a single pass. LPF 0.3 1 is a weak filter, LPF 0.05 100 is strong.
]],
	function(hf, cutf, order)
		return _hf_realfilt(hf, cutf or 0.05, order or 1, 'hpf')
	end
}

M.lpf={
	"HF [CUTFREQ=0.1] [ORDER=1]",
	M.hpf[2],
	function(hf, cutf, order)
		return _hf_realfilt(hf, cutf or 0.1, order or 1, 'lpf')
	end
}

M.bpf={
	"HF [CUTFREQ=0.1] [Q=4]",
	M.hpf[2],
	function(hf, cutf, q)
		return _hf_realfilt(hf, cutf or 0.1, q or 4, 'bpf')
	end
}

M.brf={
	"HF [CUTFREQ=0.0] [Q=50]",
	M.hpf[2],
	function(hf, cutf, q)
		return _hf_realfilt(hf, cutf or 0.0, q or 50, 'brf')
	end
}

M.fflp={
	"HF CUTFREQ ORDER",
	[[
These filters operate on complex matrices only. Use LPF, HPF
etc. for real data. The data is assumed to be in the frequency
domain, and must be subsequently transformed with IFFT.

The first (frequency) argument ranges from zero to one (ie,
normalized frequency). <order> specifies the strength of the
filter, where 1.0 is a weak filter and 100 or larger approaches
a brick-wall filter. <Q> is the 'quality factor' of a bandpass
or band-reject filter, where 1.0 is a gentle filter and 100 or
larger is an extremely sharp peak or notch filter.
]],
	function(hf, cutfreq, order)
		return _hf_fourfilt(hf, cutfreq, order, 'fflp')
	end
}

M.ffhp={
	"HF CUTFREQ ORDER",
	M.fflp[2],
	function(hf, cutfreq, order)
		return _hf_fourfilt(hf, cutfreq, order, 'ffhp')
	end
}

M.ffbp={
	"HF CUTFREQ Q",
	M.fflp[2],
	function(hf, cutfreq, q)
		return _hf_fourfilt(hf, cutfreq, q, 'ffbp')
	end
}

M.ffbr={
	"HF CUTFREQ Q",
	M.fflp[2],
	function(hf, cutfreq, q)
		return _hf_fourfilt(hf, cutfreq, q, 'ffbr')
	end
}

M.sin={
	"HF",
	[[
These operators perform the respective trigonometric function
on the current matrix.
]],
	function(hf)
		return _hf_op1(hf, _hfop1_sin())
	end
}

M.asin={
	"HF",
	[[
These operators perform the respective trigonometric function
on the current matrix. Input to ASIN is clamped to [-1..1].
]],
	function(hf)
		return _hf_op1(hf, _hfop1_asin())
	end
}

M.cos={
	"HF",
	M.sin[2],
	function(hf)
		return _hf_op1(hf, _hfop1_cos())
	end
}

M.acos={
	"HF",
	[[
These operators perform the respective trigonometric function
on the current matrix. Input to ACOS is clamped to [-1..1].
]],
	function(hf)
		return _hf_op1(hf, _hfop1_acos())
	end
}

M.tan={
	"HF",
	[[
These operators perform the respective trigonometric function
on the current matrix. Where tan is undefined (multiples of PI/2)
the value is set to 0.
]],
	function(hf)
		return _hf_op1(hf, _hfop1_tan())
	end
}

M.atan={
	"HF",
	[[
These operators perform the respective trigonometric function
on the current matrix.
]],
	function(hf)
		return _hf_op1(hf, _hfop1_atan())
	end
}

M.abs={
	"HF",
	[[Take the absolute value of each matrix element.]],
	function(hf)
		return _hf_op1(hf, _hfop1_abs())
	end
}

M.inv={
	"HF",
	[[
For each element of the matrix A, form the result 1/A. If x is zero,
the result is arbitrarily defined as zero.
]],
	function(hf)
		return _hf_op1(hf, _hfop1_inv())
	end
}

M.log={
	"HF",
	[[
Take the natural logarithm of the input matrix. The log of zero or
negative values is set to zero.
]],
	function(hf)
		return _hf_op1(hf, _hfop1_log())
	end
}

M.disc={
	"HF VAL",
	[[
Discretize HF to have only <val> different levels. Saving your
file as a GIF and then loading it is equivalent to DISC 256,
because the GIF file has only 256 different colors available.
]],
	function(hf, val)
		return _hf_op1(hf, _hfop1_disc(val))
	end
}

M.mod={
	"HF VAL",
	[[
Take the HF modulus <val>, ie, wrap around at +<val> and -<val>.
For a smoother but similar effect, prescale the HF and use SIN.
]],
	function(hf, val)
		return _hf_op1(hf, _hfop1_mod(val))
	end
}

M.pow1={
	"HF POWER",
	[[
These operators add, subtract, multiply, divide, or raise
to a power each element of the current heightfield. For instance,
* 3.14   would multiply each element in the current matrix by
the value 3.14.
]],
	function(hf, pow)
		return _hf_op1(hf, _hfop1_pow1(pow))
	end
}

M.add1={
	"HF POWER",
	M.pow1[2],
	function(hf, pow)
		return _hf_op1(hf, _hfop1_add1(pow))
	end
}

M.sub1={
	"HF POWER",
	M.pow1[2],
	function(hf, pow)
		return _hf_op1(hf, _hfop1_sub1(pow))
	end
}

M.mul1={
	"HF POWER",
	M.pow1[2],
	function(hf, pow)
		return _hf_op1(hf, _hfop1_mul1(pow))
	end
}

M.div1={
	"HF POWER",
	M.pow1[2],
	function(hf, pow)
		return _hf_op1(hf, _hfop1_div1(pow))
	end
}

M.add2={
	"X Y [XO=0] [YO=0]",
	[[
These operators combine the two HFs on top of the stack
using the respective arithmetic operation on an element-by-
element basis. Pixels resulting from a divide-by-zero operation
are set to zero.

Dimensions of the X HF must be equal to or smaller than those of
the Y HF.  The optional <xo> <yo> arguments specify the offset of
the X HF relative to the Y; the default is 0 0 (no offset).
]],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_add2())
	end
}

M.sub2={
	"X Y [XO=0] [YO=0]",
	M.add2[2],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_sub2())
	end
}

M.mul2={
	"X Y [XO=0] [YO=0]",
	M.add2[2],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_mul2())
	end
}

M.div2={
	"X Y [XO=0] [YO=0]",
	M.add2[2],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_div2())
	end
}

M.exp={
	"X Y [XO=0] [YO=0]",
	[[
Raise each element in the current matrix to the power of the
corresponding element of the second matrix on the stack.

Dimensions of the X HF must be equal to or smaller than those of
the Y HF.  The optional <xo> <yo> arguments specify the offset of
the X HF relative to the Y; the default is 0 0 (no offset).
]],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_add())
	end
}

M.gt={
	"X Y [XO=0] [YO=0]",
	[[
Combine two matrices by choosing each element to be the greater
of the two input elements. The result appears as the 'intersection'
of the two components.

Dimensions of the X HF must be equal to or smaller than those of
the Y HF.  The optional <xo> <yo> arguments specify the offset of
the X HF relative to the Y; the default is 0 0 (no offset).
]],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_com())
	end
}

M.lt={
	"X Y [XO=0] [YO=0]",
	[[
Combine two matrices by choosing each element to be the lesser
of the two input elements. The result appears as the 'intersection'
of the two components.

Dimensions of the X HF must be equal to or smaller than those of
the Y HF.  The optional <xo> <yo> arguments specify the offset of
the X HF relative to the Y; the default is 0 0 (no offset).
]],
	function(x, y, xo, yo)
		return _hf_op2(x, y, xo or 0, yo or 0, _hfop2_comn())
	end
}

M.load={
	"FNAME",
	[[
Load real or complex heightfield from file named FNAME. Read help for SAVE
for description of file format.
]],
	function(fname)
		return _hf_load(fname)
	end
}

M.save={
	"HF FNAME",
	[[
Save real or complex heightfield HF into file named FNAME.

Library used for I/O is ILM's OpenEXR library. Images are saved in compressed
format. Real images have a single channel named "H", complex images have two
channels named "RE" and "IM" (complex components in rectangular coordinate
system). All pixels are stored in 4-byte float format.
]],
	function(hf, fname)
		return _hf_save(fname, hf)
	end
}

