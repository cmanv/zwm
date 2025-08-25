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

#include <X11/Xlib.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "timer.h"
#include "socket.h"
#include "config.h"
#include "winmgr.h"
#include "xclient.h"
#include "xpointer.h"
#include "xscreen.h"
#include "desktop.h"

Desktop::Desktop(std::string &name, XScreen *screen, long index,
			std::string &mode_name, float split)
{
	m_name = name;
	m_screen = screen;
	m_index = index;
	m_master_split = split;
	m_mode_index = 0;
	for (int i = 0; i < conf::desktop_modes.size(); i++) {
		if (!mode_name.compare(conf::desktop_modes[i].name)) {
			m_mode_index = i;
			break;
		}
	}

	m_mode = conf::desktop_modes[m_mode_index].mode;
	m_rows = conf::desktop_modes[m_mode_index].rows;
	m_cols = conf::desktop_modes[m_mode_index].cols;
}

void Desktop::rotate_windows(std::vector<XClient*>&clientlist, long direction)
{
	if (!(m_mode & Mode::Tiling)) return;
	if (clientlist.size() < 2) return;

	if (direction < 0)  {
		auto first = clientlist.end();
		auto second = clientlist.end();
		auto last = clientlist.end();

		for (auto it = clientlist.begin(); it != clientlist.end(); it++) {
			XClient *c = *it;
			if (c->get_desktop_index() != m_index) continue;
			if (c->has_state(State::NoTile)) continue;
			if (first == clientlist.end()) {
				first = it;
				second = it + 1;
			}
			last = it + 1;
		}
		std::rotate(first, second, last);
	} else {
		auto first = clientlist.rend();
		auto second = clientlist.rend();
		auto last = clientlist.rend();
		for (auto it = clientlist.rbegin(); it != clientlist.rend(); it++) {
			XClient *c = *it;
			if (c->get_desktop_index() != m_index) continue;
			if (c->has_state(State::NoTile)) continue;
			if (first == clientlist.rend()) {
				first = it;
				second = it + 1;
			}
			last = it + 1;
		}
		std::rotate(first, second, last);
	}
	show(clientlist);
}

void Desktop::cycle_windows(std::vector<XClient*>&clientlist, XClient *client, long direction)
{
	if (m_mode & Mode::Monocle) return;
	if (clientlist.size() < 2) return;
	if (client->get_desktop_index() != m_index) return;

	XClient *client_next = NULL;
	if (direction > 0) {
		auto it = next_desktop_client(clientlist, client);
		if (it == clientlist.end()) return;
		client_next = *it;
	} else {
		auto it = prev_desktop_client(clientlist, client);
		if (it == clientlist.rend()) return;
		client_next = *it;
	}
	if (client == client_next) return;

	client->save_pointer();
	client_next->raise_window();
	Position &p = client_next->get_saved_pointer();
	Geometry &g = client_next->get_geometry();
	if (!g.contains(p, Coordinates::Window)) {
		p = g.get_center(Coordinates::Window);
	}
	client_next->warp_pointer();
}

void Desktop::swap_windows(std::vector<XClient*>&clientlist, XClient *client, long direction)
{
	if (!(m_mode & Mode::Swapable)) return;
	if (clientlist.size() < 2) return;
	if (client->get_desktop_index() != m_index) return;

	if (direction > 0) {
		auto current = std::find(clientlist.begin(), clientlist.end(), client);
		if (current == clientlist.end()) return;
		auto next = next_desktop_client(clientlist, client);
		if (next == clientlist.end()) return;
		if (current != next) iter_swap(current, next);
	} else {
		auto current = std::find(clientlist.rbegin(), clientlist.rend(), client);
		if (current == clientlist.rend()) return;
		auto prev = prev_desktop_client(clientlist, client);
		if (prev == clientlist.rend()) return;
		if (current != prev) iter_swap(current, prev);
	}
	client->save_pointer();
	show(clientlist);
	Position &p = client->get_saved_pointer();
	Geometry &g = client->get_geometry();
	if (!g.contains(p, Coordinates::Window)) {
		p = g.get_center(Coordinates::Window);
	}
	client->warp_pointer();
}

// Find next client on the desktop
std::vector<XClient*>::iterator Desktop::next_desktop_client(
		std::vector<XClient*>&clientlist, XClient *client)
{
	if (clientlist.empty()) return clientlist.end();

	auto current = std::find(clientlist.begin(), clientlist.end(), client);
	if (current == clientlist.end()) return current;

	auto isNext = [idx = m_index](XClient *c) {
		return ((c->get_desktop_index() == idx)
			&& (!c->has_state(State::SkipCycle))); };

	auto next = find_if(current+1, clientlist.end(), isNext);
	if (next != clientlist.end()) return next;
	return find_if(clientlist.begin(), current, isNext);
}

// Find previous client on the desktop
std::vector<XClient*>::reverse_iterator Desktop::prev_desktop_client(
		std::vector<XClient*>&clientlist, XClient *client)
{
	if (clientlist.empty()) return clientlist.rend();

	auto current = std::find(clientlist.rbegin(), clientlist.rend(), client);
	if (current == clientlist.rend()) return current;

	auto isPrev = [idx = m_index](XClient *c) {
		return ((c->get_desktop_index() == idx)
			&& (!c->has_state(State::SkipCycle))); };

	auto prev = find_if(current+1, clientlist.rend(), isPrev);
	if (prev != clientlist.rend()) return prev;
	return find_if(clientlist.rbegin(), current, isPrev);
}

void Desktop::show(std::vector<XClient*> &clientlist)
{
	restack_windows(clientlist);
	switch (m_mode)
	{
	case Mode::Monocle:
		tile_maximized(clientlist);
		break;
	case Mode::VTiled:
		tile_vertical(clientlist);
		break;
	case Mode::HTiled:
		tile_horizontal(clientlist);
		break;
	case Mode::Grid:
		tile_grid(clientlist);
		break;
	default:
		stacked_desktop(clientlist);
		break;
	}

	for (XClient *client : clientlist)
		if (client->has_state(State::Sticky)) {
			client->show_window();
	}

	statusbar_update_mode();
}

void Desktop::statusbar_update_mode()
{
	if (!socket_out::defined()) return;
	std::string message = "desktop_mode="
				+ conf::desktop_modes[m_mode_index].name;
	socket_out::send(message);
}

void Desktop::hide(std::vector<XClient*> &clientlist)
{
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() == m_index)
			client->hide_window();
	}
	m_screen->statusbar_clear_title();
}

void Desktop::close(std::vector<XClient*> &clientlist)
{
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() == m_index)
			client->close_window();
	}
	m_screen->statusbar_clear_title();
}

void Desktop::restack_windows(std::vector<XClient*>&clientlist)
{
	std::vector<Window> winlist;
	for (auto it=clientlist.begin(); it!=clientlist.end(); it++) {
		XClient *client = *it;
		if (client->get_desktop_index() == m_index)
			winlist.push_back(client->get_window());
	}
	XRestackWindows(wm::display, (Window *)winlist.data(), winlist.size());
}

void Desktop::select_mode(std::vector<XClient*> &clientlist, long index)
{
	if ((index < 0) || (index >= conf::desktop_modes.size())) return;
	m_mode_index = index;
	m_mode = conf::desktop_modes[index].mode;
	m_cols = conf::desktop_modes[index].cols;
	m_rows = conf::desktop_modes[index].rows;

	// For Monocle, put the active window on top
	if (m_mode == Mode::Monocle) {
		XClient *client = m_screen->get_active_client();
		if (client) {
			auto it = std::find(clientlist.begin(), clientlist.end(), client);
			if (it != clientlist.end())
				std::rotate(clientlist.begin(), it, it+1);

		}
	}

	show(clientlist);
}

void Desktop::rotate_mode(std::vector<XClient*> &clientlist, long direction)
{
	m_mode_index += direction;
	if (m_mode_index == conf::desktop_modes.size()) m_mode_index = 0;
	else if (m_mode_index < 0) m_mode_index = conf::desktop_modes.size() - 1;
	m_mode =  conf::desktop_modes[m_mode_index].mode;
	m_cols =  conf::desktop_modes[m_mode_index].cols;
	m_rows =  conf::desktop_modes[m_mode_index].rows;
	show(clientlist);
}

void Desktop::master_resize(std::vector<XClient*> &clientlist, long increment)
{
	if (!(m_mode & Mode::MasterSlave)) return;
	if (increment > 0) {
		m_master_split += 0.01;
		if (m_master_split > 0.9) m_master_split = 0.9;
	} else {
		m_master_split -= 0.01;
		if (m_master_split < 0.1) m_master_split = 0.1;
	}
	show(clientlist);
}

void Desktop::stacked_desktop(std::vector<XClient*>&clientlist)
{
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		client->clear_states(State::Tiled|State::Frozen|State::Hidden);
		client->set_stacked_geom();
		client->show_window();
	}
}

void Desktop::tile_grid(std::vector<XClient*>&clientlist)
{
	Position p = xpointer::get_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::tiled_border;
	int width = area.w / m_cols;
	int height = area.h / m_rows;
	int w = width - 2 * border;
	int h = height - 2 * border;
	int row = 0;
	int col = 0;

	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) continue;
		if (row < m_rows) {
			int x = area.x + col * width;
			int y = area.y + row * height;
			client->set_states(State::Tiled|State::Frozen);
			Geometry geom(x, y, w, h);
			client->set_tiled_geom(geom);
			client->show_window();
		} else {
			client->set_states(State::Hidden);
			client->hide_window();
		}

		col++;
		if (col == m_cols) {
			col = 0;
			row ++;
		}
	}

	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) {
			client->show_window();
			client->raise_window();
		}
	}
}

void Desktop::tile_horizontal(std::vector<XClient*>&clientlist)
{
	int x, y, w, h;

	Position p = xpointer::get_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::tiled_border;

	float mh = area.h;
	int nwins = 0;
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (!client->has_state(State::NoTile)) nwins++;
	}
	if (nwins > 1) {
		mh *= m_master_split;
		x = area.x;
		y = area.y + mh;
		w = area.w/(nwins -1);
		h = area.h - mh;
	}

	Geometry geom_master(area.x, area.y, area.w - 2 * border , mh - 2 * border);
	bool master = true;
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) continue;
		client->set_states(State::Tiled|State::Frozen);
		if (master) {
			client->set_states(State::HMaximized);
			client->set_tiled_geom(geom_master);
			client->statusbar_update_title();
			master = false;
		} else {
			client->clear_states(State::HMaximized);
			Geometry slave(x, y, w - 2 * border, h - 2 * border);
			client->set_tiled_geom(slave);
			x += w;
		}
		client->show_window();
	}

	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) {
			client->show_window();
			client->raise_window();
		}
	}
}

void Desktop::tile_vertical(std::vector<XClient*>&clientlist)
{
	int x, y, w, h;

Position p = xpointer::get_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::tiled_border;

	float mw = area.w;
	int nwins = 0;
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (!client->has_state(State::NoTile)) nwins++;
	}
	if (nwins > 1) {
		mw *= m_master_split;
		x = area.x + mw;
		y = area.y;
		w = area.w - mw;
		h = area.h/(nwins -1);
	}

	Geometry geom_master(area.x, area.y, mw - 2 * border, area.h - 2 * border);
	bool master = true;
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) continue;
		client->set_states(State::Tiled|State::Frozen);
		if (master) {
			client->set_states(State::VMaximized);
			client->set_tiled_geom(geom_master);
			client->statusbar_update_title();
			master = false;
		} else {
			client->clear_states(State::VMaximized);
			Geometry slave(x, y, w - 2 * border, h - 2 * border);
			client->set_tiled_geom(slave);
			y += h;
		}
		client->show_window();
	}

	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) {
			client->show_window();
			client->raise_window();
		}
	}
}

void Desktop::tile_maximized(std::vector<XClient*>&clientlist)
{
	Position p = xpointer::get_pos(m_screen->get_window());
	Geometry area = m_screen->get_area(p, true);
	int border = conf::tiled_border;

	bool master = true;
	Geometry maximized(area.x, area.y, area.w - 2 * border, area.h - 2 * border);
	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) continue;
		client->set_states(State::Tiled|State::Maximized|State::Frozen);
		if (master) {
			client->clear_states(State::Hidden);
			client->set_tiled_geom(maximized);
			client->show_window();
			client->statusbar_update_title();
			master = false;
		} else {
			client->set_states(State::Hidden);
			client->hide_window();
		}
	}

	for (XClient *client : clientlist) {
		if (client->get_desktop_index() != m_index) continue;
		if (client->has_state(State::NoTile)) {
			client->show_window();
			client->raise_window();
		}
	}
}
