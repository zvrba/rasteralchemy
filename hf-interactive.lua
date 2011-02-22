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
-- $Id: hf-interactive.lua,v 1.1.2.5 2004/09/24 17:18:23 zvrba Exp $
-- interactive routines

--------------------------------------INTERACTIVE QUERY FOR FUNCTION ARGUMENTS
-- This set of functions queries for function arguments. It parses the
-- synopsis field of given method.
------------------------------------------------------------------------------
local function is_image(v)
	return type(v) == "userdata" and v:_type() == "hfield"
end

-- acquire a single argument. vals is the table of already acquired arguments.
-- default values are computed from the expression at the right-hand side of =
-- the expression must have valid lua syntax. expression is evaluated in vals
-- environment, so it may use already defined arguments. arg is argument def.
-- function returns the queried value.
-- typing EOF (^D on UNIX) returns nil. this cancels the current operation.
local function query_argument(arg, vals)
	local i, j, argname, gval

	-- compute default value
	if string.sub(arg, 1, 1) == "[" then
		if string.sub(arg, -1) ~= "]" then
			print(string.format("invalid argument syntax: %s, no default computation.", arg))
		else
			arg = string.sub(arg, 2, -2)	-- strip outer []
			i, j, argname = string.find(arg, "([%w_]+)%s*=")
			local f, errmsg = loadstring(arg)
			if not f then
				print(string.format("error compiling default argument value: %s, no default computation.", errmsg))
			else
				setfenv(f, vals)
				f()
			end
		end
	else
		argname = arg
	end

	-- prompt for variable with defalut value
	if vals[argname] then
		arg = string.format("%s[=%f]: ", argname, vals[argname])
	else
		arg = string.format("%s: ", argname)
	end
	io.stdout:write(arg)	-- prompt

	-- if user types EOF, return nil. look up in globals if typed var name
	local val = io.stdin:read("*l")
	if val == nil then return nil end
	gval = getfenv()[val]

	if not gval then
		if string.len(val) > 0 then
			val = tonumber(val) or val
			vals[argname] = val
		else
			val = vals[argname]
		end
	else
		val = gval
	end
	return val
end

-- Acquire arguments for the function. meth is function name. Returns nil if
-- user typed EOF.
local function query_args(meth)
	local vals = { n = 0 }

	-- print method description
	print(hf.METHODS[meth][2]);

	-- search method synopsis for arguments
	meth = hf.METHODS[meth][1];
	for arg in string.gfind(meth, "([^%s]+)") do
		local val = query_argument(arg, vals)
		if val == nil then return nil end
		vals.n = vals.n+1
		vals[vals.n] = val
	end
	return vals
end

-- Generate a unique image name. It is just I appended with 3-digit hex
-- counter.
local image_counter = 0
local function uniqname()
	image_counter = image_counter + 1
	return string.format("I%03x", image_counter)
end

-- Execute function. In interactive mode, if EOF is entered, the function
-- execution is canceled. hf.IMAGES is a two-way hash: from name->image and
-- from image key->name. This method first checks if the returned image
-- already exists. If so, it just returns that image. If not, it enters the
-- image in hf.IMAGES with a new, unique name.
--
-- meth is method name. Returns a table of all return values.
local function meth_exec(meth, arg)
	local im

	if arg.n == 0 then
		arg = query_args(meth)
		if arg == nil then
			print("CANCELED.")
			return nil
		end
	end

	local ret = {hf.METHODS[meth][3](unpack(arg))}
	if table.getn(ret) == 1 then im = ret[1] end

	if is_image(im) and not hf.IMAGES[im:_hkey()] then
		-- image doesn't exist, create a 2-way mapping
		local name = uniqname()
		hf.IMAGES[name] = im
		hf.IMAGES[im:_hkey()] = name

		-- inform the user
		print(string.format("new image %s", name))

		-- display the image
		RasterWindow:setImage(im)
	end

	return ret
end

--------------------------------------------------------GLOBAL IMAGE METATABLE
-- In interactive mode, global variable assignments are intercepted if the
-- object in question is heightfield. Instead in the global table, it is put
-- in the hf.IMAGES table. We don't allow multiple variable references to the
-- same image. In that case, the old name is deleted and new name is defined.
------------------------------------------------------------------------------

-- Called via metatable only when table[key] doesn't exist
local function hf_index(table, key)
	return table.hf.IMAGES[key]
end

-- Called via metatable only when creating new keys.
local function hf_newindex(table, key, value)
	local I = table.hf.IMAGES

	if I[key] then							-- key exists in the image table
		if I[key] == value then return end	-- assignment to self

		I[I[key]:_hkey()] = nil				-- remove ref to name
		_hf_delete(I[key])					-- deallocate previous hfield
		print(string.format("deleted %s", key))
		RasterWindow:setImage(nil)			-- prevent crash if deleted im is displayed
	else									-- key doesn't exist in img table
		local oldname = is_image(value) and I[value:_hkey()]
		if oldname then						-- put images in image table
			print(string.format("renaming %s to %s", oldname, key))
			I[oldname] = nil
		end
	end

	if is_image(value) then					-- put images in image table
		I[key] = value
		I[value:_hkey()] = key
	else
		I[key] = nil
		rawset(table, key, value)			-- value not image, put in globals
	end
end

setmetatable(getfenv(), { __index = hf_index, __newindex = hf_newindex })

----------------------------------------------------------ADDITIONAL FUNCTIONS
-- Help displaying, list of all images, etc. These are NOT in hf. namespace to
-- avoid meth_exec mechanism.
------------------------------------------------------------------------------
-- help function
function help(fun)
	if not fun then for k,v in pairs(hf.METHODS) do
		print(string.format("%s %s", k, v[1]))
	end else
		local v = hf.METHODS[fun]
		if v then
			print(string.format("SYNOPSIS: %s %s", fun, v[1]))
			print(v[2])
		else
			print(string.format("NO HELP FOR %s", fun))
		end
	end
end

-- function to list all defined images along with their dimensions and refcnt
function images()
	print(string.format("%-32s%8s%8s", "NAME", "WIDTH", "HEIGHT"))
	for k,v in pairs(hf.IMAGES) do
		if type(k) == "string" then
			local i = hf.IMAGES[k]
			print(string.format("%-32s%8d%8d", k, i.width, i.height))
		end
	end
end

-- function to delete all defined images
-- TODO: ask yes/no
function reset()
	print("Deleting all images...")
	for k,v in pairs(hf.IMAGES) do
		if is_image(v) then
			print(k)
			_hf_delete(v)
		end
	end
	hf.IMAGES = {}
	print("...done.")
end

----------------------------------------------------INTERACTIVE INITIALIZATION
-- Import functions from hf.METHODS. into hf. namespace. this constructs 
-- new functions which automatically display the resultnant image, manage
-- image variables, ask questions if invoked without parameters, etc. In
-- interactive mode, non-magaged versions are available with fully qualified
-- names, i.e. hf.METHODS.
------------------------------------------------------------------------------
for k,v in pairs(hf.METHODS) do
	local methname = k
	hf[k] = function(...)
		local ret = meth_exec(methname, arg)
		return unpack(ret)
	end
end

