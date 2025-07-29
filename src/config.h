// zwm - a simple dynamic tiling window manager for X11
//
// Copyright (c) 2025 cmanv
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <vector>
#include <string>
#include "definitions.h"
#include "xobjects.h"
#include "binding.h"
#include "menu.h"

namespace conf {
	extern std::string			 cfilename;
	extern std::string			 wmname;
	extern std::string 			 windowfont;
	extern std::string 			 menufont;
	extern std::string 			 clientsocket;
	extern std::string 			 serversocket;
	extern std::string 			 startupscript;
	extern std::string 			 shutdownscript;

	extern int				 debug;
	extern int				 border_float;
	extern int				 border_tile;
	extern int				 border_menu;
	extern int				 moveamount;
	extern int				 snapdist;
	extern BorderGap			 bordergap;
	extern const int			 ndesktops;

	extern std::vector<DesktopDef> 	 	 desktopdefs;
	extern std::vector<std::string> 	 colordefs;
	extern std::vector<Binding>		 keybindings;
	extern std::vector<Binding>  		 mousebindings;
	extern std::vector<MenuDef>		 menulist;
	extern std::vector<DefaultDesktop>	 defdesktoplist;
	extern std::vector<DefaultStates>	 defstateslist;
	extern std::string			 terminal;
	extern std::string			 appmenu;
	extern std::string			 windowmenu;
	extern std::string			 desktopmenu;

	void 	init();
}

#endif // _CONFIG_H_
