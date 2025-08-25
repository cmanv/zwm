// zwm - a minimal stacking/tiling window manager for X11
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
#include "enums.h"
#include "binding.h"
#include "menu.h"

struct DesktopDef {
	std::string 	name;
	std::string	mode;
	float		master_split;
};

struct DefaultDesktop {
	std::string 	resname;
	std::string 	resclass;
	long 		index;
	DefaultDesktop(std::string &n, std::string &c, long i)
		:resname(n), resclass(c), index(i) {}
};

struct DesktopMode {
	std::string	name;
	long		mode;
	long		rows;
	long		cols;
	DesktopMode(std::string n, long m, long r, long c)
		:name(n), mode(m), rows(r), cols(c) {}
};

struct DefaultStates {
	std::string 	resname;
	std::string 	resclass;
	long		states;
	DefaultStates(std::string &n, std::string &c, long s)
		:resname(n), resclass(c), states(s) {}
};

namespace conf {
	extern std::string 			 command_socket;
	extern std::string 			 message_socket;
	extern std::string 			 menufont;
	extern std::string 			 startupscript;
	extern std::string 			 shutdownscript;
	extern std::string			 user_config;
	extern std::string			 wmname;
	extern int				 debug;
	extern int				 stacked_border;
	extern int				 tiled_border;
	extern int				 menu_border;
	extern int				 moveamount;
	extern int				 snapdist;
	extern BorderGap			 bordergap;
	extern const int			 ndesktops;
	extern std::vector<DesktopDef> 	 	 desktop_defs;
	extern std::vector<DesktopMode> 	 desktop_modes;
	extern std::vector<std::string> 	 colordefs;
	extern std::vector<Binding>		 keybindings;
	extern std::vector<Binding>  		 mousebindings;
	extern std::vector<MenuDef>		 menulist;
	extern std::vector<DefaultDesktop>	 defdesktoplist;
	extern std::vector<DefaultStates>	 defstateslist;
	extern std::string			 terminal;
	extern std::string			 menu_client_label;
	extern std::string			 menu_desktop_label;
	extern std::string			 menu_launcher_label;
	void 	init();
}
#endif // _CONFIG_H_
