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

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include "definitions.h"
#include "util.h"
#include "winmgr.h"
#include "xclient.h"
#include "xscreen.h"
#include "config.h"
#include "menu.h"

const long Menu::ButtonMask		= ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
const long Menu::MenuMask 		= Menu::ButtonMask|PointerMotionMask|ExposureMask;
const long Menu::MenuGrabMask		= Menu::ButtonMask|PointerMotionMask|StructureNotifyMask;

Menu::Menu(XScreen *s, MenuDef &md, Menu *p): m_data(md)
{
	m_screen = s;
	m_parent = p;
	m_child = NULL;
	m_nitems = m_data.items.size();
	m_rootwin = m_screen->get_window();
	m_font = m_screen->get_menu_font();
	m_titlecolor = m_screen->get_color(Color::MenuTitle);
	m_titlebgcolor = m_screen->get_color(Color::MenuTitleBackground);
	m_textcolor = m_screen->get_color(Color::MenuText);
	m_textselcolor = m_screen->get_color(Color::MenuTextSelected);
	m_bgcolor = m_screen->get_color(Color::MenuBackground);
	m_hicolor = m_screen->get_color(Color::MenuHighlight);
	m_bordercolor = m_screen->get_color(Color::MenuBorder);

	m_border = conf::border_menu;
	m_entry_height = m_font->ascent + m_font->descent;

	Position pos = xutil::get_pointer_pos(m_rootwin);
	Geometry area = m_screen->get_area(pos, 1);
	m_geom.w = get_menu_width();
	m_geom.h = (m_nitems + 1) * m_entry_height;
	if (m_parent) {
		int ypos = m_entry_height * (m_parent->get_active() + 1);
		m_geom.set_menu_placement(m_parent->get_geom(), area, ypos, m_border);
	} else {
		Position pos = xutil::get_pointer_pos(m_rootwin);
		m_geom.set_menu_placement(pos, area, m_border);
	}

	m_window =  XCreateSimpleWindow(wm::display, m_rootwin, m_geom.x, m_geom.y, m_geom.w, 
			m_geom.h, m_border, m_bordercolor->pixel, m_bgcolor->pixel);

	m_xftdraw = XftDrawCreate(wm::display, m_window, m_screen->get_visual(), 
				m_screen->get_colormap());

	XSelectInput(wm::display, m_window, MenuMask);
	XMapWindow(wm::display, m_window);
}

Menu::~Menu()
{
	XUnmapWindow(wm::display, m_window);
	XftDrawDestroy(m_xftdraw);
	XDestroyWindow(wm::display, m_window);
	if (m_child) delete m_child;
}


int Menu::get_menu_width()
{
	XGlyphInfo extents;
	XftTextExtentsUtf8(wm::display, m_font, (XftChar8 *)(m_data.label.c_str()),
					m_data.label.size(), &extents);
	int width = extents.width;
	for (auto item : m_data.items) {
		XftTextExtentsUtf8(wm::display, m_font, (XftChar8 *)(item.label.c_str()),
					item.label.size(), &extents);
		if (width < extents.width) width = extents.width;
	}

	XftTextExtentsUtf8(wm::display, m_font, (XftChar8 *)">", 1, &extents);
	m_submenu_char_width = extents.width;	
	width += m_submenu_char_width + 10; // add some spacing to the right
	return width;
}

bool Menu::run()
{
	XEvent	 e;
	Geometry g;
	Position pos;
	bool is_running = true;
	bool is_done = false;
	bool button_release = false;

	if (!grab_pointer()) return -1;

	int focusrevert;
	Window focuswin;
	XGetInputFocus(wm::display, &focuswin, &focusrevert);
	XSetInputFocus(wm::display, m_window, RevertToPointerRoot, CurrentTime);
	draw();

	m_active = -1;
	while (is_running) {
		XWindowEvent(wm::display, m_window, MenuMask, &e);
		switch (e.type) {
		case Expose:
			draw();
			break;
		case MotionNotify:
	 		pos = Position(e.xbutton.x, e.xbutton.y);
			move_pointer(pos);
			pos = xutil::get_pointer_pos(m_rootwin);
			if (m_child) {
				g = m_child->get_geom();
				if (g.contains(pos, Coordinates::Root)) {
					is_done = m_child->run();
					if (!grab_pointer()) return -1;
				}
			} else if (m_parent) {
				g = m_parent->get_geom();
				if (g.contains(pos, Coordinates::Root))
					is_running = false;
			}
			break;
		case ButtonRelease:
			button_release = true;
			is_running = false;
			break;
		default:
			break;
		}
		if (is_done) break;
	}

	XSetInputFocus(wm::display, focuswin, focusrevert, CurrentTime);
	XUngrabPointer(wm::display, CurrentTime);

	if ((!is_done) && (m_active != -1)) {
		switch(m_data.type) {
		case MenuType::Command:
			run_command(); 
			break;
		case MenuType::Desktop: 
			goto_desktop();
			break;
		case MenuType::Window: 
			activate_window();
			break;
		}
		is_done = true;
	}

	if (button_release)
		is_done = true;

	return is_done;
}

void Menu::draw()
{
	XClearWindow(wm::display, m_window);
	XMoveResizeWindow(wm::display, m_window, m_geom.x, m_geom.y,
	    		m_geom.w, m_geom.h);

	XftDrawRect(m_xftdraw, m_titlebgcolor, 0, 0, m_geom.w, m_entry_height);
	XftDrawStringUtf8(m_xftdraw, m_titlecolor, m_font, 3, m_font->ascent,
	    		(const FcChar8*)m_data.label.c_str(), m_data.label.size());

	Position pos = xutil::get_pointer_pos(m_window);
	m_active = get_active_entry(pos);	
	for (int i = 0; i< m_nitems; i++)
		draw_entry(i);

	XMapRaised(wm::display, m_window);
}

void Menu::draw_entry(int n)
{
	int y = (n + 1) * m_entry_height;
	XftColor *color = (n == m_active) ? m_hicolor : m_bgcolor;
	XftDrawRect(m_xftdraw, color, 0, y, m_geom.w, m_entry_height);

	color = (n == m_active) ? m_textselcolor : m_textcolor;
	XftDrawStringUtf8(m_xftdraw, color, m_font, 5, y + m_font->ascent,
	    (const FcChar8*)m_data.items[n].label.c_str(), m_data.items[n].label.size());

	if (!m_data.items[n].function.compare("menu")) {
		XftDrawStringUtf8(m_xftdraw, color, m_font, m_geom.w - m_submenu_char_width - 5, 
				y + m_font->ascent,
	    			(const FcChar8*)">", 1);
		if (n == m_active) open_submenu();
	} 

}

void Menu::move_pointer(Position p)
{
	int last_active = m_active;
	m_active = get_active_entry(p);

	if (last_active == m_active)
		return;

	if (last_active != -1) {
		draw_entry(last_active);
		if (!m_data.items[last_active].function.compare("menu")) {
			close_submenu();	
		}
	}
	if (m_active != -1) {
		draw_entry(m_active);
		if (!m_data.items[m_active].function.compare("menu")) {
			open_submenu();	
		}
	}
}

int Menu::get_active_entry(Position p)
{
	int active = p.y / m_entry_height - 1;
	if (p.x < -m_border || p.x > m_geom.w + m_border)
		active = -1;

	if ((active < 0) || (active >= m_nitems))
		active = -1;

	return active;
}

int Menu::grab_pointer()
{
	if (XGrabPointer(wm::display, m_window, False, MenuGrabMask, GrabModeAsync, 
		GrabModeAsync, None, wm::cursors[Pointer::ShapeNormal], CurrentTime) 
		!= GrabSuccess) {
		return 0;
	}
	return 1;
}

void Menu::open_submenu()
{
	if (m_child) return;
	std::string menupath = m_data.items[m_active].path;
	auto isSubMenu = [menupath] (MenuDef mdef) 
			{ return (!mdef.label.compare(menupath)); };
	auto it = std::find_if(conf::menulist.begin(),conf::menulist.end(), isSubMenu);
	if (it == conf::menulist.end()) return;
	m_child = new Menu(m_screen, *it, this);
	m_child->draw();
}

void Menu::close_submenu()
{
	if (m_child) delete m_child;
	m_child = NULL;
}

void Menu::run_command()
{
	std::string function = m_data.items[m_active].function;
	std::string path = m_data.items[m_active].path;
	if (!function.compare("exec"))
		util::spawn_process(path);
	if (!function.compare("quit"))
		wm::status =  IsQuitting;
	if (!function.compare("restart")) {
		if (!path.empty())
			wm::set_param_restart(path);
		wm::status =  IsRestarting;
	}
}

void Menu::activate_window()
{
	XClient *client = m_data.items[m_active].client;
	int index = client->get_desktop_index();
	int active_desktop = m_screen->get_active_desktop();
	if ((index != -1) && (index != active_desktop)) 
		m_screen->switch_to_desktop(index);
 	if (client->has_state(State::Hidden)) 
		client->show_window();
 	if (!(client->has_states(State::Ignored))) {
		client->raise_window();
		client->warp_pointer();
	}
}

void Menu::goto_desktop()
{
	int index = m_data.items[m_active].index;
	int active_desktop = m_screen->get_active_desktop();
	if (index != active_desktop)
		m_screen->switch_to_desktop(index); 	
}
