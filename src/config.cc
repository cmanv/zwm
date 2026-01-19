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

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "wmfunc.h"
#include "binding.h"
#include "xclient.h"
#include "menu.h"
#include "version.h"
#include "config.h"

namespace conf {
	std::vector<DesktopDef> desktop_defs = {
		{ "one", "default", 0.5 },
		{ "two", "default", 0.5 },
		{ "three", "default", 0.5 },
		{ "four", "default", 0.5 },
		{ "five", "default", 0.5 },
		{ "six", "default", 0.5 },
		{ "seven", "default", 0.5 },
		{ "eigth", "default", 0.5 },
		{ "nine", "default", 0.5 },
		{ "ten", "default", 0.5 },
	};

	std::vector<DesktopMode> desktop_modes = {
		{ "Stacked", Mode::Stacked, 0, 0 },
		{ "Monocle", Mode::Monocle, 0, 0 },
		{ "VTiled",  Mode::VTiled, 0, 0 },
		{ "HTiled",  Mode::HTiled, 0, 0 },
	};

	std::vector<BindingDef>	keybinding_defs = {
		{ "CM-1",	"desktop-switch-1" },
		{ "CM-2",	"desktop-switch-2" },
		{ "CM-3",	"desktop-switch-3" },
		{ "CM-4",	"desktop-switch-4" },
		{ "CM-5",	"desktop-switch-5" },
		{ "CM-6",	"desktop-switch-6" },
		{ "CM-7",	"desktop-switch-7" },
		{ "CM-8",	"desktop-switch-8" },
		{ "CM-9",	"desktop-switch-9" },
		{ "CM-0",	"desktop-switch-10" },
		{ "CM-Right",	"desktop-switch-next" },
		{ "CM-Left",	"desktop-switch-prev" },
		{ "M-1",	"desktop-mode-1" },
		{ "M-2",	"desktop-mode-2" },
		{ "M-3",	"desktop-mode-3" },
		{ "M-4",	"desktop-mode-4" },
		{ "M-Up",	"desktop-mode-next" },
		{ "M-Down",	"desktop-mode-prev" },
		{ "M-Tab",	"desktop-window-focus-next" },
		{ "SM-Tab",	"desktop-window-focus-prev" },
		{ "M-greater",	"desktop-window-master-incr" },
		{ "M-less",	"desktop-window-master-decr" },
		{ "SM-Right",	"desktop-window-rotate-next" },
		{ "SM-Left",	"desktop-window-rotate-prev" },
		{ "M-Right",	"desktop-window-swap-next" },
		{ "M-Left",	"desktop-window-swap-prev" },
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
		{ "M-h",	"window-move-left" },
		{ "M-l",	"window-move-right" },
		{ "M-j",	"window-move-up" },
		{ "M-k",	"window-move-down" },
		{ "SM-h",	"window-resize-left" },
		{ "SM-l",	"window-resize-right" },
		{ "SM-j",	"window-resize-up" },
		{ "SM-k",	"window-resize-down" },
		{ "CM-h",	"window-snap-left" },
		{ "CM-l",	"window-snap-right" },
		{ "CM-j",	"window-snap-up" },
		{ "CM-k",	"window-snap-down" },
		{ "C-Return",	"terminal" },
		{ "CM-r",	"restart" },
		{ "CM-q",	"quit" },
	};

	std::vector<BindingDef> mousebinding_defs = {
		{ "1",		"menu-client" },
		{ "2",		"menu-desktop" },
		{ "3",		"menu-launcher" },
		{ "M-1",	"window-move" },
		{ "M-3",	"window-resize" },
		{ "M-4",	"window-lower" },
		{ "M-5",	"window-raise" },
	};

	std::string	default_theme = "";
	std::string	user_config = "";
	std::string	wmname = "ZWM";
	std::string 	menufont = "Mono:size=12";
	std::string	menu_client_label = "X Clients";
	std::string	menu_desktop_label = "Active desktops";
	std::string	menu_launcher_label = "Launchers";
	std::string	command_socket = "";
	std::string	message_socket = "";
	std::string	terminal = "xterm";
	std::string	startupscript = "";
	std::string	shutdownscript = "";
	std::string 	install_prefix(INSTALL_PREFIX);

	std::vector<std::string> lightcolordefs;
	std::vector<std::string> darkcolordefs;

	int			debug = 0;
	const int		ndesktops = desktop_defs.size();
	int			menu_border = 2;
	int			tiled_border = 2;
	int			stacked_border = 4;
	int			moveamount = 10;
	int			snapdist = 9;

	std::vector<Binding>		keybindings;
	std::vector<Binding>		mousebindings;
	std::vector<DefaultDesktop>	defdesktoplist;
	std::vector<DefaultStates>	defstateslist;
	std::vector<MenuDef>		menulist;

	void read_config();
	void read_bindings();
	void read_menus();

	bool get_line(std::ifstream &, std::string &);
	int  get_tokens(std::string &, std::vector<std::string> &);
	int  split_string(std::string &, std::vector<std::string> &, char);
	void parse_grid_mode(std::string &, long &, long &);
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

	lightcolordefs.resize(Color::NumColors);
	lightcolordefs[Color::WindowBorderActive] 	= "tan";
	lightcolordefs[Color::WindowBorderInactive] 	= "SlateGray4";
	lightcolordefs[Color::WindowBorderUrgent] 	= "orange";
	lightcolordefs[Color::MenuBackground] 		= "gray90";
	lightcolordefs[Color::MenuBorder] 		= "SlateGray3";
	lightcolordefs[Color::MenuHighlight] 		= "SlateGray2";
	lightcolordefs[Color::MenuItemText] 		= "black";
	lightcolordefs[Color::MenuItemTextSelected] 	= "black";
	lightcolordefs[Color::MenuTitle] 		= "black";
	lightcolordefs[Color::MenuTitleBackground] 	= "SlateGray3";

	darkcolordefs.resize(Color::NumColors);
	darkcolordefs[Color::WindowBorderActive] 	= "ForestGreen";
	darkcolordefs[Color::WindowBorderInactive] 	= "DarkGreySlate";
	darkcolordefs[Color::WindowBorderUrgent] 	= "DarkOrange";
	darkcolordefs[Color::MenuBackground] 		= "grey20";
	darkcolordefs[Color::MenuBorder] 		= "SkyBlue4";
	darkcolordefs[Color::MenuHighlight] 		= "SteelBlue4";
	darkcolordefs[Color::MenuItemText] 		= "grey88";
	darkcolordefs[Color::MenuItemTextSelected] 	= "WhiteSmoke";
	darkcolordefs[Color::MenuTitle] 		= "WhiteSmoke";
	darkcolordefs[Color::MenuTitleBackground] 	= "SkyBlue4";

	if (!std::getenv("HOME")) {
		std::cerr << "HOME is not defined in the environment!\n";
		return;
	}

	// Define and create path to socket
	std::string socket_path;
	if (std::getenv("XDG_CACHE_HOME")) {
		socket_path = std::getenv("XDG_CACHE_HOME");
	} else {
		socket_path = std::getenv("HOME") + std::string("/.cache");
	}
	socket_path = socket_path + "/" + APP_NAME;
	std::filesystem::path sockdir(socket_path);
	if (!std::filesystem::exists(sockdir)) {
		create_directory(sockdir);
	}
	command_socket = socket_path + "/socket";

	// Define and create path to config files
	std::string config_path;
	if (std::getenv("XDG_CONFIG_HOME")) {
		config_path = std::getenv("XDG_CONFIG_HOME");
	} else {
		config_path = std::getenv("HOME") + std::string("/.config");
	}
	config_path = config_path + "/" + APP_NAME;
	std::filesystem::path confdir(config_path);
	if (!std::filesystem::exists(confdir)) {
		create_directory(confdir);
	}

	if (user_config.empty()) {
		// Use standard config file location
		user_config = config_path + "/config";

		// Copy example config file if one does not exist at standard location
		if (!std::filesystem::exists(user_config)) {
			std::string install_path = install_prefix + "/share/doc/"
						+ APP_NAME;
			std::filesystem::path default_config(install_path + "/config");
			try {
				std::filesystem::copy_file(default_config, user_config);
			} catch (std::filesystem::filesystem_error& e) {
				std::cerr << e.what() << '\n';
			}
		}
	}
	read_config();
	menulist.push_back(MenuDef(menu_client_label, MenuType::Client));
	menulist.push_back(MenuDef(menu_desktop_label, MenuType::Desktop));

	if (!default_theme.empty()) return;
	default_theme = "light";
	if (std::getenv("THEME_STATE_FILE")) {
		std::string line;
		std::ifstream theme_file(std::getenv("THEME_STATE_FILE"));
		if (theme_file.is_open()) {
			if (std::getline(theme_file, line)) {
				if (!line.compare("dark")) default_theme = "dark";
			}
			theme_file.close();
		}
	}

	return;
}

void conf::read_config()
{
	std::ifstream config_file(user_config);
	if (!config_file.is_open()) {
		std::cerr << "Could not open [" << user_config << "] for reading.\n";
		return;
	}

	std::string line;
	std::vector<std::string> tokens;
	while (get_line(config_file, line)) {
		if (!get_tokens(line, tokens)) continue;

		// Rest need at least 2 tokens
		if (tokens.size() < 2)
			continue;

		if (!tokens[0].compare("debug-level")) {
			debug = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("desktop-modes")) {
			std::vector<std::string> modes;
			split_string(tokens[1], modes, ',');
			add_desktop_modes(modes);
			continue;
		}
		if (message_socket.empty() && !tokens[0].compare("message-socket")) {
			message_socket = tokens[1];
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
		if (!tokens[0].compare("window-tiled-border")) {
			tiled_border = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("window-stacked-border")) {
			stacked_border = std::strtol(tokens[1].c_str(), NULL, 10);
			continue;
		}
		if (!tokens[0].compare("terminal")) {
			terminal = tokens[1];
			continue;
		}
		if (!tokens[0].compare("menu-font")) {
			menufont = tokens[1];
			continue;
		}
		if (!tokens[0].compare("menu-client-label")) {
			menu_client_label = tokens[1];
			continue;
		}
		if (!tokens[0].compare("menu-desktop-label")) {
			menu_desktop_label = tokens[1];
			continue;
		}
		if (!tokens[0].compare("menu-launcher-label")) {
			menu_launcher_label = tokens[1];
			continue;
		}
		if (!tokens[0].compare("desktop-defaults")) {
			if (tokens.size() < 3) continue;
			int index = std::strtol(tokens[1].c_str(), NULL, 10) - 1;
			if ((index < 0) || (index >= ndesktops)) continue;
			desktop_defs[index].name = tokens[2];
			if (tokens.size() < 4) continue;
			if ((!tokens[3].compare("Stacked"))
				|| (!tokens[3].compare("Monocle"))
				|| (!tokens[3].compare("HTiled"))
				|| (!tokens[3].compare("VTiled")))
					desktop_defs[index].mode = tokens[3];
			else {
				long rows, cols;
				parse_grid_mode(tokens[3], rows, cols);
				if ((rows >= 1) && (rows <= 9)
					&& (cols >= 1) && (cols <= 9))
					desktop_defs[index].mode = tokens[3];
			}
			if (tokens.size() < 5) continue;

			float split = std::strtof(tokens[4].c_str(), NULL);
			if (split < 0.1) split = 0.1;
			if (split > 0.9) split = 0.9;
			desktop_defs[index].master_split = split;
		}
		if (!tokens[0].compare("color")) {
			if (tokens.size() < 3) continue;
			if (!tokens[1].compare("window-border-active")) {
				lightcolordefs[Color::WindowBorderActive] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::WindowBorderActive] = tokens[3];
			} else if (!tokens[1].compare("window-border-inactive")) {
				lightcolordefs[Color::WindowBorderInactive] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::WindowBorderInactive] = tokens[3];
			} else if (!tokens[1].compare("window-border-urgent")) {
				lightcolordefs[Color::MenuBackground] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuBackground] = tokens[3];
			} else if (!tokens[1].compare("menu-background")) {
				lightcolordefs[Color::WindowBorderUrgent] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::WindowBorderUrgent] = tokens[3];
			} else if (!tokens[1].compare("menu-border")) {
				lightcolordefs[Color::MenuBorder] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuBorder] = tokens[3];
			} else if (!tokens[1].compare("menu-item-text")) {
				lightcolordefs[Color::MenuItemText] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuItemText] = tokens[3];
			} else if (!tokens[1].compare("menu-item-text-selected")) {
				lightcolordefs[Color::MenuItemTextSelected] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuItemTextSelected] = tokens[3];
			} else if (!tokens[1].compare("menu-highlight")) {
				lightcolordefs[Color::MenuHighlight] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuHighlight] = tokens[3];
			} else if (!tokens[1].compare("menu-title")) {
				lightcolordefs[Color::MenuTitle] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuTitle] = tokens[3];
			} else if (!tokens[1].compare("menu-title-background")) {
				lightcolordefs[Color::MenuTitleBackground] = tokens[2];
				if (tokens.size() > 3)
					darkcolordefs[Color::MenuTitleBackground] = tokens[3];
			}
			continue;
		}
		if (!tokens[0].compare("menu-start")) {
			add_menu(tokens[1], config_file);
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
			if (!tokens[1].compare("all")) {
				mousebindings.clear();
			} else {
				BindingDef bdef(tokens[1]);
				Binding mb(bdef, EventType::Button);
				remove_mousebinding(mb);
			}
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
			BindingDef bdef;
			if (!tokens[2].compare("exec")) {
				if (tokens.size() < 4) continue;
				bdef = BindingDef(tokens[1], tokens[2], tokens[3]);
			} else
				bdef = BindingDef(tokens[1], tokens[2]);
			Binding mb(bdef, EventType::Button);
			if (mb.valid)
				add_mousebinding(mb);
			continue;
		}
		if (!tokens[0].compare("app-default-desktop")) {
			std::string rname, rclass;
			get_name_class(tokens[1], rname, rclass);
			int index = std::strtol(tokens[2].c_str(), NULL, 10) - 1;
			if ((index < 0) || (index >= ndesktops)) continue;
			add_default_desktop(rname, rclass, index);
			continue;
		}
		if (!tokens[0].compare("app-default-state")) {
			std::string rname, rclass;
			get_name_class(tokens[1], rname, rclass);
			std::vector<std::string> states;
			split_string(tokens[2], states, ',');
			add_window_states(rname, rclass, states);
			continue;
		}
	}
	config_file.close();
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
			&& (kb.keysym == b.keysym)); };
	auto it = std::find_if(keybindings.begin(), keybindings.end(), isCombo);
	if (it != keybindings.end()) keybindings.erase(it);
	keybindings.push_back(kb);
}

void conf::remove_keybinding(Binding &kb)
{
	auto isCombo = [kb](Binding b) { return ((kb.modmask == b.modmask)
			&& (kb.keysym == b.keysym)); };
	auto it = std::find_if(keybindings.begin(), keybindings.end(), isCombo);
	if (it != keybindings.end()) keybindings.erase(it);
}

void conf::add_mousebinding(Binding &mb)
{
	auto isCombo = [mb](Binding b) { return ((mb.modmask == b.modmask)
			&& (mb.button == b.button)); };
	auto it = std::find_if(mousebindings.begin(), mousebindings.end(), isCombo);
	if (it != mousebindings.end()) mousebindings.erase(it);
	mousebindings.push_back(mb);
}

void conf::remove_mousebinding(Binding &mb)
{
	auto isCombo = [mb](Binding b) { return ((mb.modmask == b.modmask)
			&& (mb.button == b.button)); };
	auto it = std::find_if(mousebindings.begin(), mousebindings.end(), isCombo);
	if (it != mousebindings.end()) mousebindings.erase(it);
}

void conf::add_desktop_modes(std::vector<std::string> &modes)
{
	long index = 0;
	desktop_modes.clear();
	for (std::string &mode : modes) {
		if (!mode.compare("Stacked"))
			desktop_modes.push_back(
				DesktopMode("Stacked", Mode::Stacked, 0, 0));
		else if (!mode.compare("Monocle"))
			desktop_modes.push_back(
				DesktopMode("Monocle", Mode::Monocle, 0, 0));
		else if (!mode.compare("VTiled"))
			desktop_modes.push_back(
				DesktopMode("VTiled", Mode::VTiled, 0, 0));
		else if (!mode.compare("HTiled"))
			desktop_modes.push_back(
				DesktopMode("HTiled", Mode::HTiled, 0, 0));
		else {
			long rows, cols;
			parse_grid_mode(mode, rows, cols);
			if ((rows < 1) || (rows > 9)) continue;
			if ((cols < 1) || (cols > 9)) continue;
			desktop_modes.push_back(
				DesktopMode(mode, Mode::Grid, rows, cols));
		}
	}
	if (desktop_modes.empty())
		desktop_modes.push_back(
				DesktopMode("Stacked", Mode::Stacked, 0, 0));
}

// Parse grid mode rows x columns
void conf::parse_grid_mode(std::string &mode, long &rows, long &cols)
{
	rows = 0;
	cols = 0;
	std::vector<std::string> values;
	split_string(mode, values, 'x');
	if (values.size() != 2) return;
	rows = std::strtol(values[0].c_str(), NULL, 10);
	cols = std::strtol(values[1].c_str(), NULL, 10);
}

void conf::add_window_states(std::string &rname, std::string &rclass,
				std::vector<std::string> &states)
{
	long statemask = 0;
	for (std::string &state : states) {
		if (!state.compare("docked")) statemask |= State::Docked;
		if (!state.compare("float")) statemask |= State::NoTile;
		if (!state.compare("frozen")) statemask |= State::Frozen;
		if (!state.compare("ignored")) statemask |= State::Ignored;
		if (!state.compare("noborder")) statemask |= State::NoBorder;
		if (!state.compare("noresize")) statemask |= State::NoResize;
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
	MenuDef 		 menudef(label, MenuType::Launcher);
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
			!tokens[2].compare("restart") ||
			!tokens[2].compare("menu"))) {
			MenuItem item(tokens[1], tokens[2], tokens[3]);
			menudef.items.push_back(item);
		}
	}
	menulist.push_back(menudef);
}
