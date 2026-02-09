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

#ifndef _BINDING_H_
#define _BINDING_H_
#include <X11/Xlib.h>
#include <string>
#include "enums.h"

class XClient;
class XScreen;

struct ModKeyDef {
	char		ch;
	unsigned int	mask;
	ModKeyDef(char c, int m): ch(c), mask(m) {}
};

struct BindingDef {
	std::string		keycombo;
	std::string		namefunc;
	std::string		path;
	BindingDef() {}
	BindingDef(std::string &k, std::string &f, std::string &p)
		: keycombo(k), namefunc(f), path(p) {}
	BindingDef(std::string &k, std::string &f): keycombo(k), namefunc(f) {}
	BindingDef(const char *k, const char *f): keycombo(k), namefunc(f) {}
	BindingDef(std::string &k): keycombo(k), namefunc("None") {}
};

struct Binding {
	std::string		keycombo;
	std::string		function;
	unsigned int		modmask;
	union {
		KeySym		keysym;
		unsigned int	button;
	};
	Context			context;
	union {
		void		(*fcall)(long);
		void		(*flaunch)(std::string&);
		void		(*fscreen)(XScreen *, long);
		void		(*fclient)(XClient *, long);
		void		(*froot)(XScreen *);
	};
	long			param;
	std::string		path;
	bool			valid;
	static std::vector<ModKeyDef> modkey_defs;
	Binding(BindingDef&, long);
};
#endif // BINDING_H_
