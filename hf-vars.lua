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
-- $Id: hf-vars.lua,v 1.1.2.4 2004/09/24 17:18:23 zvrba Exp $
-- Help on global parameters.
-- NOTE: HF_PARAMS is userdata and so can't be traversed with pairs(). For
-- each variable that is to be listed by showvars, its help must be defined
-- in PARAMS_HELP.

local PARAMS_HELP={
	rand_gauss = [[
1: gaussian (normal) distrib. 0: uniform. This relates to the rand matrix
generator.
]],
	rnd_seed = [[
Next random number seed. Use SEED to change it.
]],
	rnd_seed_stale = [[
If non-zero, next random number seed will be based on the system clock.
Otherwise, the displayed rnd_seed will be used. Should not be changed
manually. Use SEED.
]],
	histbins = [[
Number of bins used in histogram equalization. This can be any positive integer
greater than one.
]],
	tile_mode = [[
Treatment of matrix edge-wraparound.

HF_PARAMS.TILE_ON    consider matrix to be seamlessly tilable
HF_PARAMS.TILE_OFF   consider matrix to have unique edges
HF_PARAMS.TILE_AUTO  attempt to auto-determine tilability of any given matrix
  by using algorithm.

The functions DOUBLE, RESCALE, SMOOTH, CRATER, GAUSS, and RING have
two modes of behavior, and which one should be used depends on whether
the matrix is seamlessly tilable (e.g. any output of GFORGE) or not
(e.g. almost anything else).  The auto-detect mode isn't as sensitive
as your eye, so if you know the tilability of your matrix, it's better
to set tile_mode to ON or OFF rather than the default AUTO.
]],
	tile_tol = [[
Value used to distinguish a matrix which can be seamlessly tiled, from one that
can't. 0.01 seems to work ok. This has meaning only when variable 'tile_mode'
is set to HF_PARAMS.TILE_AUTO.
]],
	gaufac = [[
Internal variable that was hard-coded to 4.0. Used only in GHILL.
]]
}

-- function to display the value of all runtime global variables
function showvar(varname)
	if not varname then
		for k, v in pairs(PARAMS_HELP) do
			print(string.format("%24s: %f", k, hf.PARAMS[k]))
		end
	else
		local help = PARAMS_HELP[varname]
		local val  = hf.PARAMS[varname]
		print("VALUE: " .. (val or "NOT DEFINED"))
		print(help or "**NO DESCRIPTION for variable " .. varname)
	end
end


