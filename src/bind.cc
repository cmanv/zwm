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

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "timer.h"
#include "config.h"
#include "wmcore.h"
#include "wmfunc.h"
#include "bind.h"

std::vector<ModKeyDef> Bind::modkey_defs = {
	{ 'S',  ShiftMask },
	{ 'C',  ControlMask },
	{ 'M',  Mod1Mask },
	{ '4',  Mod4Mask },
	{ '5',  Mod5Mask }
};

Bind::Bind( BindDef& binddef, long eventtype)
{
	valid = false;
	modmask = 0;

	keycombo = binddef.keycombo;
	std::string symbol;

	size_t pos = keycombo.find('-');
	if (pos != std::string::npos) {
		for (unsigned int i=0; i<pos; i++) {
			bool modkey = false;
			for (ModKeyDef &def : modkey_defs) {
				if (keycombo[i] == def.ch) {
					modmask |= def.mask;
					modkey = true;
					break;
				}
			}
			if (!modkey) {
				std::cerr << __func__ << ": Modkey (" << keycombo
					<< ") is not valid!\n";
				return;
			}
		}
		symbol = keycombo.substr(pos+1);
	} else
		symbol = keycombo;

	if (eventtype == EventType::Key) {
		keysym = XStringToKeysym(symbol.c_str());
		if (keysym == NoSymbol) {
			std::cerr << __func__ << ": Keysym (" << symbol.c_str()
				<< ") was not found!\n";
			return;
		}
	} else {
		button = std::strtol(symbol.c_str(), NULL, 10);
		if ((button <1) || (button>5)) {
			std::cerr << __func__ << ": Mouse button (" << button
				<< ") is not valid!\n";
			return;
		}
	}

	for (wmfunc::FuncDef &funcdef : wmfunc::funcdefs) {
		if (!funcdef.namefunc.compare(binddef.namefunc)) {
			function = binddef.namefunc;
			context = funcdef.context;
			valid = true;

			switch (context) {
			case Context::Root:
				fscreen = funcdef.fscreen;
				param = funcdef.param;
				break;
			case Context::Window:
				fclient = funcdef.fclient;
				param = funcdef.param;
				break;
			case Context::FuncCall:
				fcall = funcdef.fcall;
				param = funcdef.param;
				break;
			case Context::Launcher:
				flaunch = funcdef.flaunch;
				path = binddef.path;
				break;
			default:
				valid = false;
			}
			break;
		}
	}
	if (!valid) {
		std::cerr << timer::gettime() << " [Bind::" << __func__ << "] function ("
			<< binddef.namefunc << ") is not defined!\n";
	}
	else if (conf::debug) {
		std::cout << timer::gettime() << " [Bind::" << __func__ << "] define {"
			<< keycombo << "} -> " << function << "(" << path << ")\n";
	}
}
