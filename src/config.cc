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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "util.h"
#include "wmfunc.h"
#include "binding.h"
#include "xclient.h"
#include "menu.h"
#include "config.h"

namespace conf {
	std::vector<DesktopDef> desktop_defs = {
		{ 0, "one", "Stacked", 0.5 },
		{ 1, "two", "Stacked", 0.5 },
		{ 2, "three", "Stacked", 0.5 },
		{ 3, "four", "Stacked", 0.5 },
		{ 4, "five", "Stacked", 0.5 },
		{ 5, "six", "Stacked", 0.5 },
		{ 6, "seven", "Stacked", 0.5 },
		{ 7, "eigth", "Stacked", 0.5 },
		{ 8, "nine", "Stacked", 0.5 },
		{ 9, "ten", "Stacked", 0.5 },
	};

	std::vector<DesktopMode> desktop_modes = {
		{ 0,	"Stacked",	"F" },
		{ 1,	"Monocle",	"M" },
		{ 2,	"VTiled",	"V" },
		{ 3,	"HTiled",	"H" },
	};

	std::vector<BindingDef>	keybinding_defs = {
		{ "M-1",	"desktop-select-1" },
		{ "M-2",	"desktop-select-2" },
		{ "M-3",	"desktop-select-3" },
		{ "M-4",	"desktop-select-4" },
		{ "M-5",	"desktop-select-5" },
		{ "M-6",	"desktop-select-6" },
		{ "M-7",	"desktop-select-7" },
		{ "M-8",	"desktop-select-8" },
		{ "M-9",	"desktop-select-9" },
		{ "M-0",	"desktop-select-10" },
		{ "CM-Right",	"desktop-next" },
		{ "CM-Left",	"desktop-prev" },
		{ "SM-Right",	"desktop-mode-next" },
		{ "SM-Left",	"desktop-mode-prev" },
		{ "M-Tab",	"desktop-window-next" },
		{ "SM-Tab",	"desktop-window-prev" },
		{ "SM-1",	"window-move-to-desktop-1" },
		{ "SM-2",	"window-move-to-desktop-2" },
		{ "SM-3",	"window-move-to-desktop-3" },
		{ "SM-4",	"window-move-to-desktop-4" },
		{ "SM-5",	"window-move-to-desktop-5" },
		{ "SM-6",	"window-move-to-desktop-6" },
		{ "SM-7",	"window-move-to-desktop-7" },
		{ "SM-8",	"window-move-to-desktop-8" },
		{ "SM-9",	"window-move-to-desktop-9" },
		{ "SM-0",	"window-move-to-desktop-10" },
		{ "SM-f",	"window-toggle-fullscreen" },
		{ "SM-s",	"window-toggle-sticky" },
		{ "SM-t",	"window-toggle-tiled" },
		{ "SM-i",	"window-hide" },
		{ "SM-x",	"window-close" },
		{ "M-Down",	"window-lower" },
		{ "M-Up",	"window-raise" },
		{ "M-h",	"window-move-left" },
		{ "M-l",	"window-move-right" },
		{ "M-j",	"window-move-down" },
		{ "M-k",	"window-move-up" },
		{ "CM-h",	"window-snap-left" },
		{ "CM-l",	"window-snap-right" },
		{ "CM-j",	"window-snap-down" },
		{ "CM-k",	"window-snap-up" },
		{ "CM-Return",	"terminal" },
		{ "CM-r",	"restart" },
		{ "CM-q",	"quit" },
	};

	std::vector<BindingDef> mousebinding_defs = {
		{ "1",		"root-menu-window" },
		{ "2",		"root-menu-desktop" },
		{ "3",		"root-menu-app" },
		{ "M-1",	"window-move" },
		{ "M-3",	"window-resize" },
	};

	std::vector<std::string> cfilenames = {
		"/.config/zwm/config",
		"/.zwmrc"
	};

	std::string		cfilename = "";
	std::string		wmname = "ZWM";
	std::string 		windowfont = "Mono:size=10";
	std::string 		menufont = "Mono:size=10";
	std::string		appmenu = "Applications";
	std::string		windowmenu = "Windows";
	std::string		desktopmenu = "Active desktops";
	std::string		serversocket = "";
	std::string		clientsocket = "";
	std::string		terminal = "xterm";
	std::string		startupscript = "";
	std::string		shutdownscript = "";

	std::vector<std::string> colordefs;

	int			debug = 0;
	const int		ndesktops = desktop_defs.size();
	int			border_menu = 1;
	int			border_tile = 2;
	int			border_stack = 5;
	int			moveamount = 10;
	int			snapdist = 9;
	BorderGap		bordergap = { 1, 1, 1, 1 };

	std::vector<Binding>		keybindings;
	std::vector<Binding>  		mousebindings;
	std::vector<DefaultDesktop>	defdesktoplist;
	std::vector<DefaultStates>	defstateslist;
	std::vector<MenuDef>		menulist;

	bool get_line(std::ifstream &, std::string &);
	int  get_tokens(std::string &, std::vector<std::string> &);
	int  split_string(std::string &, std::vector<std::string> &, char);
	void get_name_class(std::string&, std::string&, std::string&);
	void add_keybinding(Binding&);
	void remove_keybinding(Binding &);
	void add_mousebinding(Binding&);
	void remove_mousebinding(Binding &);
	void add_desktop_modes(std::vector<std::string> &);
	void add_window_states(std::string &, std::string &, std::vector<std::string> &);
	void add_default_desktop(std::string &, std::string &, int);
	void add_menu(std::string &, std::ifstream &);
}

void conf::init()
{
	for (BindingDef& def : keybinding_defs) {
		Binding kb(def, EventType::Key);
		if (kb.valid) keybindings.push_back(kb);
	}

	for (BindingDef& def : mousebinding_defs) {
		Binding mb(def, EventType::Button);
		if (mb.valid) mousebindings.push_back(mb);
	}

	colordefs.resize(Color::NumColors);
	colordefs[Color::WindowBorderActive] 	= "tan";
	colordefs[Color::WindowBorderInactive] 	= "grey40";
	colordefs[Color::WindowBorderUrgent] 	= "red";
	colordefs[Color::MenuBackground] 	= "grey21";
	colordefs[Color::MenuBorder] 		= "SkyBlue4";
	colordefs[Color::MenuHighlight] 	= "SteelBlue4";
	colordefs[Color::MenuText] 		= "grey85";
	colordefs[Color::MenuTextSelected] 	= "WhiteSmoke";
	colordefs[Color::MenuTitle] 		= "WhiteSmoke";
	colordefs[Color::MenuTitleBackground] 	= "SkyBlue4";

	const char *wm_socket = std::getenv("WM_SERVER_SOCKET");
	if (wm_socket) {
		serversocket = wm_socket;
	}
	const char *statusbar_socket = std::getenv("WM_CLIENT_SOCKET");
	if (statusbar_socket) {
		clientsocket = statusbar_socket;
	}

	if (!std::getenv("HOME")) {
		std::cerr << "HOME is not defined in the environment!\n";
		return;
	}

	if (!cfilename.size()) {
		for (auto filename : cfilenames) {
			std::string file = std::getenv("HOME") + filename;
			std::filesystem::path f(file);
			if (std::filesystem::exists(f)) {
				cfilename = file;
				break;
			}
		}
	}
	if (!cfilename.size()) return;

	std::ifstream configfile(cfilename);
	if (!configfile.is_open()) {
        	std::cerr << "Could not open [" << cfilename << "] for reading.\n";
		return;
	}

       	std::string line;
	std::vector<std::string> tokens;
	while (get_line(configfile, line)) {
		if (!get_tokens(line, tokens)) continue;

		// Rest need at least 2 tokens
		if (tokens.size() < 2)
			continue;

		if (!tokens[0].compare("debug-level")) {
			debug = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("border-tile")) {
			border_tile = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("border-stack")) {
			border_stack = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("border-gap")) {
			if (tokens.size() < 5) continue;
			bordergap.top = std::strtol(tokens[1].c_str(), NULL, 10);
			bordergap.bottom = std::strtol(tokens[2].c_str(), NULL, 10);
			bordergap.left = std::strtol(tokens[3].c_str(), NULL, 10);
			bordergap.right = std::strtol(tokens[4].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("desktop-modes")) {
			std::vector<std::string> modes;
			split_string(tokens[1], modes, ',');
			add_desktop_modes(modes);
			continue;
		}
		if (!tokens[0].compare("server-socket")) {
			serversocket = tokens[1];
			continue;
		}
		if (!tokens[0].compare("client-socket")) {
			clientsocket = tokens[1];
			continue;
		}
		if (!tokens[0].compare("startup-script")) {
			startupscript = tokens[1];
			continue;
		}
		if (!tokens[0].compare("shutdown-script")) {
			shutdownscript = tokens[1];
			continue;
		}
		if (!tokens[0].compare("window-font")) {
			windowfont = tokens[1];
			continue;
		}
		if (!tokens[0].compare("menu-font")) {
			menufont = tokens[1];
			continue;
		}
		if (!tokens[0].compare("terminal")) {
			terminal = tokens[1];
			continue;
		}
		if (!tokens[0].compare("app-menu-title")) {
			appmenu = tokens[1];
			continue;
		}
		if (!tokens[0].compare("window-menu-title")) {
			windowmenu = tokens[1];
			continue;
		}
		if (!tokens[0].compare("desktop-menu-title")) {
			desktopmenu = tokens[1];
			continue;
		}
		if (!tokens[0].compare("desktop")) {
			if (tokens.size() < 3) continue;
			int index = std::strtol(tokens[1].c_str(), NULL, 10) - 1;
			if ((index < 0) || (index >= ndesktops)) continue;
			desktop_defs[index].name = tokens[2];
			if (tokens.size() < 4) continue;

			desktop_defs[index].mode = "Stacked";
			desktop_defs[index].split = 0.55;

			if ((!tokens[3].compare("VTiled")) || (!tokens[3].compare("HTiled"))
				|| (!tokens[3].compare("Monocle")))
					desktop_defs[index].mode = tokens[3];
			if (tokens.size() < 5) continue;

			float split = std::strtof(tokens[4].c_str(), NULL);
			if (split < 0.1) split = 0.1;
			if (split > 0.9) split = 0.9;
			desktop_defs[index].split = split;
		}
		if (!tokens[0].compare("color")) {
			if (tokens.size() < 3) continue;
			if (!tokens[1].compare("window-border-active"))
				colordefs[Color::WindowBorderActive] = tokens[2];
			else if (!tokens[1].compare("window-border-inactive"))
				colordefs[Color::WindowBorderInactive] = tokens[2];
			else if (!tokens[1].compare("window-border-urgent"))
				colordefs[Color::WindowBorderUrgent] = tokens[2];
			else if (!tokens[1].compare("menu-background"))
				colordefs[Color::MenuBackground] = tokens[2];
			else if (!tokens[1].compare("menu-border"))
				colordefs[Color::MenuBorder] = tokens[2];
			else if (!tokens[1].compare("menu-text"))
				colordefs[Color::MenuText] = tokens[2];
			else if (!tokens[1].compare("menu-text-selected"))
				colordefs[Color::MenuTextSelected] = tokens[2];
			else if (!tokens[1].compare("menu-highlight"))
				colordefs[Color::MenuHighlight] = tokens[2];
			else if (!tokens[1].compare("menu-title"))
				colordefs[Color::MenuTitle] = tokens[2];
			else if (!tokens[1].compare("menu-title-background"))
				colordefs[Color::MenuTitleBackground] = tokens[2];
			continue;
		}
		if (!tokens[0].compare("unbind-key")) {
			if (!tokens[1].compare("all")) {
				keybindings.clear();
			} else {
				BindingDef bdef(tokens[1]);
				Binding kb(bdef, EventType::Key);
				remove_keybinding(kb);
			}
			continue;
		}
		if (!tokens[0].compare("unbind-mouse")) {
			BindingDef bdef(tokens[1]);
			Binding mb(bdef, EventType::Button);
			remove_mousebinding(mb);
			continue;
		}
		if (!tokens[0].compare("menu-start")) {
			add_menu(tokens[1], configfile);
			continue;
		}

		if (tokens.size() < 3) continue;
		if (!tokens[0].compare("bind-key")) {
			BindingDef bdef;
			if (!tokens[2].compare("exec")) {
				if (tokens.size() < 4) continue;
				bdef = BindingDef(tokens[1], tokens[2], tokens[3]);
			} else
				bdef = BindingDef(tokens[1], tokens[2]);
			Binding kb(bdef, EventType::Key);
			if (kb.valid)
				add_keybinding(kb);
			continue;
		}
		if (!tokens[0].compare("bind-mouse")) {
			BindingDef bdef(tokens[1], tokens[2]);
			Binding mb(bdef, EventType::Button);
			if (mb.valid)
				add_mousebinding(mb);
			continue;
		}
		if (!tokens[0].compare("default-desktop")) {
			std::string rname, rclass;
			get_name_class(tokens[1], rname, rclass);
			int index = std::strtol(tokens[2].c_str(), NULL, 10) - 1;
			if ((index < 0) || (index >= ndesktops)) continue;
			add_default_desktop(rname, rclass, index);
			continue;
		}
		if (!tokens[0].compare("window-state")) {
			std::string rname, rclass;
			get_name_class(tokens[1], rname, rclass);
			std::vector<std::string> states;
			split_string(tokens[2], states, ',');
			add_window_states(rname, rclass, states);
			continue;
		}
	} // end-while

	return;		
}

bool conf::get_line(std::ifstream &configfile, std::string &line)
{
	bool valid = false;
	std::string nextline;

	line = "";
	while (std::getline(configfile, nextline)) {
		valid = true;
		line += nextline;
		if (!line.empty() && line.back() == '\\') {
			line.pop_back();
			continue;
		}
		break;
	}
	return valid;
}

int conf::get_tokens(std::string &line, std::vector<std::string> &tokens)
{
	tokens.clear();
	if (line[0] == '#' || line.empty()) return 0;
	std::istringstream iss(line);
	std::string out;
	std::vector<std::string> words;
	while (iss >> std::quoted(out))
		words.push_back(out);

	// Drops everything from first word starting with a '#'
	for (auto word : words) {
		if (!word.compare("#")) break;
		tokens.push_back(word);
	}
	return tokens.size();
}

int conf::split_string(std::string &str, std::vector<std::string> &tokens, char separator)
{
	std::istringstream iss(str);
	std::string out;
	while (std::getline(iss, out, separator))
		tokens.push_back(out);
	return tokens.size();
}

void conf::get_name_class(std::string &s, std::string &rname, std::string &rclass)
{
	int pos = s.find(":");
	if (pos != s.npos) {
		rname = s.substr(0, pos);
		rclass = s.substr(pos+1);
	} else {
		rname = s;
		rclass = "";
	}
}

void conf::add_keybinding(Binding &kb)
{
	auto isCombo = [kb](Binding b) { return ((kb.modmask == b.modmask) 
			&& (kb.keysym == b.keysym) && (kb.context == b.context)); };
	auto it = std::find_if(keybindings.begin(), keybindings.end(), isCombo);
	if (it != keybindings.end()) keybindings.erase(it);
	keybindings.push_back(kb);
}

void conf::remove_keybinding(Binding &kb)
{
	auto isCombo = [kb](Binding b) { return ((kb.modmask == b.modmask) 
			&& (kb.keysym == b.keysym) && (kb.context == b.context)); };
	auto it = std::find_if(keybindings.begin(), keybindings.end(), isCombo);
	if (it != keybindings.end()) keybindings.erase(it);
}

void conf::add_mousebinding(Binding &mb)
{
	auto isCombo = [mb](Binding b) { return ((mb.modmask == b.modmask) 
			&& (mb.button == b.button) && (mb.context == b.context)); };
	auto it = std::find_if(mousebindings.begin(), mousebindings.end(), isCombo);
	if (it != mousebindings.end()) mousebindings.erase(it);
	mousebindings.push_back(mb);
}

void conf::remove_mousebinding(Binding &mb)
{
	auto isCombo = [mb](Binding b) { return ((mb.modmask == b.modmask) 
			&& (mb.button == b.button) && (mb.context == b.context)); };
	auto it = std::find_if(mousebindings.begin(), mousebindings.end(), isCombo);
	if (it != mousebindings.end()) mousebindings.erase(it);
}

void conf::add_desktop_modes(std::vector<std::string> &modes)
{
	long index = 0;
	desktop_modes.clear();
	for (std::string &mode : modes) {
		if (!mode.compare("Stacked")) 
			desktop_modes.push_back(DesktopMode(index++, "Stacked", "F")); 
		else if (!mode.compare("Monocle"))
			desktop_modes.push_back(DesktopMode(index++, "Monocle", "M")); 
		else if (!mode.compare("VTiled"))
			desktop_modes.push_back(DesktopMode(index++, "VTiled", "V")); 
		else if (!mode.compare("HTiled"))
			desktop_modes.push_back(DesktopMode(index++, "HTiled", "H")); 
	}
	if (desktop_modes.empty()) 
		desktop_modes.push_back(DesktopMode(0, "Stacked", "F")); 
}

void conf::add_window_states(std::string &rname, std::string &rclass,
				std::vector<std::string> &states)
{
	long statemask = 0;
	for (std::string &state : states) {
		if (!state.compare("docked")) statemask |= State::Docked;
		if (!state.compare("frozen")) statemask |= State::Frozen;
		if (!state.compare("ignored")) statemask |= State::Ignored;
		if (!state.compare("noborder")) statemask |= State::NoBorder;
		if (!state.compare("noresize")) statemask |= State::NoResize;
		if (!state.compare("notile")) statemask |= State::NoTile;
		if (!state.compare("sticky")) statemask |= State::Sticky;
	}

	auto isResource = [rname,rclass](DefaultStates def)
		{ return (!rname.compare(def.resname) && !rclass.compare(def.resclass)); };
	auto it = std::find_if(defstateslist.begin(), defstateslist.end(), isResource);
	if (it != defstateslist.end())
		it->states |= statemask;
	else
		defstateslist.push_back(DefaultStates(rname, rclass, statemask));
}

void conf::add_default_desktop(std::string &rname, std::string &rclass, int index)
{
	auto isDefaultDesktop = [index,rname,rclass](DefaultDesktop d)
		{ return ((index == d.index) && !rname.compare(d.resname)
			&& !rclass.compare(d.resclass)); };
	auto it = std::find_if(defdesktoplist.begin(), defdesktoplist.end(), isDefaultDesktop);
	if (it != defdesktoplist.end()) defdesktoplist.erase(it);
	defdesktoplist.push_back(DefaultDesktop(rname, rclass, index));
}

void conf::add_menu(std::string &label, std::ifstream &configfile)
{
	MenuDef 		 menudef(label, MenuType::Command);
	std::vector<std::string> tokens;
	std::string 		 line;

	while (get_line(configfile, line)) {
		if (!get_tokens(line, tokens)) continue;
		if (!tokens[0].compare("menu-end")) break;
		if (tokens[0].compare("menu-item")) continue;
		if ((tokens.size() == 3) && (!tokens[2].compare("restart") ||
			!tokens[2].compare("quit"))) {
			MenuItem item(tokens[1], tokens[2]);
			menudef.items.push_back(item);
			continue;
		}
		if ((tokens.size() >= 4) && (!tokens[2].compare("exec") ||
			!tokens[2].compare("restart") || !tokens[2].compare("menu"))) {
			MenuItem item(tokens[1], tokens[2], tokens[3]);
			menudef.items.push_back(item);
		}
	}
	menulist.push_back(menudef);
}
