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

#ifndef _MENU_H_
#define _MENU_H_
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include "geometry.h"

class XClient;
class XScreen;

// Menu item
struct MenuItem {
	std::string 	 label;
	std::string	 function;
	std::string 	 path;
	XClient 	*client;
	long 		 index;
	MenuItem(std::string &l) : label(l) {}
	MenuItem(std::string &l, std::string &f)
		: label(l), function(f), path("") {}
	MenuItem(std::string &l, std::string &f, std::string &p)
		: label(l), function(f), path(p) {}
	MenuItem(std::string &l, XClient *c) : label(l), client(c) {}
	MenuItem(std::string &l, long i) : label(l), index(i) {}
};

// Menu content
struct MenuDef {
	std::string 			label;
	MenuType			type;
	std::vector<MenuItem>		items;
	MenuDef(const char *l, MenuType t): label(l), type(t) {}
	MenuDef(std::string &l, MenuType t): label(l), type(t) {}
};

// Menu object
class Menu {
	XScreen				*m_screen;
	Menu				*m_parent;
	Menu				*m_child;
	MenuDef				&m_data;
	int				 m_nitems;
	int				 m_active;
	Window				 m_window;
	Window				 m_rootwin;
	XftFont				*m_font;
	XftDraw				*m_xftdraw;
	unsigned long			 m_bgpixel;
	unsigned long			 m_fgpixel;
	XftColor			*m_titlecolor;
	XftColor			*m_titlebgcolor;
	XftColor			*m_textcolor;
	XftColor			*m_textselcolor;
	XftColor			*m_bgcolor;
	XftColor			*m_hicolor;
	XftColor			*m_bordercolor;
	Geometry			 m_geom;
	int				 m_border;
	int				 m_entry_height;
	Window				 m_submenu_char_width;
	bool				 m_running;
public:
	Menu(XScreen *s, MenuDef &d, Menu *p = NULL);
	~Menu();
	Geometry 		&get_geom() { return m_geom; }
	int 			 get_active() const { return m_active; }
	bool 			 run();
	void 			 draw();
private:
	int 			 get_menu_width();
	int 			 grab_pointer();
	void 			 draw_entry(int);
	void 			 move_pointer(Position);
	int 			 get_active_entry(Position);
	void 			 open_submenu();
	void 			 close_submenu();
	void			 run_launcher();
	void 			 activate_client();
	void			 switch_to_desktop();
	static const long 	 ButtonMask;
	static const long 	 MenuMask;
	static const long 	 MenuGrabMask;
};
#endif /* _MENU_H_ */
