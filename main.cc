/*
  This file is a part of the Raster Alchemy package.
  Copyright (C) 2004  Zeljko Vrba

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  You can reach me at the following e-mail addresses:

  zvrba@globalnet.hr
  mordor@fly.srk.fer.hr
*/
#include <stdio.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fox/fx.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "rdispwin.h"

extern "C" {
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
}
#include <luabind/luabind.hpp>

static char rcsid[] = "$Id: main.cc,v 1.2.2.11 2004/09/24 17:18:23 zvrba Exp $";

// must be global!
RasterDisplayWindow *RasterWindow;
FXApp *RasterAlchemyApplication;
int GuiSocket[2];

static char *rl_gets(const char *prompt)
{
	static char *line_read = NULL;
	
	if(line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline(prompt);
	if(line_read && *line_read) add_history(line_read);
	return line_read;
}
	
static void *console_thread(void *data)
{
	lua_State *Lua = (lua_State*)data;
	char *input;

	while((input = rl_gets("rasteralchemy> "))) {
		try {
			if(luaL_loadbuffer(Lua, input, strlen(input), "=stdin")) {
				luabind::object top(Lua); top.set();
				fprintf(stderr, "COMPILE ERROR: %s\n",
						luabind::object_cast<const char*>(top));
			} else {
				if(lua_pcall(Lua, 0, 0, 0)) {
					luabind::object top(Lua); top.set();
					fprintf(stderr, "RUNTIME ERROR: %s\n",
							luabind::object_cast<const char*>(top));
				}
			}
		} catch(luabind::error &e) {
			fprintf(stderr, "RUNTIME ERROR: %s\n", e.what());
		}
	}
	exit(0);
}

int main(int argc, char **argv)
{
	lua_State *Lua;
	void register_lua_rdispwin(lua_State*);
	void register_lua_hf(lua_State*);
	void register_lua_sclxform(lua_State*);

	pthread_t console_thr;
	pthread_attr_t console_thr_attr;

	// create sockets for communication with GUI thread. [0] will be used
	// for reading, 1 for writing.
	if(socketpair(PF_UNIX, SOCK_DGRAM, 0, GuiSocket) < 0) {
		perror("socketpair");
		return 1;
	}

	// initial window creation
	RasterAlchemyApplication = new FXApp("Raster Alchemy", "ZAX");
	RasterAlchemyApplication->init(argc, argv);
	RasterWindow = new RasterDisplayWindow(RasterAlchemyApplication, "mainwin");
	RasterAlchemyApplication->addInput(GuiSocket[0], INPUT_READ, RasterWindow, 0);
	RasterAlchemyApplication->create();
	RasterWindow->show();

	// initialize lua interpreter
	Lua = lua_open();
	lua_baselibopen(Lua);
	lua_tablibopen(Lua);
	lua_iolibopen(Lua);
	lua_strlibopen(Lua);
	luabind::open(Lua);
	register_lua_hf(Lua);
	register_lua_rdispwin(Lua);
	register_lua_sclxform(Lua);
	if(lua_dofile(Lua, "hf.lua")) {
		fprintf(stderr, "FATAL ERROR: can't load hf.lua. exiting.\n");
		exit(1);
	}
	
	// put console in separate thread
	pthread_attr_init(&console_thr_attr);
	pthread_attr_setdetachstate(&console_thr_attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&console_thr, &console_thr_attr, console_thread, Lua) < 0) {
		perror("pthread_create");
		exit(1);
	}

	// run GUI
	RasterAlchemyApplication->run();
	return 0;
}
