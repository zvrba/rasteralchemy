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
-- $Id: hf.lua,v 1.1.2.10 2004/09/24 17:18:23 zvrba Exp $
-- bootstrap and stack manipulation code.
-- hf table is created in main C code.

-- TODO: set this in runtime!!
hf.interactive = true

-- create in advance all possibly needed tables
hf.METHODS = {}		-- low-level method definitions
hf.IMAGES  = {}		-- image table in interactive mode

dofile('hf-native.lua')
dofile('hf-vars.lua')


if hf.interactive then
	-- interactive mode. read comments in the file for details.
	dofile('hf-interactive.lua')
else
	-- non-interactive mode, just import function definitions into hf table.
	-- the programmer is expected to keep track of all images and must care
	-- about dangling pointers and aliasing
	for k,v in pairs(hf.METHODS) do
		hf[k] = v[3]
	end
end

