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

#include <sys/event.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "util.h"
#include "config.h"
#include "xscreen.h"
#include "xevents.h"
#include "winmgr.h"

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

	std::vector<StateMap> statemaps = {
		{ _NET_WM_STATE_STICKY,			State::Sticky },
		{ _NET_WM_STATE_MAXIMIZED_VERT, 	State::VMaximized },
		{ _NET_WM_STATE_MAXIMIZED_HORZ, 	State::HMaximized },
		{ _NET_WM_STATE_HIDDEN, 		State::Hidden },
		{ _NET_WM_STATE_FULLSCREEN, 		State::FullScreen },
		{ _NET_WM_STATE_DEMANDS_ATTENTION, 	State::Urgent },
		{ _NET_WM_STATE_SKIP_PAGER, 		State::SkipPager },
		{ _NET_WM_STATE_SKIP_TASKBAR, 		State::SkipTaskbar },
	};

	std::vector<Atom>	 ewmh;
	std::vector<Atom>	 hints;

	static void	x_startup(void);
	static void	x_shutdown(void);
	static void 	process_message(std::string&);
	static void	setup_wmhints(void);
	static void	setup_ewmhnts(void);
	static int 	start_error_handler(Display *, XErrorEvent *);
	static int	error_handler(Display *, XErrorEvent *);
}

static void wm::setup_wmhints()
{
	std::vector<const char *> hint_defs(NUM_WMHINTS);
	hint_defs[WM_STATE] 				= "WM_STATE";
	hint_defs[WM_DELETE_WINDOW] 			= "WM_DELETE_WINDOW";
	hint_defs[WM_TAKE_FOCUS] 			= "WM_TAKE_FOCUS";
	hint_defs[WM_PROTOCOLS] 			= "WM_PROTOCOLS";
	hint_defs[WM_CHANGE_STATE]			= "WM_CHANGE_STATE";
	hint_defs[_MOTIF_WM_HINTS] 			= "_MOTIF_WM_HINTS";
	hint_defs[UTF8_STRING] 				= "UTF8_STRING";

	hints.resize(hint_defs.size());
	XInternAtoms(display, (char **)hint_defs.data(), hint_defs.size(), False,
			(Atom *)hints.data());
}

static void wm::setup_ewmhnts()
{
	std::vector<const char *> ewmh_defs(NUM_EWMHINTS);
	ewmh_defs[_NET_SUPPORTED] 			= "_NET_SUPPORTED";
	ewmh_defs[_NET_SUPPORTING_WM_CHECK] 		= "_NET_SUPPORTING_WM_CHECK";
	ewmh_defs[_NET_ACTIVE_WINDOW] 			= "_NET_ACTIVE_WINDOW";
	ewmh_defs[_NET_CLIENT_LIST] 			= "_NET_CLIENT_LIST";
	ewmh_defs[_NET_CLIENT_LIST_STACKING] 		= "_NET_CLIENT_LIST_STACKING";
	ewmh_defs[_NET_NUMBER_OF_DESKTOPS] 		= "_NET_NUMBER_OF_DESKTOPS";
	ewmh_defs[_NET_CURRENT_DESKTOP] 		= "_NET_CURRENT_DESKTOP";
	ewmh_defs[_NET_DESKTOP_VIEWPORT] 		= "_NET_DESKTOP_VIEWPORT";
	ewmh_defs[_NET_DESKTOP_GEOMETRY] 		= "_NET_DESKTOP_GEOMETRY";
	ewmh_defs[_NET_VIRTUAL_ROOTS] 			= "_NET_VIRTUAL_ROOTS";
	ewmh_defs[_NET_SHOWING_DESKTOP] 		= "_NET_SHOWING_DESKTOP";
	ewmh_defs[_NET_DESKTOP_NAMES] 			= "_NET_DESKTOP_NAMES";
	ewmh_defs[_NET_WORKAREA] 			= "_NET_WORKAREA";
	ewmh_defs[_NET_WM_NAME] 			= "_NET_WM_NAME";
	ewmh_defs[_NET_WM_DESKTOP] 			= "_NET_WM_DESKTOP";
	ewmh_defs[_NET_CLOSE_WINDOW] 			= "_NET_CLOSE_WINDOW";
	ewmh_defs[_NET_WM_WINDOW_TYPE] 			= "_NET_WM_WINDOW_TYPE";
	ewmh_defs[_NET_WM_WINDOW_TYPE_DIALOG] 		= "_NET_WM_WINDOW_TYPE_DIALOG";
	ewmh_defs[_NET_WM_WINDOW_TYPE_DOCK] 		= "_NET_WM_WINDOW_TYPE_DOCK";
	ewmh_defs[_NET_WM_WINDOW_TYPE_SPLASH] 		= "_NET_WM_WINDOW_TYPE_SPLASH";
	ewmh_defs[_NET_WM_WINDOW_TYPE_TOOLBAR] 		= "_NET_WM_WINDOW_TYPE_TOOLBAR";
	ewmh_defs[_NET_WM_WINDOW_TYPE_UTILITY] 		= "_NET_WM_WINDOW_TYPE_UTILITY";
	ewmh_defs[_NET_WM_STATE] 			= "_NET_WM_STATE";
	ewmh_defs[_NET_WM_STATE_STICKY] 		= "_NET_WM_STATE_STICKY";
	ewmh_defs[_NET_WM_STATE_MAXIMIZED_VERT] 	= "_NET_WM_STATE_MAXIMIZED_VERT";
	ewmh_defs[_NET_WM_STATE_MAXIMIZED_HORZ] 	= "_NET_WM_STATE_MAXIMIZED_HORZ";
	ewmh_defs[_NET_WM_STATE_HIDDEN] 		= "_NET_WM_STATE_HIDDEN";
	ewmh_defs[_NET_WM_STATE_FULLSCREEN] 		= "_NET_WM_STATE_FULLSCREEN";
	ewmh_defs[_NET_WM_STATE_DEMANDS_ATTENTION]	= "_NET_WM_STATE_DEMANDS_ATTENTION";
	ewmh_defs[_NET_WM_STATE_SKIP_PAGER] 		= "_NET_WM_STATE_SKIP_PAGER";
	ewmh_defs[_NET_WM_STATE_SKIP_TASKBAR] 		= "_NET_WM_STATE_SKIP_TASKBAR";

	ewmh.resize(ewmh_defs.size());
	XInternAtoms(display, (char **)ewmh_defs.data(), ewmh_defs.size(), False,
			(Atom *)ewmh.data());
}

void wm::run() 
{
	if (conf::clientsocket.length()) {
		util::init_client_socket(conf::clientsocket);
	}

	if (conf::startupscript.length())  
		util::exec_child(conf::startupscript);
	x_startup();

	int kq = kqueue();
	if (kq == -1) {
		std::cerr << " [wm:: " << __func__ << "] kqueue " 
				<< std::strerror(errno) << std::endl;
		exit(1);
	}

	int n = 0;
	struct kevent watch[2];
	int xfd = ConnectionNumber(display);
	if (xfd != -1) 
		EV_SET(&watch[n++], xfd, EVFILT_READ, EV_ADD, 0, 0, 0);

	int sfd = -1;
	if (conf::serversocket.length()) {
		sfd = util::init_server_socket(conf::serversocket);
		if (sfd != -1) 
			EV_SET(&watch[n++], sfd, EVFILT_READ, EV_ADD, 0, 0, 0);
	}
	if (kevent(kq, watch, n, NULL, 0, NULL) == -1) {
		std::cerr << " [wm::" << __func__ << "] kevent(setup) " 
			<< std::strerror(errno) << std::endl;
		exit(1);
	}

	status = IsRunning;
	struct kevent events[2];
	while (status == IsRunning) {
		int nev = kevent(kq, NULL, 0, events, n, NULL);
		if ((nev == -1) && (errno != EINTR)) {
			std::cerr << " [wm::" << __func__ << "] kevent " 
				<< std::strerror(errno) << std::endl;
		}
		for (int i=0; i<nev; i++) {
			if (events[i].ident == xfd)
				XEvents::process();
			else if (events[i].ident == sfd) {
				std::string mesg = "";
				util::get_message(sfd, mesg);
				process_message(mesg);
			}
		}
	}
	x_shutdown();

	closefrom(3);
	if (status == IsRestarting) {
		setsid();
		execvp((char *)restart_argv[0], (char **)restart_argv.data());
		std::cerr << "[wm::" << __func__ << "]'" << restart_argv[0] 
			<< "' failed to start.\n";
	}

	if (conf::shutdownscript.length())
		util::exec_child(conf::shutdownscript);
}

static void wm::process_message(std::string& mesg)
{
	std::istringstream iss(mesg);
	
	int id;
	std::string token;
	if (!std::getline(iss, token, ';')) 
		return;
	try {
		id = std::stoi(token);
	} catch(...) {
		return;
	}

	XScreen *screen = NULL;	
	for (XScreen *s : screenlist) {
		if (id == s->get_screenid()) {
			screen = s;
			break;
		}
	}
	if (!screen) return;

	if (!std::getline(iss, token, ';')) 
		return;
	if (!token.compare("NextDesktop")) {
		screen->cycle_desktops(1);
		XFlush(display);
	} else if (!token.compare("PrevDesktop")) {
		screen->cycle_desktops(-1);
		XFlush(display);
	} else if (!token.compare("NextMode")) {
		screen->rotate_desktop_mode(1);
		XFlush(display);
	} else if (!token.compare("PrevMode")) {
		screen->rotate_desktop_mode(-1);
		XFlush(display);
	}
}

static void wm::x_startup()
{
	if (conf::debug) {
		std::cout << util::gettime() << " [wm::" << __func__ << "] Open X display..\n";
	}

	display = XOpenDisplay(displayname.c_str());
	if (!display) {
		std::cerr << util::gettime() << " [wm::" << __func__ 
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

	setup_wmhints();
	setup_ewmhnts();

	for (int i = 0; i < ScreenCount(display); i++) 
		screenlist.push_back(new XScreen(i));

	for (XScreen *screen : screenlist)
		screen->query_clients();

	XSync(display, False);
}

static void wm::x_shutdown()
{
	if (conf::debug) {
		std::cout << util::gettime() << " [wm::" << __func__ 
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
	
	if (conf::clientsocket.length())
		util::free_client_socket();
}

static int  wm::start_error_handler(Display *display, XErrorEvent *e)
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

	std::cerr << util::gettime() << " [wm::" << __func__ << "]:(" << req << ") (0x" 
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
		std::cout << util::gettime() << " [wm::" << __func__ << "] cmd = " << cmd << std::endl;
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

// EWMH functions
void wm::set_net_supported(Window rootwin)
{
	XChangeProperty(display, rootwin, ewmh[_NET_SUPPORTED], XA_ATOM, 32, 
			PropModeReplace, (unsigned char *)ewmh.data(), ewmh.size());
}

void wm::set_net_supported_wm_check(Window rootwin, std::string &name)
{
	Window w = XCreateSimpleWindow(display, rootwin, -1, -1, 1, 1, 0, 0, 0);
	XChangeProperty(display, rootwin, ewmh[_NET_SUPPORTING_WM_CHECK],
	    XA_WINDOW, 32, PropModeReplace, (unsigned char *)&w, 1);
	XChangeProperty(display, w, ewmh[_NET_SUPPORTING_WM_CHECK],
	    XA_WINDOW, 32, PropModeReplace, (unsigned char *)&w, 1);
	XChangeProperty(display, w, ewmh[_NET_WM_NAME],
	    hints[UTF8_STRING], 8, PropModeReplace,
	    (unsigned char *)name.c_str(), name.length());
}

void wm::set_net_desktop_geometry(Window rootwin, Geometry &view)
{
	long	geometry[2] = { view.w, view.h };

	XChangeProperty(display, rootwin, ewmh[_NET_DESKTOP_GEOMETRY], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)geometry , 2);
}

void wm::set_net_desktop_viewport(Window rootwin)
{
	long	viewport[2] = {0, 0};

	XChangeProperty(display, rootwin, ewmh[_NET_DESKTOP_VIEWPORT], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)viewport, 2);
}

void wm::set_net_workarea(Window rootwin, int ndesktops, Geometry &work)
{
	std::vector<unsigned long> workarea(ndesktops*4);
	for (int i = 0; i < ndesktops; i++) {
		workarea[4*i + 0] = work.x;
		workarea[4*i + 1] = work.y;
		workarea[4*i + 2] = work.w;
		workarea[4*i + 3] = work.h;
	}
	XChangeProperty(display, rootwin, ewmh[_NET_WORKAREA], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)workarea.data(), ndesktops * 4);
}

void wm::set_net_client_list(Window rootwin, std::vector<Window> &winlist)
{
	int nwins = winlist.size();
	if (nwins == 0) return;

	XChangeProperty(display, rootwin, ewmh[_NET_CLIENT_LIST], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)winlist.data(), nwins);
}

void wm::set_net_client_list_stacking(Window rootwin, std::vector<Window> &winlist)
{
	int nwins = winlist.size();
	if (nwins == 0) return;

	XChangeProperty(display, rootwin, ewmh[_NET_CLIENT_LIST_STACKING], XA_WINDOW, 32, 
			PropModeReplace, (unsigned char *)winlist.data(), nwins);
}

void wm::set_net_active_window(Window rootwin, Window active)
{
	XChangeProperty(display, rootwin, ewmh[_NET_ACTIVE_WINDOW], XA_WINDOW, 32, 
			PropModeReplace, (unsigned char *)&active, 1);
}

void wm::set_net_number_of_desktops(Window rootwin, int ndesks)
{
	XChangeProperty(display, rootwin, ewmh[_NET_NUMBER_OF_DESKTOPS], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)&ndesks, 1);
}

int wm::get_net_current_desktop(Window window, long *num)
{
	unsigned long	 n;
	long 		*prop;

	prop = (long *)get_window_property(window, ewmh[_NET_CURRENT_DESKTOP], XA_CARDINAL, 1L, &n);
	if (prop) {
		*num = *prop;	
		XFree(prop);
	}
	return n;
}

void wm::set_net_current_desktop(Window rootwin, int active)
{
	long	 num = active;
	XChangeProperty(display, rootwin, ewmh[_NET_CURRENT_DESKTOP], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)&num, 1);
}

void wm::unset_net_showing_desktop(Window rootwin)
{
	long	 zero = 0;

	// showing desktop mode is not supported.
	XChangeProperty(display, rootwin, ewmh[_NET_SHOWING_DESKTOP], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)&zero, 1);
}

void wm::delete_net_virtual_roots(Window rootwin)
{
	// Virtual roots is not supported
	XDeleteProperty(display, rootwin, ewmh[_NET_VIRTUAL_ROOTS]);
}

void wm::set_net_desktop_names(Window rootwin, std::vector<std::string> &names)
{
	unsigned long	 n = 9;
	int 		 i = 0, ndesks = 0;
	unsigned char	*prop;

	// Let desktop names be overwritten if _NET_DESKTOP_NAMES is set.
	prop = (unsigned char *)get_window_property(rootwin, ewmh[_NET_DESKTOP_NAMES], 
						hints[UTF8_STRING], 0xffffff, &n);
	while (i < n)
		if (prop[i++] == '\0')
			ndesks++;
	i = 0;
	char *p = (char *)prop;
	for (std::string &str : names) {
		if (++i >= ndesks) break;
		str = p;
		p += strlen(p) + 1;
	}
	if (prop)
		XFree(prop);

	size_t len = 0;
	for (std::string &str : names)
		len += str.size() + 1;

	std::vector<unsigned char> namelist(len);
	p = (char *)namelist.data();
	size_t remain = len;
	for (std::string &str : names) {
		size_t slen = str.size() + 1;
		strlcpy(p, str.c_str(), remain);
		remain -= slen;
		p += slen;
	}

	XChangeProperty(display, rootwin, ewmh[_NET_DESKTOP_NAMES], hints[UTF8_STRING],
			8, PropModeReplace, (unsigned char *)namelist.data(), len);
}

long wm::get_wm_state(Window window)
{
	unsigned long	 n;
	long		*prop, state;

	state = -1;
	prop = (long *)get_window_property(window, hints[WM_STATE], hints[WM_STATE], 2L, &n); 
	if (prop) {
		state = *prop;
		XFree(prop);
	}

	return state;
}

void wm::set_wm_state(Window window, long wstate)
{
	long	 data[] = { wstate, None };

	XChangeProperty(display, window, hints[WM_STATE], hints[WM_STATE], 32,
	    		PropModeReplace, (unsigned char *)data, 2);
}

int wm::get_net_wm_desktop(Window window, long *num)
{
	unsigned long 	 n;
	long		*prop;

	prop = (long *)get_window_property(window, ewmh[_NET_WM_DESKTOP], XA_CARDINAL, 1L, &n);
	if (prop) {
		*num = *prop;
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

void wm::set_net_wm_desktop(Window window, int desktop)
{
	long num = 0xffffffff;

	if (desktop >=0 ) num = desktop;

	XChangeProperty(display, window, ewmh[_NET_WM_DESKTOP], XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char *)&num, 1);
}

int wm::get_net_wm_window_type(Window window, std::vector<Atom> &atoms)
{
	unsigned long 	 n;
	Atom 		*prop;

	prop = (Atom *)get_window_property(window, ewmh[_NET_WM_WINDOW_TYPE], XA_ATOM, 64L, &n);
	if (prop) {
		atoms.resize(n);
		memcpy(atoms.data(), (void *)prop, sizeof(Atom) * n);
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

int wm::get_net_wm_state_atoms(Window window, std::vector<Atom> &atoms)
{
	unsigned long	 n;
	Atom 		*prop;

	prop = (Atom *)get_window_property(window, ewmh[_NET_WM_STATE], XA_ATOM, 1024L, &n);
	if (prop) {
		atoms.resize(n);
		memcpy(atoms.data(), (void *)prop, sizeof(Atom) * n);
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

long wm::get_net_wm_states(Window window, long initial)
{
	long states = initial;
	std::vector<Atom> atoms;
	get_net_wm_state_atoms(window, atoms);
	for (Atom atom : atoms)
		for (StateMap &sm : wm::statemaps)
			if (atom == ewmh[sm.atom]) {
				states |= sm.state;
				break;
			}
	return states;
}

void wm::set_net_wm_states(Window window, long states)
{
	std::vector<Atom> old_atoms;
	get_net_wm_state_atoms(window, old_atoms);

	std::vector<Atom> atoms;
	for (Atom atom : old_atoms) {
		bool found = false;
		for (StateMap &sm : wm::statemaps) {
			if (atom == ewmh[sm.atom]) { 
				found = true;	
				break;
			}
		}
		if (!found)
			atoms.push_back(atom);
	}

	for (StateMap &sm : wm::statemaps)
		if (states & sm.state) atoms.push_back(ewmh[sm.atom]);

	if (atoms.size()) {
		XChangeProperty(display, window, ewmh[_NET_WM_STATE], XA_ATOM, 32, 
			PropModeReplace, (unsigned char *)atoms.data(), atoms.size());
	}
	else {
		XDeleteProperty(display, window, ewmh[_NET_WM_STATE]);
	}
}

void *wm::get_window_property(Window w, Atom atom, Atom req_type, long length, 
				unsigned long *nitems)
{
	Atom		 actualtype;
	int		 actualformat;
	unsigned long	 bytes_extra;
	unsigned char	*prop;
	
	if (XGetWindowProperty(display, w, atom, 0L, length, False, req_type,
	    	&actualtype, &actualformat, nitems, &bytes_extra, &prop) == Success)
	{
		if (actualtype == req_type)
			return (void *)prop;
		XFree(prop);
	}
	return NULL;
}

int wm::get_text_property(Window window, Atom atom, std::vector<char>& text) 
{
	XTextProperty	 prop;
	char		**textlist;
	int		 nitems = 0, len;

	XGetTextProperty(display, window, &prop, atom);
	if (!prop.nitems) {
		XFree(prop.value);
		return 0;
	}

	if (Xutf8TextPropertyToTextList(display, &prop, &textlist, &nitems) == Success) {
		if ((nitems == 1) && (*textlist)) {
			len = strlen(*textlist) + 1;
			text.resize(len);
			strlcpy((char *)text.data(), *textlist, len);
		} else if ((nitems > 1) && (*textlist)) {
			XTextProperty	prop2;
			if (Xutf8TextListToTextProperty(display, textlist, nitems,
			    XUTF8StringStyle, &prop2) == Success) {
				len = strlen((char *)prop2.value) + 1;
				text.resize(len);
				strlcpy((char *)text.data(), (char *)prop2.value, len);
				XFree(prop2.value);
			}
		}
		if (*textlist) XFreeStringList(textlist);
	}
	XFree(prop.value);

	return nitems;
}

void wm::send_client_message(Window win, Atom proto, Time ts)
{
	XClientMessageEvent	 cm;

	memset(&cm, 0, sizeof(cm));
	cm.type = ClientMessage;
	cm.window = win;
	cm.message_type = hints[WM_PROTOCOLS];
	cm.format = 32;
	cm.data.l[0] = proto;
	cm.data.l[1] = ts;

	XSendEvent(display, win, False, NoEventMask, (XEvent *)&cm);
}
