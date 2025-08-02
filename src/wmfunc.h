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

#ifndef _WMFUNC_H_
#define _WMFUNC_H_
#include <vector>
#include <string>
#include "definitions.h"

class XClient;
class XScreen;

namespace wmfunc {

	struct FuncDef {
		std::string 		namefunc;
		Context			context;
		union {
			void		(*fcall)(long);
			void		(*fexec)(std::string&);
			void		(*fclient)(XClient *, long);
			void		(*fscreen)(XScreen *, long);
			void		(*froot)(XScreen *);
		};
		long			param;

		FuncDef(const char *n, void (*f)(XScreen *), Context c)
			:namefunc(n), froot(f), context(c) {}
		FuncDef(const char *n, void (*f)(long))
			:namefunc(n), fcall(f), param(0) { context = Context::Function; }
		FuncDef(const char *n, void (*f)(long), long v)
			:namefunc(n), fcall(f), param(v) { context = Context::Function; }
		FuncDef(const char *n, void (*f)(XClient *, long))
			:namefunc(n), fclient(f), param(0) { context = Context::Window; }
		FuncDef(const char *n, void (*f)(XClient *, long), long v)
			:namefunc(n), fclient(f), param(v) { context = Context::Window; }
		FuncDef(const char *n, void (*f)(XScreen *, long))
			:namefunc(n), fscreen(f), param(0) { context = Context::Root; }
		FuncDef(const char *n, void (*f)(XScreen *, long), long v)
			:namefunc(n), fscreen(f), param(v) { context = Context::Root; }
		FuncDef(const char *n, void (*f)(std::string&))
			:namefunc(n), fexec(f) { context = Context::Command; }
	};

	extern std::vector<FuncDef> funcdefs;

	void window_resize(XClient *, long);
	void window_move(XClient *, long);
	void window_resize(XClient *, long);
	void window_snap(XClient *, long);
	void window_close(XClient *, long);
	void window_lower(XClient *, long);
	void window_raise(XClient *, long);
	void window_hide(XClient *, long);
	void window_state(XClient *, long);
	void window_to_desktop(XClient *, long);
	void window_menu_label(XClient *, long);

	void desktop_select(XScreen *, long);
	void desktop_last(XScreen *, long);
	void desktop_hide(XScreen *, long);
	void desktop_close(XScreen *, long);
	void desktop_master(XScreen *, long);
	void desktop_mode_stacked(XScreen *, long);
	void desktop_mode_monocle(XScreen *, long);
	void desktop_mode_htiled(XScreen *, long);
	void desktop_mode_vtiled(XScreen *, long);
	void desktop_rotate_mode(XScreen *, long);
	void desktop_rotate_tiles(XScreen *, long);
	void desktop_cycle(XScreen *, long);
	void desktop_window_cycle(XScreen *, long);

	void menu_launcher(XScreen *);
	void menu_client(XScreen *);
	void menu_desktop(XScreen *);

	void exec_term(long);
	void set_wm_status(long);
	void exec_cmd(std::string&);
}
#endif // _WMFUNC_H_
