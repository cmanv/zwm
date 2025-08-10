// zwm - a dynamic tiling/stacking window manager for X11
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

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include "misc.h"
#include "config.h"
#include "winmgr.h"
#include "wmhints.h"

namespace wmh {
	std::vector<Atom>	 hints;
}

void wmh::setup()
{
	std::vector<const char *> defs(NUM_WMHINTS);
	defs[WM_STATE] 			= "WM_STATE";
	defs[WM_DELETE_WINDOW] 		= "WM_DELETE_WINDOW";
	defs[WM_TAKE_FOCUS] 		= "WM_TAKE_FOCUS";
	defs[WM_PROTOCOLS] 		= "WM_PROTOCOLS";
	defs[WM_CHANGE_STATE]		= "WM_CHANGE_STATE";
	defs[_MOTIF_WM_HINTS] 		= "_MOTIF_WM_HINTS";
	defs[UTF8_STRING] 		= "UTF8_STRING";

	hints.resize(defs.size());
	XInternAtoms(wm::display, (char **)defs.data(), defs.size(), False,
			(Atom *)hints.data());
}

long wmh::get_wm_state(Window window)
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

void wmh::set_wm_state(Window window, long wstate)
{
	long	 data[] = { wstate, None };

	XChangeProperty(wm::display, window, hints[WM_STATE], hints[WM_STATE], 32,
			PropModeReplace, (unsigned char *)data, 2);
}

void wmh::send_client_message(Window win, Atom proto, Time ts)
{
	XClientMessageEvent	 cm;

	memset(&cm, 0, sizeof(cm));
	cm.type = ClientMessage;
	cm.window = win;
	cm.message_type = hints[WM_PROTOCOLS];
	cm.format = 32;
	cm.data.l[0] = proto;
	cm.data.l[1] = ts;

	XSendEvent(wm::display, win, False, NoEventMask, (XEvent *)&cm);
}

int wmh::get_text_property(Window window, Atom atom, std::vector<char>& text)
{
	XTextProperty	 prop;
	char		**textlist;
	int		 nitems = 0, len;

	XGetTextProperty(wm::display, window, &prop, atom);
	if (!prop.nitems) {
		XFree(prop.value);
		return 0;
	}

	if (Xutf8TextPropertyToTextList(wm::display, &prop, &textlist, &nitems) == Success) {
		if ((nitems == 1) && (*textlist)) {
			len = strlen(*textlist) + 1;
			text.resize(len);
			strlcpy((char *)text.data(), *textlist, len);
		} else if ((nitems > 1) && (*textlist)) {
			XTextProperty	prop2;
			if (Xutf8TextListToTextProperty(wm::display, textlist, nitems,
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

void *wmh::get_window_property(Window w, Atom atom, Atom req_type, long length,
				unsigned long *nitems)
{
	Atom		 actualtype;
	int		 actualformat;
	unsigned long	 bytes_extra;
	unsigned char	*prop;

	if (XGetWindowProperty(wm::display, w, atom, 0L, length, False, req_type,
		&actualtype, &actualformat, nitems, &bytes_extra, &prop) == Success)
	{
		if (actualtype == req_type)
			return (void *)prop;
		XFree(prop);
	}
	return NULL;
}

namespace ewmh {
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

	std::vector<Atom>	hints;
}

void ewmh::setup()
{
	std::vector<const char *> defs(NUM_EWMHINTS);
	defs[_NET_SUPPORTED] 			= "_NET_SUPPORTED";
	defs[_NET_SUPPORTING_WM_CHECK] 		= "_NET_SUPPORTING_WM_CHECK";
	defs[_NET_ACTIVE_WINDOW] 		= "_NET_ACTIVE_WINDOW";
	defs[_NET_CLIENT_LIST] 			= "_NET_CLIENT_LIST";
	defs[_NET_CLIENT_LIST_STACKING] 	= "_NET_CLIENT_LIST_STACKING";
	defs[_NET_NUMBER_OF_DESKTOPS] 		= "_NET_NUMBER_OF_DESKTOPS";
	defs[_NET_CURRENT_DESKTOP] 		= "_NET_CURRENT_DESKTOP";
	defs[_NET_DESKTOP_VIEWPORT] 		= "_NET_DESKTOP_VIEWPORT";
	defs[_NET_DESKTOP_GEOMETRY] 		= "_NET_DESKTOP_GEOMETRY";
	defs[_NET_VIRTUAL_ROOTS] 		= "_NET_VIRTUAL_ROOTS";
	defs[_NET_SHOWING_DESKTOP] 		= "_NET_SHOWING_DESKTOP";
	defs[_NET_DESKTOP_NAMES] 		= "_NET_DESKTOP_NAMES";
	defs[_NET_WORKAREA] 			= "_NET_WORKAREA";
	defs[_NET_WM_NAME] 			= "_NET_WM_NAME";
	defs[_NET_WM_DESKTOP] 			= "_NET_WM_DESKTOP";
	defs[_NET_CLOSE_WINDOW] 		= "_NET_CLOSE_WINDOW";
	defs[_NET_WM_WINDOW_TYPE] 		= "_NET_WM_WINDOW_TYPE";
	defs[_NET_WM_WINDOW_TYPE_DIALOG] 	= "_NET_WM_WINDOW_TYPE_DIALOG";
	defs[_NET_WM_WINDOW_TYPE_DOCK] 		= "_NET_WM_WINDOW_TYPE_DOCK";
	defs[_NET_WM_WINDOW_TYPE_SPLASH] 	= "_NET_WM_WINDOW_TYPE_SPLASH";
	defs[_NET_WM_WINDOW_TYPE_TOOLBAR] 	= "_NET_WM_WINDOW_TYPE_TOOLBAR";
	defs[_NET_WM_WINDOW_TYPE_UTILITY] 	= "_NET_WM_WINDOW_TYPE_UTILITY";
	defs[_NET_WM_STATE] 			= "_NET_WM_STATE";
	defs[_NET_WM_STATE_STICKY] 		= "_NET_WM_STATE_STICKY";
	defs[_NET_WM_STATE_MAXIMIZED_VERT] 	= "_NET_WM_STATE_MAXIMIZED_VERT";
	defs[_NET_WM_STATE_MAXIMIZED_HORZ] 	= "_NET_WM_STATE_MAXIMIZED_HORZ";
	defs[_NET_WM_STATE_HIDDEN] 		= "_NET_WM_STATE_HIDDEN";
	defs[_NET_WM_STATE_FULLSCREEN] 		= "_NET_WM_STATE_FULLSCREEN";
	defs[_NET_WM_STATE_DEMANDS_ATTENTION]	= "_NET_WM_STATE_DEMANDS_ATTENTION";
	defs[_NET_WM_STATE_SKIP_PAGER] 		= "_NET_WM_STATE_SKIP_PAGER";
	defs[_NET_WM_STATE_SKIP_TASKBAR] 	= "_NET_WM_STATE_SKIP_TASKBAR";

	hints.resize(defs.size());
	XInternAtoms(wm::display, (char **)defs.data(), defs.size(), False,
			(Atom *)hints.data());
}

// EWMH functions
void ewmh::set_net_supported(Window rootwin)
{
	XChangeProperty(wm::display, rootwin, hints[_NET_SUPPORTED], XA_ATOM, 32,
			PropModeReplace, (unsigned char *)hints.data(), hints.size());
}

void ewmh::set_net_supported_wm_check(Window rootwin, std::string &name)
{
	Window w = XCreateSimpleWindow(wm::display, rootwin, -1, -1, 1, 1, 0, 0, 0);
	XChangeProperty(wm::display, rootwin, hints[_NET_SUPPORTING_WM_CHECK],
		XA_WINDOW, 32, PropModeReplace, (unsigned char *)&w, 1);
	XChangeProperty(wm::display, w, hints[_NET_SUPPORTING_WM_CHECK],
		XA_WINDOW, 32, PropModeReplace, (unsigned char *)&w, 1);
	XChangeProperty(wm::display, w, hints[_NET_WM_NAME],
		wmh::hints[UTF8_STRING], 8, PropModeReplace,
		(unsigned char *)name.c_str(), name.length());
}

void ewmh::set_net_desktop_geometry(Window rootwin, Geometry &view)
{
	long	geometry[2] = { view.w, view.h };

	XChangeProperty(wm::display, rootwin, hints[_NET_DESKTOP_GEOMETRY], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)geometry , 2);
}

void ewmh::set_net_desktop_viewport(Window rootwin)
{
	long	viewport[2] = {0, 0};

	XChangeProperty(wm::display, rootwin, hints[_NET_DESKTOP_VIEWPORT], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)viewport, 2);
}

void ewmh::set_net_workarea(Window rootwin, int ndesktops, Geometry &work)
{
	std::vector<unsigned long> workarea(ndesktops*4);
	for (int i = 0; i < ndesktops; i++) {
		workarea[4*i + 0] = work.x;
		workarea[4*i + 1] = work.y;
		workarea[4*i + 2] = work.w;
		workarea[4*i + 3] = work.h;
	}
	XChangeProperty(wm::display, rootwin, hints[_NET_WORKAREA], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)workarea.data(), ndesktops * 4);
}

void ewmh::set_net_client_list(Window rootwin, std::vector<Window> &winlist)
{
	int nwins = winlist.size();
	if (nwins == 0) return;

	XChangeProperty(wm::display, rootwin, hints[_NET_CLIENT_LIST], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)winlist.data(), nwins);
}

void ewmh::set_net_client_list_stacking(Window rootwin, std::vector<Window> &winlist)
{
	int nwins = winlist.size();
	if (nwins == 0) return;

	XChangeProperty(wm::display, rootwin, hints[_NET_CLIENT_LIST_STACKING], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)winlist.data(), nwins);
}

void ewmh::set_net_active_window(Window rootwin, Window active)
{
	XChangeProperty(wm::display, rootwin, hints[_NET_ACTIVE_WINDOW], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)&active, 1);
}

void ewmh::set_net_number_of_desktops(Window rootwin, int ndesks)
{
	XChangeProperty(wm::display, rootwin, hints[_NET_NUMBER_OF_DESKTOPS], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&ndesks, 1);
}

int ewmh::get_net_current_desktop(Window window, long *num)
{
	unsigned long	 n;
	long 		*prop;

	prop = (long *)wmh::get_window_property(window, hints[_NET_CURRENT_DESKTOP], XA_CARDINAL, 1L, &n);
	if (prop) {
		*num = *prop;
		XFree(prop);
	}
	return n;
}

void ewmh::set_net_current_desktop(Window rootwin, int active)
{
	long	 num = active;
	XChangeProperty(wm::display, rootwin, hints[_NET_CURRENT_DESKTOP], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&num, 1);
}

void ewmh::unset_net_showing_desktop(Window rootwin)
{
	long	 zero = 0;

	// showing desktop mode is not supported.
	XChangeProperty(wm::display, rootwin, hints[_NET_SHOWING_DESKTOP], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&zero, 1);
}

void ewmh::delete_net_virtual_roots(Window rootwin)
{
	// Virtual roots is not supported
	XDeleteProperty(wm::display, rootwin, hints[_NET_VIRTUAL_ROOTS]);
}

void ewmh::set_net_desktop_names(Window rootwin, std::vector<std::string> &names)
{
	unsigned long	 n = 9;
	int 		 i = 0, ndesks = 0;
	unsigned char	*prop;

	// Let desktop names be overwritten if _NET_DESKTOP_NAMES is set.
	prop = (unsigned char *)wmh::get_window_property(rootwin, hints[_NET_DESKTOP_NAMES],
						wmh::hints[UTF8_STRING], 0xffffff, &n);
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

	XChangeProperty(wm::display, rootwin, hints[_NET_DESKTOP_NAMES],
			wmh::hints[UTF8_STRING], 8, PropModeReplace,
			(unsigned char *)namelist.data(), len);
}

int ewmh::get_net_wm_desktop(Window window, long *num)
{
	unsigned long 	 n;
	long		*prop;

	prop = (long *)wmh::get_window_property(window, hints[_NET_WM_DESKTOP],
					XA_CARDINAL, 1L, &n);
	if (prop) {
		*num = *prop;
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

void ewmh::set_net_wm_desktop(Window window, int desktop)
{
	long num = 0xffffffff;

	if (desktop >=0 ) num = desktop;

	XChangeProperty(wm::display, window, hints[_NET_WM_DESKTOP], XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&num, 1);
}

int ewmh::get_net_wm_window_type(Window window, std::vector<Atom> &atoms)
{
	unsigned long 	 n;
	Atom 		*prop;

	prop = (Atom *)wmh::get_window_property(window, hints[_NET_WM_WINDOW_TYPE],
						XA_ATOM, 64L, &n);
	if (prop) {
		atoms.resize(n);
		memcpy(atoms.data(), (void *)prop, sizeof(Atom) * n);
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

int ewmh::get_net_wm_state_atoms(Window window, std::vector<Atom> &atoms)
{
	unsigned long	 n;
	Atom 		*prop;

	prop = (Atom *)wmh::get_window_property(window, hints[_NET_WM_STATE],
						XA_ATOM, 1024L, &n);
	if (prop) {
		atoms.resize(n);
		memcpy(atoms.data(), (void *)prop, sizeof(Atom) * n);
		XFree((char *)prop);
		return 1;
	}
	return 0;
}

long ewmh::get_net_wm_states(Window window, long initial)
{
	long states = initial;
	std::vector<Atom> atoms;
	get_net_wm_state_atoms(window, atoms);
	for (Atom atom : atoms)
		for (StateMap &sm : statemaps)
			if (atom == hints[sm.atom]) {
				states |= sm.state;
				break;
			}
	return states;
}

void ewmh::set_net_wm_states(Window window, long states)
{
	std::vector<Atom> current_atoms;
	get_net_wm_state_atoms(window, current_atoms);

	std::vector<Atom> atoms;
	for (Atom atom : current_atoms) {
		bool found = false;
		for (StateMap &sm : statemaps) {
			if (atom == hints[sm.atom]) {
				found = true;
				break;
			}
		}
		if (!found)
			atoms.push_back(atom);
	}

	for (StateMap &sm : statemaps)
		if (states & sm.state) atoms.push_back(hints[sm.atom]);

	if (atoms.size()) {
		XChangeProperty(wm::display, window,hints[_NET_WM_STATE], XA_ATOM, 32,
			PropModeReplace, (unsigned char *)atoms.data(), atoms.size());
	}
	else {
		XDeleteProperty(wm::display, window, hints[_NET_WM_STATE]);
	}
}

