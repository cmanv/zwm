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

#include <X11/Xlib.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "util.h"
#include "config.h"
#include "winmgr.h"
#include "xclient.h"
#include "xscreen.h"
#include "desktop.h"

Desktop::Desktop(XScreen *screen, long index, std::string &name, long mode, float split)
{
	m_screen = screen;
	m_name = name;
	m_index = index;
	m_mode = mode;
	m_split = split;
}

void Desktop::add_window(XClient *client)
{
	if (conf::debug > 1) {
		std::cout << util::gettime() << " [Desktop::" << __func__ 
			<< "] ADD Client ptr window 0x" << std::hex << client->get_window() 
			<< std::endl;
	}

	m_clientstack.push_back(client);
	if (client->has_state(State::Tiled)) 
		add_window_tile(client);
}

void Desktop::remove_window(XClient *client)
{
	if (conf::debug > 1) {
		std::cout << util::gettime() << " [Desktop::" << __func__ 
			<< "] REMOVE Client ptr window 0x" << std::hex << client->get_window() 
			<< std::endl;
	}

	auto itstack = std::find(m_clientstack.begin(), m_clientstack.end(), client);
	if (itstack != m_clientstack.end())
			m_clientstack.erase(itstack);

	remove_window_tile(client);
}

void Desktop::add_window_tile(XClient *client)
{
	m_clienttile.insert(m_clienttile.begin(), client);
}

void Desktop::remove_window_tile(XClient *client)
{
	auto ittile = std::find(m_clienttile.begin(), m_clienttile.end(), client);
	if (ittile != m_clienttile.end())
			m_clienttile.erase(ittile);
}

void Desktop::raise_window(XClient *client)
{
	auto it = std::find(m_clientstack.rbegin(), m_clientstack.rend(), client);
	if (it != m_clientstack.rend())
			std::rotate(m_clientstack.rbegin(), it, it+1);
}

void Desktop::rotate_windows(long direction)
{
	if (m_clienttile.size() < 2) return;

	if (direction > 0) {
		auto it = m_clienttile.begin() + 1;
		std::rotate(m_clienttile.begin(), it, m_clienttile.end());
	} else {
		auto it = m_clienttile.rbegin() + 1;
		std::rotate(m_clienttile.rbegin(), it, m_clienttile.rend());
	}
	show();
}

void Desktop::cycle_windows(XClient *old_client, long direction)
{
	if (m_clientstack.empty()) 
		return;

	XClient *new_client = (direction == -1) ?  prev_window(old_client)
				: next_window(old_client);

	if (old_client == new_client) return;

	old_client->save_pointer();
	new_client->raise_window();
	Position &p = new_client->get_saved_pointer();
	Geometry &g = new_client->get_geometry();
	if (!g.contains(p, Coordinates::Window)) {
		p = g.get_center(Coordinates::Window);
	}
	new_client->warp_pointer();
}

// Find next visible client on the desktop
XClient *Desktop::next_window(XClient *client)
{
	if (m_clientstack.empty()) return client;

	auto current = std::find(m_clientstack.rbegin(), m_clientstack.rend(), client);
	if (current == m_clientstack.rend()) 
		return client;

	auto isNext = [](XClient *c) { return (!c->has_state(State::SkipCycle)); };
	auto next = std::find_if(current+1, m_clientstack.rend(), isNext);
	if (next == m_clientstack.rend())
		next = std::find_if(m_clientstack.rbegin(), current, isNext);

	return *next;
}

// Find previous visible client on the desktop
XClient *Desktop::prev_window(XClient *client)
{
	if (m_clientstack.empty()) return client;

	auto current = std::find(m_clientstack.begin(), m_clientstack.end(), client);
	if (current == m_clientstack.end()) 
		return client;

	auto isPrev = [](XClient *c) { return (!c->has_state(State::SkipCycle)); };
	auto prev = std::find_if(current+1, m_clientstack.end(), isPrev);
	if (prev == m_clientstack.end())
		prev = std::find_if(m_clientstack.begin(), current, isPrev);

	return *prev;
}

void Desktop::show()
{
	std::string mode = "";

	restack_windows();
	switch(m_mode) {
	case DeskMode::HTiled:	
		mode = "H";	
		tile_horizontal();
		break;
	case DeskMode::VTiled:	
		mode = "V";	
		tile_vertical();
		break;
	case DeskMode::Monocle:	
		mode = "M";	
		tile_maximized();
		break;
	}

	if (!conf::clientsocket.length())
		return;
	std::string message = "desktop_mode=" + mode;
	util::send_message(message);
}

void Desktop::hide()
{
	for (XClient *client : m_clientstack)
		client->hide_window();
	m_screen->clear_statusbar_title();
}

void Desktop::close()
{
	for (XClient *client : m_clientstack)
		client->close_window();
	m_screen->clear_statusbar_title();
}

void Desktop::restack_windows()
{
	std::vector<Window> winlist;
	for (auto it=m_clientstack.rbegin(); it!=m_clientstack.rend(); it++) {
		XClient *c = *it;
		winlist.push_back(c->get_window());	
	}
	XRestackWindows(wm::display, (Window *)winlist.data(), winlist.size());
}

void Desktop::toggle()
{
	if (has_hidden_only())
		show();
	else
		hide();
}

void Desktop::rotate_mode(long direction)
{
	m_mode += direction;
	if (m_mode > DeskMode::Monocle) m_mode = DeskMode::VTiled;
	if (m_mode < DeskMode::VTiled) m_mode = DeskMode::Monocle;
	show();
}

bool Desktop::has_hidden_only()
{
	for (XClient *client : m_clientstack) {
		if (!(client->has_state(State::Hidden)))
			return false;
	}
	return true;
}

void Desktop::tile_horizontal()
{
	int x, y, w, h;
	
	Position p = xutil::get_pointer_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::border_tile;

	float mh = area.h;
	int nwins = m_clienttile.size();
	if (nwins > 1) {
		mh *= m_split;
		x = area.x;
		y = area.y + mh;
		w = area.w/(nwins -1);
		h = area.h - mh;
	}

	Geometry geom_master(area.x, area.y, area.w - 2 * border , mh - 2 * border);
	bool master = true;
	for (XClient *client : m_clienttile) {
		if (master) {
			client->set_states(State::HMaximized|State::Frozen);
			client->set_tiling(geom_master);
//			client->warp_pointer();
			client->update_statusbar_title();
			master = false;
		} else {
			Geometry slave(x, y, w - 2 * border, h - 2 * border);
			client->clear_states(State::HMaximized);
			client->set_states(State::Frozen);
			client->set_tiling(slave);
			x += w;
		}
		client->set_border(border);
		client->show_window();
	}

	for (XClient *client : m_clientstack) {
		if (!client->has_state(State::Tiled)) {
			client->show_window();
			client->raise_window();
		}
	}
}

void Desktop::tile_vertical()
{
	int x, y, w, h;

	Position p = xutil::get_pointer_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::border_tile;

	float mw = area.w;
	int nwins = m_clienttile.size();
	if (nwins > 1) {
		mw *= m_split;
		x = area.x + mw;
		y = area.y;
		w = area.w - mw;
		h = area.h/(nwins -1);
	}

	Geometry geom_master(area.x, area.y, mw - 2 * border, area.h - 2 * border);
	bool master = true;
	for (XClient *client : m_clienttile) {
		if (master) {
			client->set_states(State::VMaximized|State::Frozen);
			client->set_tiling(geom_master);
//			client->warp_pointer();
			client->update_statusbar_title();
			master = false;
		} else {
			Geometry slave(x, y, w - 2 * border, h - 2 * border);
			client->set_states(State::Frozen);
			client->clear_states(State::VMaximized);
			client->set_tiling(slave);
			y += h;
		}
		client->set_border(border);
		client->show_window();
	}

	for (XClient *client : m_clientstack) {
		if (!client->has_state(State::Tiled)) {
			client->show_window();
			client->raise_window();
		}
	}
}

void Desktop::master_split(long increment)
{
	if ((m_mode != DeskMode::HTiled) && (m_mode != DeskMode::VTiled))
		return;

	if (increment > 0) {
		m_split += 0.01;
		if (m_split > 0.9) m_split = 0.9;	
	} else {
		m_split -= 0.01;
		if (m_split < 0.1) m_split = 0.1;	
	}
	show();
}

void Desktop::tile_maximized()
{
	Position p = xutil::get_pointer_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::border_tile;

	Geometry maximized(area.x, area.y, area.w - 2 * border, area.h - 2 * border);
	bool master = true;
	for (XClient *client : m_clienttile) {
		if (master) {
			client->set_states(State::Maximized|State::Frozen);
			client->set_border(border);
			client->set_tiling(maximized);
//			client->warp_pointer();
			client->show_window();
			client->update_statusbar_title();
			master = false;
		} else {
			client->set_states(State::Maximized|State::Frozen|State::Hidden);
			client->set_border(border);
			client->set_tiling(maximized);
			client->hide_window();
		}
	}

	for (XClient *client : m_clientstack) {
		if (!client->has_state(State::Tiled)) {
			client->show_window();
			client->raise_window();
		}
	}
}
