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

#include <csignal>
#include <sstream>
#include <string>
#include <vector>
#include "util.h"
#include "config.h"
#include "winmgr.h"
#include "menu.h"
#include "desktop.h"
#include "xclient.h"
#include "xscreen.h"
#include "wmfunc.h"

namespace wmfunc {
std::vector<FuncDef> funcdefs = {
	{ "root-menu-window", 		rootmenu_window, Context::Root},
	{ "root-menu-desktop", 		rootmenu_desktop, Context::Root},
	{ "root-menu-app", 		rootmenu_app, Context::Root},
	{ "window-titlebar-move", 	window_titlebar_move, Context::TitleBar},
	{ "window-titlebar-raise", 	window_titlebar_raise, Context::TitleBar},
	{ "window-titlebar-lower", 	window_titlebar_lower, Context::TitleBar},
	{ "window-button-hide", 	window_button_hide, Context::LeftButton},
	{ "window-button-close", 	window_button_close, Context::RightButton},
	{ "window-left-resize", 	window_left_resize, Context::LeftHandle},
	{ "window-middle-resize",	window_middle_resize, Context::MiddleHandle},
	{ "window-right-resize", 	window_right_resize, Context::RightHandle},

	{ "window-lower", 		window_lower},
	{ "window-hide", 		window_hide},
	{ "window-raise", 		window_raise},
	{ "window-close", 		window_close},
	{ "window-toggle-fullscreen", 	window_state, State::FullScreen},
	{ "window-toggle-sticky", 	window_state, State::Sticky},
	{ "window-toggle-tiled", 	window_state, State::Tiled},
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
	{ "window-snap-up-right", 	window_snap, Direction::NorthEast},
	{ "window-snap-up-left", 	window_snap, Direction::NorthWest},
	{ "window-snap-down-right", 	window_snap, Direction::SouthEast},
	{ "window-snap-down-left", 	window_snap, Direction::SouthWest},
	{ "window-move-up", 		window_move, Direction::North},
	{ "window-move-down", 		window_move, Direction::South},
	{ "window-move-right", 		window_move, Direction::East},
	{ "window-move-left", 		window_move, Direction::West},
	{ "window-resize-up", 		window_resize, Direction::North},
	{ "window-resize-down", 	window_resize, Direction::South},
	{ "window-resize-right", 	window_resize, Direction::East},
	{ "window-resize-left", 	window_resize, Direction::West},

	{ "desktop-select-1", 		desktop_select, 0},
	{ "desktop-select-2", 		desktop_select, 1},
	{ "desktop-select-3", 		desktop_select, 2},
	{ "desktop-select-4", 		desktop_select, 3},
	{ "desktop-select-5", 		desktop_select, 4},
	{ "desktop-select-6", 		desktop_select, 5},
	{ "desktop-select-7", 		desktop_select, 6},
	{ "desktop-select-8", 		desktop_select, 7},
	{ "desktop-select-9", 		desktop_select, 8},
	{ "desktop-select-10", 		desktop_select, 9},
	{ "desktop-last", 		desktop_last},
	{ "desktop-hide", 		desktop_hide},
	{ "desktop-close", 		desktop_close},

	{ "desktop-mode-next", 		desktop_rotate_mode, 1},
	{ "desktop-mode-prev", 		desktop_rotate_mode, -1},
	{ "desktop-window-next", 	desktop_window_cycle, 1},
	{ "desktop-window-prev", 	desktop_window_cycle, -1},
	{ "desktop-next", 		desktop_cycle, 1},
	{ "desktop-prev", 		desktop_cycle, -1},
	{ "desktop-rotate-next", 	desktop_rotate_tiles, 1},
	{ "desktop-rotate-prev", 	desktop_rotate_tiles, -1},
	{ "desktop-master-incr",	desktop_master, 1},
	{ "desktop-master-decr",	desktop_master, -1},

	{ "terminal", 			exec_term},
	{ "restart", 			set_wm_status, IsRestarting},
	{ "quit", 			set_wm_status, IsQuitting},
	{ "exec", 			exec_cmd},
};
}

void wmfunc::window_move(XClient *client, long direction)
{
	client->move_window_with_keyboard(direction);
}

void wmfunc::window_resize(XClient *client, long direction)
{
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
	if (!client->has_states(State::FullScreen)) {
		XScreen *screen = client->get_screen();
		screen->show_desktop();
	}
}

void wmfunc::window_to_desktop(XClient *client, long index)
{
	XScreen *screen = client->get_screen();
	screen->move_client_to_desktop(client, index);
}

void wmfunc::desktop_select(XScreen *screen, long index)
{
	screen->switch_to_desktop(index);
}

void wmfunc::desktop_last(XScreen *screen, long)
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

void wmfunc::desktop_master(XScreen *screen, long increment)
{
	screen->desktop_master(increment);
}

void wmfunc::desktop_rotate_mode(XScreen *screen, long direction)
{
	screen->rotate_desktop_mode(direction);
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

void wmfunc::window_button_close(XClient *client)
{
	client->close_window_with_button();
}

void wmfunc::window_button_hide(XClient *client)
{
	client->hide_window_with_button();
}

void wmfunc::window_titlebar_move(XClient *client)
{
	client->move_window_with_pointer();
}

void wmfunc::window_titlebar_lower(XClient *client)
{
	client->lower_window();
}

void wmfunc::window_titlebar_raise(XClient *client)
{
	client->raise_window();
}

void wmfunc::window_left_resize(XClient *client)
{
	client->resize_window_with_pointer(Handle::Left);
}

void wmfunc::window_middle_resize(XClient *client)
{
	client->resize_window_with_pointer(Handle::Middle);
}

void wmfunc::window_right_resize(XClient *client)
{
	client->resize_window_with_pointer(Handle::Right);
}

void wmfunc::rootmenu_app(XScreen *screen)
{
	screen->run_rootmenu_app();
}

void wmfunc::rootmenu_window(XScreen *screen)
{
	screen->run_rootmenu_window();
}

void wmfunc::rootmenu_desktop(XScreen *screen)
{
	screen->run_rootmenu_desktop();
}

void wmfunc::set_wm_status(long status)
{
	wm::status = status;
}

void wmfunc::exec_term(long)
{
	util::spawn_process(conf::terminal);
}

void wmfunc::exec_cmd(std::string &cmd)
{
	util::spawn_process(cmd);
}
