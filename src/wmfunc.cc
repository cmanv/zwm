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

#include <csignal>
#include <sstream>
#include <string>
#include <vector>
#include "process.h"
#include "config.h"
#include "desktop.h"
#include "xclient.h"
#include "xscreen.h"
#include "wmcore.h"
#include "wmfunc.h"

namespace wmfunc {
const long free_param = 99999;

std::vector<FuncDef> funcdefs = {
	{ "desktop-close", 		desktop_close},
	{ "desktop-hide", 		desktop_hide},
	{ "desktop-layout-1", 		desktop_select_layout, 0},
	{ "desktop-layout-2", 		desktop_select_layout, 1},
	{ "desktop-layout-3", 		desktop_select_layout, 2},
	{ "desktop-layout-4", 		desktop_select_layout, 3},
	{ "desktop-layout-5", 		desktop_select_layout, 4},
	{ "desktop-layout-6", 		desktop_select_layout, 5},
	{ "desktop-layout-7", 		desktop_select_layout, 6},
	{ "desktop-layout-8", 		desktop_select_layout, 7},
	{ "desktop-layout-9", 		desktop_select_layout, 8},
	{ "desktop-layout-next", 	desktop_rotate_layout, 1},
	{ "desktop-layout-prev", 	desktop_rotate_layout, -1},
	{ "desktop-set-light-theme",	desktop_set_theme, Theme::Light},
	{ "desktop-set-dark-theme",	desktop_set_theme, Theme::Dark},
	{ "desktop-switch-1", 		desktop_switch, 0},
	{ "desktop-switch-2", 		desktop_switch, 1},
	{ "desktop-switch-3", 		desktop_switch, 2},
	{ "desktop-switch-4", 		desktop_switch, 3},
	{ "desktop-switch-5", 		desktop_switch, 4},
	{ "desktop-switch-6", 		desktop_switch, 5},
	{ "desktop-switch-7", 		desktop_switch, 6},
	{ "desktop-switch-8", 		desktop_switch, 7},
	{ "desktop-switch-9", 		desktop_switch, 8},
	{ "desktop-switch-10", 		desktop_switch, 9},
	{ "desktop-switch-last", 	desktop_switch_last},
	{ "desktop-switch-next", 	desktop_cycle, 1},
	{ "desktop-switch-prev", 	desktop_cycle, -1},
	{ "activate-client", 		activate_client, free_param},
	{ "desktop-window-focus-next", 	desktop_window_cycle, 1},
	{ "desktop-window-focus-prev", 	desktop_window_cycle, -1},
	{ "desktop-window-rotate-next",	desktop_rotate_tiles, 1},
	{ "desktop-window-rotate-prev",	desktop_rotate_tiles, -1},
	{ "desktop-window-swap-next", 	desktop_swap_tiles, 1},
	{ "desktop-window-swap-prev", 	desktop_swap_tiles, -1},
	{ "desktop-window-master-incr",	desktop_master_resize, 1},
	{ "desktop-window-master-decr",	desktop_master_resize, -1},

	{ "window-lower", 		window_lower},
	{ "window-hide", 		window_hide},
	{ "window-raise", 		window_raise},
	{ "window-close", 		window_close},
	{ "window-move-to-desktop-1", 	window_to_desktop, 0},
	{ "window-move-to-desktop-2", 	window_to_desktop, 1},
	{ "window-move-to-desktop-3", 	window_to_desktop, 2},
	{ "window-move-to-desktop-4", 	window_to_desktop, 3},
	{ "window-move-to-desktop-5", 	window_to_desktop, 4},
	{ "window-move-to-desktop-6", 	window_to_desktop, 5},
	{ "window-move-to-desktop-7", 	window_to_desktop, 6},
	{ "window-move-to-desktop-8", 	window_to_desktop, 7},
	{ "window-move-to-desktop-9", 	window_to_desktop, 8},
	{ "window-move-to-desktop-10", 	window_to_desktop, 9},
	{ "window-snap-up", 		window_snap, Direction::North},
	{ "window-snap-down", 		window_snap, Direction::South},
	{ "window-snap-right", 		window_snap, Direction::East},
	{ "window-snap-left", 		window_snap, Direction::West},
	{ "window-move", 		window_move, Direction::Pointer},
	{ "window-move-up", 		window_move, Direction::North},
	{ "window-move-down", 		window_move, Direction::South},
	{ "window-move-right", 		window_move, Direction::East},
	{ "window-move-left", 		window_move, Direction::West},
	{ "window-resize", 		window_resize, Direction::Pointer},
	{ "window-resize-up", 		window_resize, Direction::North},
	{ "window-resize-down", 	window_resize, Direction::South},
	{ "window-resize-right", 	window_resize, Direction::East},
	{ "window-resize-left", 	window_resize, Direction::West},
	{ "window-toggle-fullscreen", 	window_state, State::FullScreen},
	{ "window-toggle-sticky", 	window_state, State::Sticky},
	{ "window-toggle-tiled", 	window_state, State::NoTile},

	{ "terminal", 			exec_term},
	{ "restart", 			set_wm_status, IsRestarting},
	{ "quit", 			set_wm_status, IsQuitting},
	{ "exec", 			exec_cmd},
};
}

void wmfunc::window_move(XClient *client, long direction)
{
	if (direction == Direction::Pointer)
		client->move_window_with_pointer();
	else
		client->move_window_with_keyboard(direction);
}

void wmfunc::window_resize(XClient *client, long direction)
{
	if (direction == Direction::Pointer)
		client->resize_window_with_pointer();
	else
		client->resize_window_with_keyboard(direction);
}

void wmfunc::window_snap(XClient *client, long direction)
{
	client->snap_window(direction);
}

void wmfunc::window_close(XClient *client, long)
{
	client->close_window();
}

void wmfunc::window_lower(XClient *client, long)
{
	client->save_pointer();
	client->lower_window();
}

void wmfunc::window_raise(XClient *client, long)
{
	client->raise_window();
}

void wmfunc::window_hide(XClient *client, long)
{
	client->hide_window();
}

void wmfunc::window_state(XClient *client, long state)
{
	client->toggle_state(state);
	if (!client->has_state(State::FullScreen)) {
		XScreen *screen = client->get_screen();
		screen->show_desktop();
	}
}

void wmfunc::window_to_desktop(XClient *client, long index)
{
	XScreen *screen = client->get_screen();
	screen->move_client_to_desktop(client, index);
}

void wmfunc::activate_client(XScreen *screen, long windowid)
{
	screen->activate_client(windowid);
}

void wmfunc::desktop_switch(XScreen *screen, long index)
{
	screen->switch_to_desktop(index);
}

void wmfunc::desktop_switch_last(XScreen *screen, long)
{
	int last = screen->get_last_desktop();
	screen->switch_to_desktop(last);
}

void wmfunc::desktop_hide(XScreen *screen, long)
{
	screen->hide_desktop();
}

void wmfunc::desktop_close(XScreen *screen, long)
{
	screen->close_desktop();
}

void wmfunc::desktop_master_resize(XScreen *screen, long increment)
{
	screen->desktop_master_resize(increment);
}

void wmfunc::desktop_select_layout(XScreen *screen, long index)
{
	screen->select_desktop_layout(index);
}

void wmfunc::desktop_rotate_layout(XScreen *screen, long direction)
{
	screen->rotate_desktop_layout(direction);
}

void wmfunc::desktop_cycle(XScreen *screen, long direction)
{
	screen->cycle_desktops(direction);
}

void wmfunc::desktop_window_cycle(XScreen *screen, long direction)
{
	screen->cycle_windows(direction);
}

void wmfunc::desktop_rotate_tiles(XScreen *screen, long direction)
{
	screen->rotate_desktop_tiles(direction);
}

void wmfunc::desktop_swap_tiles(XScreen *screen, long direction)
{
	screen->swap_desktop_tiles(direction);
}

void wmfunc::desktop_set_theme(XScreen *screen, long theme)
{
	screen->set_theme(theme);
}

void wmfunc::set_wm_status(long status)
{
	wm::status = status;
}

void wmfunc::exec_term(long)
{
	process::spawn(conf::terminal);
}

void wmfunc::exec_cmd(std::string &cmd)
{
	process::spawn(cmd);
}
