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

#ifndef _WINMGR_H_
#define _WINMGR_H_
#include <signal.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include "xobjects.h"

class XScreen;

struct StateMap {
	Atom atom;
	long state;
	StateMap(Atom a, long s):atom(a), state(s) {}
};

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

	extern std::vector<Atom>		 ewmh;
	extern std::vector<Atom>		 hints;
	extern std::vector<StateMap> 		 statemaps;

	void		 run(void);
	void		 set_param_restart(int, char**);
	void		 set_param_restart(std::string &);

	// EWMH functions
	void		 set_net_supported(Window);
	void		 set_net_supported_wm_check(Window, std::string&);
	void		 set_net_desktop_geometry(Window, Geometry&);
	void		 set_net_desktop_viewport(Window);
	void		 set_net_workarea(Window, int, Geometry&);
	void		 set_net_client_list(Window, std::vector<Window>&);
	void		 set_net_client_list_stacking(Window, std::vector<Window>&);
	void		 set_net_active_window(Window, Window);
	void		 set_net_number_of_desktops(Window, int);
	void		 unset_net_showing_desktop(Window);
	void		 delete_net_virtual_roots(Window);

	int		 get_net_current_desktop(Window, long *);
	void		 set_net_current_desktop(Window, int);
	void		 set_net_desktop_names(Window, std::vector<std::string>&);

	long		 get_wm_state(Window);
	void		 set_wm_state(Window, long);

	int		 get_net_wm_desktop(Window, long *);
	void		 set_net_wm_desktop(Window, int);

	int		 get_net_wm_window_type(Window, std::vector<Atom>&);

	int		 get_net_wm_state_atoms(Window, std::vector<Atom>&);
	long		 get_net_wm_states(Window, long);
	void		 set_net_wm_states(Window, long);

	void		*get_window_property(Window, Atom, Atom, long, unsigned long *);
	int		 get_text_property(Window, Atom, std::vector<char>&);
	void		 send_client_message(Window, Atom, Time);
}
#endif // _WINMGR_H_
