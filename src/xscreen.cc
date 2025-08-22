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

#include <X11/extensions/Xrandr.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "binding.h"
#include "config.h"
#include "desktop.h"
#include "menu.h"
#include "socket.h"
#include "timer.h"
#include "winmgr.h"
#include "wmhints.h"
#include "xclient.h"
#include "xscreen.h"

XScreen::XScreen(int id): m_screenid(id)
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [XScreen:" << __func__
			<< "] Add screen " << m_screenid << std::endl;
	}

	m_rootwin = RootWindow(wm::display, m_screenid);
	m_colormap = DefaultColormap(wm::display, m_screenid);
	m_visual = DefaultVisual(wm::display, m_screenid);
	m_cycling = false;

	m_bordergap = conf::bordergap;

	// Desktops
	for (DesktopDef &def : conf::desktop_defs)
		m_desktoplist.push_back(Desktop(def.name, this, def.index, def.mode, def.master_split));

	m_ndesktops = m_desktoplist.size();

	ewmh::set_net_supported(m_rootwin);
	ewmh::set_net_supported_wm_check(m_rootwin, conf::wmname);
	ewmh::unset_net_showing_desktop(m_rootwin);
	ewmh::delete_net_virtual_roots(m_rootwin);
	ewmh::set_net_number_of_desktops(m_rootwin, m_ndesktops);
	set_net_desktop_names();

	m_desktop_active = 0;
	m_desktop_last = 0;

	// Use the _NET_CURRENT_DESKTOP atom if it exists.
	long net_current_desktop;
	if (ewmh::get_net_current_desktop(m_rootwin, &net_current_desktop))
		m_desktop_active = net_current_desktop;
	else
		ewmh::set_net_current_desktop(m_rootwin, m_desktop_active);

	// fonts
	m_menufont = XftFontOpenName(wm::display, m_screenid, conf::menufont.c_str());
	if (!m_menufont) {
		std::cerr << timer::gettime() << " [XScreen::" << __func__
				<< "] Cant open font name '" << conf::menufont << "'\n";
		m_menufont = XftFontOpenName(wm::display, m_screenid, "Mono:size=10");
	}

	// colors
	for (std::string& def : conf::colordefs) {
		XftColor xc;
		if (!XftColorAllocName(wm::display, m_visual, m_colormap, def.c_str(), &xc)) {
			std::cerr << timer::gettime() << " [XScreen::" << __func__
					<< "] Cant allocate color for name '" << def << "'\n";
			XftColorAllocName(wm::display, m_visual, m_colormap, "gray50", &xc);
		}
		m_xftcolors.push_back(xc);
	}

	grab_keybindings();
	update_geometry();

	XSetWindowAttributes	 attr;
	attr.cursor = wm::cursors[Pointer::ShapeNormal];
	attr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
	    EnterWindowMask | PropertyChangeMask | ButtonPressMask;
	XChangeWindowAttributes(wm::display, m_rootwin, (CWEventMask | CWCursor), &attr);

	if (wm::xrandr)
		XRRSelectInput(wm::display, m_rootwin, RRScreenChangeNotifyMask);

	add_existing_clients();

}

XScreen::~XScreen()
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [XScreen:" << __func__
			<< "] REMOVE screen " << m_screenid << std::endl;
	}

	for (XClient *client : m_clientlist)
		delete client;

	for (XftColor &color : m_xftcolors)
		XftColorFree(wm::display, DefaultVisual(wm::display, m_screenid),
		    DefaultColormap(wm::display, m_screenid),
		    &color);

	XftFontClose(wm::display, m_menufont);
	XUngrabKey(wm::display, AnyKey, AnyModifier, m_rootwin);
}


void XScreen::grab_keybindings()
{
	XUngrabKey(wm::display, AnyKey, AnyModifier, m_rootwin);
	for (Binding& kb : conf::keybindings) {
		KeyCode kc = XKeysymToKeycode(wm::display, kb.keysym);
		if (!kc) {
			std::cerr << timer::gettime() << " XScreen::" << __func__
				<< "] Failed converting '" << XKeysymToString(kb.keysym)
				<< "' keysym to keycode" << std::endl;
			continue;
		}
		if ((XkbKeycodeToKeysym(wm::display, kc, 0, 0) != kb.keysym) &&
			(XkbKeycodeToKeysym(wm::display, kc, 0, 1) == kb.keysym))
			kb.modmask |= ShiftMask;
		for (auto mod : wm::ignore_mods)
			XGrabKey(wm::display, kc, (kb.modmask | mod), m_rootwin,
					True, GrabModeAsync, GrabModeAsync);
	}
}

XClient *XScreen::get_active_client()
{
	for (XClient *client : m_clientlist)
		if (client->has_state(State::Active)) return client;
	return NULL;
}

void XScreen::add_existing_clients()
{
	Window		*wins, w0, w1, rwin, cwin;
	unsigned int	 nwins, mask;
	int		 rx, ry, wx, wy;

	if (conf::debug) {
		std::cout << timer::gettime() << " [XScreen:" << __func__ << "]\n";
	}

	if (XQueryTree(wm::display, m_rootwin, &w0, &w1, &wins, &nwins)) {
		for (int i = 0; i < nwins; i++)
			if (can_manage(wins[i], true))
				m_clientlist.push_back(new XClient(wins[i], this, true));
		XFree(wins);
	}

	update_net_client_lists();

	for (int i = 0; i < m_ndesktops; i++) {
		if (i == m_desktop_active)
			m_desktoplist[i].show(m_clientlist);
		else
			m_desktoplist[i].hide(m_clientlist);
	}

	update_statusbar_desktops();

	// Obtain window where the pointer is positioned
	XQueryPointer(wm::display, m_rootwin, &rwin, &cwin,
	    		&rx, &ry, &wx, &wy, &mask);
	if (cwin == None) return;

	// Set pointed window as active
	for (XClient *client : m_clientlist) {
		if (cwin == client->get_window()) {
			if (!(client->has_states(State::Ignored)))
				client->set_window_active();
			break;
		}
	}
}

void XScreen::add_client(Window window)
{
	if (!can_manage(window, false)) return;
	if (conf::debug) {
		std::cout << timer::gettime() << " [XScreen:" << __func__ << "]\n";
	}
	m_clientlist.insert(m_clientlist.begin(), new XClient(window, this, false));

	update_net_client_lists();

	XClient *client = m_clientlist.front();
	if (client->has_state(State::Sticky)) {
		client->show_window();
		return;
	}

	int index = client->get_desktop_index();
	if (index == m_desktop_active) {
		m_desktoplist[index].show(m_clientlist);
	 	if (!(client->has_states(State::Ignored))) {
			client->warp_pointer();
			client->raise_window();
		}
	} else {
		switch_to_desktop(index);
	 	if (!(client->has_states(State::Ignored))) {
			client->warp_pointer();
			client->raise_window();
		}
	}
	update_statusbar_desktops();

}

bool XScreen::can_manage(Window w, bool query)
{
	XWindowAttributes	 wattr;

	if (w == None) return false;
	if (!XGetWindowAttributes(wm::display, w, &wattr)) return false;
	if (wattr.override_redirect) return false;
	if (query && wattr.map_state != IsViewable) return false;
	return true;
}

void XScreen::remove_client(XClient *client)
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [XScreen:" << __func__ << "] "
			<< std::hex << client->get_window() << std::endl;
	}

	long states = client->get_states();

	auto it = std::find(m_clientlist.begin(), m_clientlist.end(), client);
	if (it != m_clientlist.end()) {
		client->set_removed();
		delete client;
		m_clientlist.erase(it);
	}

	update_net_client_lists();
	if (states & State::Active) {
		XSetInputFocus(wm::display, PointerRoot, RevertToPointerRoot, CurrentTime);
		ewmh::set_net_active_window(m_rootwin, None);
		clear_statusbar_title();
	}

	m_desktoplist[m_desktop_active].show(m_clientlist);
	update_statusbar_desktops();
}

void XScreen::raise_client(XClient *client)
{
	if (m_cycling) return; 	// Dont change order of clients while cycling
	if (client->get_desktop_index() != m_desktop_active) return;
	if (client->has_state(State::Tiled)) return; // Dont change the order in tiling mode.

	auto it = std::find(m_clientlist.rbegin(), m_clientlist.rend(), client);
	if (it != m_clientlist.rend())
			std::rotate(m_clientlist.rbegin(), it, it+1);
}

void XScreen::move_client_to_desktop(XClient *client, long index)
{
	long desktop_index = client->get_desktop_index();
	if ((desktop_index == -1) || (desktop_index == index)) return;
	client->hide_window();
	client->assign_to_desktop(index);
	m_desktoplist[m_desktop_active].show(m_clientlist);
	update_statusbar_desktops();
}

void XScreen::show_desktop()
{
	m_desktoplist[m_desktop_active].show(m_clientlist);
}

void XScreen::hide_desktop()
{
	m_desktoplist[m_desktop_active].hide(m_clientlist);
}

void XScreen::close_desktop()
{
	m_desktoplist[m_desktop_active].close(m_clientlist);
}

void XScreen::select_desktop_mode(long mode)
{
	m_desktoplist[m_desktop_active].select_mode(m_clientlist, mode);
}

void XScreen::rotate_desktop_mode(long direction)
{
	m_desktoplist[m_desktop_active].rotate_mode(m_clientlist, direction);
}

void XScreen::update_net_client_lists()
{
	// _NET_CLIENT_LIST is ordered from oldest to newest
	std::vector<Window> netclientlist;
	for (auto it = m_clientlist.rbegin(); it != m_clientlist.rend(); it++)
		netclientlist.push_back((*it)->get_window());
	ewmh::set_net_client_list(m_rootwin, netclientlist);

	// _NET_CLIENT_LIST_STACKING use reverse order
	netclientlist.clear();
	for (auto it = m_clientlist.begin(); it != m_clientlist.end(); it++)
		netclientlist.push_back((*it)->get_window());
	ewmh::set_net_client_list_stacking(m_rootwin, netclientlist);
}

void XScreen::set_net_desktop_names()
{
	std::vector<std::string> names;

	for (Desktop &d : m_desktoplist)
		names.push_back(d.get_name());

	ewmh::set_net_desktop_names(m_rootwin, names);
}

void XScreen::clear_statusbar_title()
{
	if (!socket_out::defined()) return;
	std::string message = "no_window_active";
	socket_out::send(message);
}

void XScreen::update_statusbar_desktops()
{
	if (!socket_out::defined()) return;
	std::stringstream ss;
	for (int i = 0; i < m_ndesktops; i++) {
		char prefix = ' ';
		if (i == m_desktop_active)
			prefix = '+';
		else if (desktop_empty(i))
			continue;
		else if (desktop_urgent(i))
			prefix = '!';
		ss << prefix << i+1 << ' ';
	}
	std::string s(ss.str());
	std::string message = "desktop_list=" + s;
	socket_out::send(message);
}

void XScreen::update_geometry()
{
	m_view = Geometry(0, 0, DisplayWidth(wm::display, m_screenid),
				DisplayHeight(wm::display, m_screenid));
	m_work = m_view;
	m_work.apply_border_gap(m_bordergap);

	m_viewportlist.clear();
	if (wm::xrandr) {
		XRRScreenResources *sr;
		XRRCrtcInfo *ci;
		int i;

		sr = XRRGetScreenResources(wm::display, m_rootwin);
		for (i = 0, ci = NULL; i < sr->ncrtc; i++) {
			ci = XRRGetCrtcInfo(wm::display, sr, sr->crtcs[i]);
			if (ci == NULL)
				continue;
			if (ci->noutput == 0) {
				XRRFreeCrtcInfo(ci);
				continue;
			}
			Viewport viewport(i, ci->x, ci->y, ci->width, ci->height, m_bordergap);
			m_viewportlist.push_back(viewport);

			XRRFreeCrtcInfo(ci);
		}
		XRRFreeScreenResources(sr);
	} else {
		Viewport viewport(0, m_view, m_bordergap);
		m_viewportlist.push_back(viewport);
	}

	ewmh::set_net_desktop_geometry(m_rootwin, m_view);
	ewmh::set_net_desktop_viewport(m_rootwin);
	ewmh::set_net_workarea(m_rootwin, m_ndesktops, m_work);
}

Geometry XScreen::get_area(Position &p, bool gap)
{
	Geometry area = m_view;

	for (Viewport &v : m_viewportlist) {
		if (v.contains(p)) {
			area = v.get_view();
			break;
		}
	}

	if (gap) area.apply_border_gap(m_bordergap);
	return area;
}

Viewport *XScreen::find_viewport(Position &p)
{
	for (Viewport &v : m_viewportlist) {
		if (v.contains(p))
			return &v;
	}
	return NULL;
}

// Bring back clients which are beyond the screen.
void XScreen::ensure_clients_are_visible()
{
	for (XClient *client : m_clientlist) {
		Geometry &geom = client->get_geometry();
		if (!geom.intersects(m_view, client->get_border())) {
			geom.set_pos(m_bordergap.left, m_bordergap.top);
			client->move_window();
		}
	}
}

void XScreen::cycle_windows(long direction)
{
	XClient *client = get_active_client();
	if (!client) return;

	XGrabKeyboard(wm::display, m_rootwin, True, GrabModeAsync, GrabModeAsync,
			CurrentTime);
	m_cycling = true;  // Reset when mod key is released.
	m_desktoplist[m_desktop_active].cycle_windows(m_clientlist, client, direction);
}

void XScreen::cycle_desktops(long direction)
{
	int nextdesktop = m_desktop_active;
	int showdesktop = -1;

	for (;;) {
		nextdesktop = (direction == -1) ? --nextdesktop: ++nextdesktop;

		if (nextdesktop < 0) nextdesktop = m_ndesktops - 1;
		if (nextdesktop == m_ndesktops) nextdesktop = 0;

		if (nextdesktop == m_desktop_active)
			break;

		if ((!desktop_empty(nextdesktop)) && showdesktop == -1) {
			showdesktop = nextdesktop;
		}
	}
	if (showdesktop == -1)
		return;

	switch_to_desktop(showdesktop);
}

void XScreen::rotate_desktop_tiles(long direction)
{
	m_desktoplist[m_desktop_active].rotate_windows(m_clientlist, direction);
}

void XScreen::swap_desktop_tiles(long direction)
{
	XClient *client = get_active_client();
	if (!client) return;

	XGrabKeyboard(wm::display, m_rootwin, True, GrabModeAsync, GrabModeAsync,
			CurrentTime);
	m_cycling = true;  // Reset when mod key is released.
	m_desktoplist[m_desktop_active].swap_windows(m_clientlist, client, direction);
}

void XScreen::desktop_master_resize(long increment)
{
	m_desktoplist[m_desktop_active].master_resize(m_clientlist, increment);
}

bool XScreen::desktop_empty(long index)
{
	for (XClient *c : m_clientlist)
		if (c->get_desktop_index() == index) return false;
	return true;
}

bool XScreen::desktop_urgent(long index)
{
	for (XClient *c : m_clientlist)
		if ((c->get_desktop_index() == index) &&
			(c->has_state(State::Urgent)))
			return true;
	return false;
}

void XScreen::switch_to_desktop(int index)
{
	m_desktoplist[m_desktop_active].hide(m_clientlist);
	m_desktoplist[index].show(m_clientlist);
	m_desktop_last = m_desktop_active;
	m_desktop_active = index;
	ewmh::set_net_current_desktop(m_rootwin, m_desktop_active);
	update_statusbar_desktops();
}

void XScreen::run_launcher_menu()
{
	std::string menu = conf::menu_launcher_label;
	auto isLauncherMenu = [menu] (MenuDef mdef)
			{ return (!mdef.label.compare(menu)); };
	auto it = std::find_if(conf::menulist.begin(),conf::menulist.end(), isLauncherMenu);
	if (it == conf::menulist.end()) return;

	Menu appmenu(this, *it);
	appmenu.run();
}

void XScreen::populate_client_menu(MenuDef &menudef)
{
	menudef.items.clear();
	for (int i = -1; i < m_ndesktops; i++) {
		for (XClient *client : m_clientlist) {
			if (client->has_states(State::Ignored)) continue;
			long index = client->get_desktop_index();
			if (index != i) continue;
			char ws = '+';
			if (client->has_state(State::Active)) ws = '#';
			if (client->has_state(State::Hidden)) ws = '.';
			if (client->has_state(State::Urgent)) ws = '!';
			std::stringstream ss;
			if (index < 0)
				ss << "[s]" << ws  << ' ' << client->get_name();
			else
				ss << "[" << index+1 << "]" << ws << ' '
					<< client->get_name();

			std::string s(ss.str().substr(0,127));
			menudef.items.push_back(MenuItem(s, client));
		}
	}
}

void XScreen::run_client_menu()
{
	MenuDef menudef(conf::menu_client_label, MenuType::Client);
	populate_client_menu(menudef);
	Menu client_menu(this, menudef);
	client_menu.run();
}

void XScreen::populate_desktop_menu(MenuDef &menudef)
{
	menudef.items.clear();
	for (int i = 0; i < m_ndesktops; i++) {
		if (desktop_empty(i)) continue;
		std::stringstream ss;
		ss << "[" << m_desktoplist[i].get_name() << "]";
		std::string s(ss.str());
		menudef.items.push_back(MenuItem(s,i));
	}
}

void XScreen::run_desktop_menu()
{
	MenuDef menudef(conf::menu_desktop_label, MenuType::Desktop);
	populate_desktop_menu(menudef);
	Menu desktop_menu(this, menudef);
	desktop_menu.run();
}

XClient *XScreen::find_active_client()
{
	for (XScreen *screen : wm::screenlist)
		for (XClient *client : screen->m_clientlist)
			if (client->has_state(State::Active)) return client;
	return NULL;
}

XScreen *XScreen::find_screen(Window win)
{
	for (XScreen *screen : wm::screenlist) {
		if (screen->m_rootwin == win) return screen;
	}
	return NULL;
}

XClient *XScreen::find_client(Window win)
{
	auto isClient = [win](XClient *c) { return (c->has_window(win)); };

	for (XScreen *screen : wm::screenlist) {
		auto it = std::find_if(screen->m_clientlist.begin(), screen->m_clientlist.end(),
					isClient);
		if (it != screen->m_clientlist.end()) {
			return *it;
		}
	}
	return NULL;
}
