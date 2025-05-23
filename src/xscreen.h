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

#ifndef _XSCREEN_H_
#define _XSCREEN_H_
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <string>
#include <list>
#include <vector>
#include "xobjects.h"

class Desktop;
class XClient;

class XScreen {
	int			 	 m_screenid;
	Window			 	 m_rootwin;
	bool				 m_cycling;

	Geometry	 		 m_view; // viewable area
	Geometry			 m_work; // workable area, gap-applied
	BorderGap	 		 m_bordergap;

	std::list<XClient*>		 m_clientlist;
	std::vector<XClient*> 		 m_stickylist;
	std::vector<Desktop>		 m_desktoplist;
	std::vector<Viewport>		 m_viewportlist;

	long			 	 m_ndesktops;
	long				 m_desktop_active;
	long				 m_desktop_last;

	Visual				*m_visual;
	Colormap			 m_colormap;
	std::vector<XftColor>		 m_xftcolors;
	XftFont				*m_windowfont;
	XftFont				*m_menufont;

public:
	XScreen(int);
	~XScreen();
	int			 	 get_screenid() const { return m_screenid; }
	Window			 	 get_window() const { return m_rootwin; }
	Visual			 	*get_visual() const { return m_visual; }
	Colormap		 	 get_colormap() const { return m_colormap; }
	long			 	 get_active_desktop() const { return m_desktop_active; }
	long			 	 get_last_desktop() const { return m_desktop_last; }
	std::list<XClient*> 		&get_clients() { return m_clientlist; }
	std::vector<Desktop> 		&get_desktops() { return m_desktoplist; }
	XftFont				*get_window_font() { return m_windowfont; }
	XftFont				*get_menu_font() { return m_menufont; }
	XftColor			*get_color(Color c) { return &m_xftcolors[c]; }
	unsigned long			 get_pixel(Color c) const { return m_xftcolors[c].pixel; }
	long	 		 	 get_num_desktops() const { return m_ndesktops; }
	Geometry	 		 get_view() const { return m_view; }
	bool			 	 is_cycling() const { return m_cycling; }
	void				 stop_cycling() { m_cycling = false; };

	void				 grab_keybindings();
	XClient				*get_active_client();
	void 				 query_clients();
	void 				 add_client(Window);
	bool  				 can_manage(Window, bool);
	void 				 remove_client(XClient *);
	void 				 remove_sticky(XClient *);
	void 				 update_net_client_lists();
	void 				 move_client_to_desktop(XClient *, long);
	void 				 raise_client(XClient *);
	void 				 set_net_desktop_names();
	void 				 clear_statusbar_title();
	void 				 update_statusbar_desktops();

	void 				 add_window_tile_to_desktop(XClient *);
	void 				 remove_window_tile_from_desktop(XClient *);
	void 				 assign_client_to_desktop(XClient *, long, bool);
	void 				 show_desktop();
	void 				 hide_desktop();
	void 				 close_desktop();

	void				 update_geometry();
	Geometry			 get_area(Position&, bool);
	Viewport 			*find_viewport(Position&);
	void	 			 ensure_clients_are_visible();
	void	 			 move_pointer(long);

	void	 			 cycle_windows(long);
	void	 			 cycle_desktops(long);
	void 				 rotate_desktop_mode(long);
	void	 			 rotate_desktop_tiles(long);
	void 				 desktop_master(long);
	void	 			 switch_to_desktop(int);
	void	 			 update_desktop_names();

	void				 run_rootmenu_app();
	void				 run_rootmenu_window();
	void				 run_rootmenu_desktop();

	static XClient			*find_active_client();
	static XClient			*find_client(Window);
	static XScreen			*find_screen(Window);
};

#endif /* _XSCREEN_H_ */
