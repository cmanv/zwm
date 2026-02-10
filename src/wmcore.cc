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

#include <sys/event.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "timer.h"
#include "process.h"
#include "socket.h"
#include "config.h"
#include "xscreen.h"
#include "xevents.h"
#include "wmfunc.h"
#include "wmhints.h"
#include "wmcore.h"

namespace wm {
	Display				*display;
	std::string			 displayname = "";
	Time				 last_event_time = CurrentTime;
	volatile sig_atomic_t	 	 status;

	int				 xrandr;
	int				 xrandr_event_base;

	std::vector<XScreen*>		 screenlist;

	std::vector<std::string>	 restart_argstr;
	std::vector<char*>		 restart_argv;

	std::vector<Cursor> 		 cursors(Pointer::NumShapes);

	const std::vector<unsigned int>  ignore_mods = {
		0,
		LockMask,
		Mod2Mask,
		Mod2Mask | LockMask
	};

	static void	wm_startup(void);
	static void	wm_shutdown(void);
	static void 	process_message(void);
	static int 	start_error_handler(Display *, XErrorEvent *);
	static int	error_handler(Display *, XErrorEvent *);
}

void wm::run()
{
	if (conf::message_socket.length()) {
		socket_out::init(conf::message_socket);
	}

	if (conf::startupscript.length())
		process::exec(conf::startupscript);
	wm_startup();

	int kq = kqueue();
	if (kq == -1) {
		std::cerr << " [wm:: " << __func__ << "] kqueue "
				<< std::strerror(errno) << std::endl;
		exit(1);
	}

	int n = 0;
	struct kevent watch[2];
	long xfd = ConnectionNumber(display);
	if (xfd == -1L) {
		std::cerr << " [wm::" << __func__ << "] bad X connection number\n";
		exit(1);
	}
	EV_SET(&watch[n++], xfd, EVFILT_READ, EV_ADD, 0, 0, 0);

	long sfd = socket_in::init(conf::command_socket);
	if (sfd == -1L) {
		std::cerr << " [wm::" << __func__ << "] error creating socket "
			<< conf::command_socket << std::endl;
		exit(1);
	}
	EV_SET(&watch[n++], sfd, EVFILT_READ, EV_ADD, 0, 0, 0);

	if (kevent(kq, watch, n, NULL, 0, NULL) == -1) {
		std::cerr << " [wm::" << __func__ << "] kevent(setup) "
			<< std::strerror(errno) << std::endl;
		exit(1);
	}

	// Main event loop
	status = IsRunning;
	struct kevent events[2];
	while (status == IsRunning) {
		int nev = kevent(kq, NULL, 0, events, n, NULL);
		if ((nev == -1) && (errno != EINTR)) {
			std::cerr << " [wm::" << __func__ << "] kevent "
				<< std::strerror(errno) << std::endl;
		}
		for (int i=0; i<nev; i++) {
			if ((long)events[i].ident == xfd)
				XEvents::process();
			else if ((long)events[i].ident == sfd) {
				process_message();
			}
		}
	}
	wm_shutdown();

	closefrom(3);
	if (status == IsRestarting) {
		setsid();
		execvp((char *)restart_argv[0], (char **)restart_argv.data());
		std::cerr << "[wm::" << __func__ << "]'" << restart_argv[0]
			<< "' failed to start.\n";
	}

	if (conf::shutdownscript.length())
		process::exec(conf::shutdownscript);
}

// Process message received on the listening socket
static void wm::process_message()
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [wm::" << __func__ << "]\n";
	}
	std::istringstream iss(socket_in::get_message());

	int id;
	std::string screenid;
	if (!std::getline(iss, screenid, ':'))
		return;
	try {
		id = std::stoi(screenid);
	} catch(...) {
		return;
	}

	// Look for the screen
	XScreen *screen = NULL;
	for (XScreen *s : screenlist) {
		if (id == s->get_screenid()) {
			screen = s;
			break;
		}
	}
	if (!screen) return;

	std::string wmfunction;
	if (!std::getline(iss, wmfunction, '='))
		return;

	long param = 0;
	std::string wmparam;
	if (std::getline(iss, wmparam, ';')) {
		param = std::stol(wmparam);
	}

	// Look if the wm function is defined and execute if found
	for (wmfunc::FuncDef &funcdef : wmfunc::funcdefs) {
		// Only screen functions will be performed
		if ((funcdef.context == Context::Root) &&
			!wmfunction.compare(funcdef.namefunc)) {
			if (funcdef.param == wmfunc::free_param) {
				(*funcdef.fscreen)(screen, param);
			} else {
				(*funcdef.fscreen)(screen, funcdef.param);
			}
			XFlush(display);
			break;
		}
	}
}

static void wm::wm_startup()
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [wm::" << __func__ << "] Open X display..\n";
	}

	display = XOpenDisplay(displayname.c_str());
	if (!display) {
		std::cerr << timer::gettime() << " [wm::" << __func__
			<< "] Unable to open display " << XDisplayName(displayname.c_str()) << '\n';
		exit(1);
	}

	XSetErrorHandler(wm::start_error_handler);
	XSelectInput(display, DefaultRootWindow(display), SubstructureRedirectMask);
	XSync(display, False);
	XSetErrorHandler(wm::error_handler);

	int n;
	xrandr = XRRQueryExtension(display, &xrandr_event_base, &n);

	cursors[Pointer::ShapeNormal] = XCreateFontCursor(display, XC_left_ptr);
	cursors[Pointer::ShapeMove] = XCreateFontCursor(display, XC_fleur);
	cursors[Pointer::ShapeNorth] = XCreateFontCursor(display, XC_top_side);
	cursors[Pointer::ShapeEast] = XCreateFontCursor(display, XC_right_side);
	cursors[Pointer::ShapeSouth] = XCreateFontCursor(display, XC_bottom_side);
	cursors[Pointer::ShapeWest] = XCreateFontCursor(display, XC_left_side);
	cursors[Pointer::ShapeNE] = XCreateFontCursor(display, XC_top_right_corner);
	cursors[Pointer::ShapeSE] = XCreateFontCursor(display, XC_bottom_right_corner);
	cursors[Pointer::ShapeSW] = XCreateFontCursor(display, XC_bottom_left_corner);
	cursors[Pointer::ShapeNW] = XCreateFontCursor(display, XC_top_left_corner);

	wmh::setup();
	ewmh::setup();

	for (int i = 0; i < ScreenCount(display); i++)
		screenlist.push_back(new XScreen(i));

	XSync(display, False);
}

static void wm::wm_shutdown()
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [wm::" << __func__
				<< "] Window manager shutdown..\n";
	}

	for (XScreen *screen : screenlist)
		delete screen;

	for (Cursor cursor : cursors)
		XFreeCursor(display, cursor);

	XUngrabPointer(display, CurrentTime);
	XUngrabKeyboard(display, CurrentTime);

	XSync(display, False);
	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(display);

	socket_out::clear();
}

static int  wm::start_error_handler(Display *, XErrorEvent *)
{
	std::cerr << "root window unavailable - perhaps another wm is running?\n";
	exit(1);
}

static int wm::error_handler(Display *display, XErrorEvent *e)
{
	if (conf::debug < 2) return 0;
	char msg[100], req[100];
	XGetErrorText(display, e->error_code, msg, sizeof(msg));
	std::string number = std::to_string(e->request_code);
	XGetErrorDatabaseText(display, "XRequest", number.c_str(),
	    			"<unknown>", req, sizeof(req));

	std::cerr << timer::gettime() << " [wm::" << __func__ << "]:(" << req << ") (0x"
			<< std::hex << (unsigned int)e->resourceid
			<< ") "  << msg << '\n';
	return 0;
}

void wm::set_param_restart(int argc, char **argv)
{
	// Store the command line arguments in restart_argstr
	restart_argstr.resize(argc);
	for (int i=0; i<argc; i++)
		restart_argstr[i] = argv[i];

	// Store access pointers in restart_argv
	restart_argv.resize(argc+1);
	for (int i=0; i<argc; i++)
		restart_argv[i] = (char *)restart_argstr[i].c_str();
	restart_argv[argc] = NULL;
}

void wm::set_param_restart(std::string &cmd)
{
	std::string word;
	restart_argstr.clear();

	if (conf::debug) {
		std::cout << timer::gettime() << " [wm::" << __func__ << "] cmd = " << cmd << std::endl;
	}

	// Split the command into a array of space or quote delimited strings
	std::istringstream iss(cmd);
	while (iss >> std::quoted(word))
		restart_argstr.push_back(word);
	int argc = restart_argstr.size();

	// Store access pointers in restart_argv
	restart_argv.clear();
	restart_argv.resize(argc+1);
	for (int i=0; i<argc; i++)
		restart_argv[i] = (char *)restart_argstr[i].c_str();
	restart_argv[argc] = NULL;
}
