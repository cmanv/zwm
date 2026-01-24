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

#ifndef _WMCORE_H_
#define _WMCORE_H_
#include <signal.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>

class XScreen;

namespace wm {
	extern Display				*display;
	extern std::string			 displayname;
	extern Time				 last_event_time;
	extern volatile sig_atomic_t 		 status;
	extern int				 xrandr;
	extern int				 xrandr_event_base;
	extern std::vector<Cursor> 		 cursors;
	extern std::vector<XScreen*>		 screenlist;
	extern const std::vector<unsigned int>	 ignore_mods;
	void		 run(void);
	void		 set_param_restart(int, char**);
	void		 set_param_restart(std::string &);
}
#endif // _WMCORE_H_
