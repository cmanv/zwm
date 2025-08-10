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

#include <X11/Xatom.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "timer.h"
#include "socket.h"
#include "binding.h"
#include "config.h"
#include "wmhints.h"
#include "winmgr.h"
#include "xscreen.h"
#include "xclient.h"

const long XClient::MouseMask	= ButtonReleaseMask|PointerMotionMask;

XClient::XClient(Window w, XScreen *s, bool existing): m_window(w), m_screen(s)
{
	XWindowAttributes	 wattr;

	m_removed = false;
	m_ignore_unmap = false;

	if (conf::debug) {
		std::cout << timer::gettime() << " [XClient::" << __func__
			<< "] Create Client window 0x" << std::hex << m_window << std::endl;
	}
	// For existing clients, reparent will send an unmap request which should be ignored.
	if (existing)
		m_ignore_unmap = true;

	m_rootwin = m_screen->get_window();
	m_border_w = conf::stacked_border;
	m_parent = None;
	m_states = 0;
	m_initial_state = 0;

	// Disable processing of X requests
	XGrabServer(wm::display);

	// Get window informations
	XGetWindowAttributes(wm::display, m_window, &wattr);
	m_geom.x = wattr.x;
	m_geom.y = wattr.y;
	m_geom.w = wattr.width;
	m_geom.h = wattr.height;
	m_colormap = wattr.colormap;
	m_border_orig = wattr.border_width;

	get_net_wm_name();		// Get window name
	get_net_wm_window_type();	// Get window type
	get_wm_hints();			// Get input, urgency and initial status hints
	get_class_hint();		// Get class and name hint
	get_wm_protocols();		// Get wm defined protocols.
	get_wm_normal_hints();		// Get hints from the window geometry
	get_transient();		// Set transient windows to ignore
	get_motif_hints();		// Check if window wants no border

	apply_user_states();	// Apply user configured states

	if (has_state(State::NoBorder))
		m_border_w = 0;

	// Pointer position
	m_ptr = m_geom.get_center(Coordinates::Window);

	// New window starts as hidden until reparent
	if (wattr.map_state != IsViewable) {
		set_initial_placement();
		wmh::set_wm_state(m_window, IconicState);
	}
	m_geom_stack = m_geom;

	// Request some types of event from X server
	XSelectInput(wm::display, m_window, EnterWindowMask|PropertyChangeMask);

	send_configure_event();
	m_states = ewmh::get_net_wm_states(m_window, m_states);

	// Set the desktop index.
	m_deskindex = -1;
	if (!has_state(State::Sticky)) {
		if (!existing) m_deskindex = get_configured_desktop();
		else m_deskindex = get_net_wm_desktop();
		if (m_deskindex == -1) m_deskindex = m_screen->get_active_desktop();
	}
	reparent_window();
	m_screen->assign_client_to_desktop(this, m_deskindex, false);

	// Resume processing of X requests
	XSync(wm::display, False);
	XUngrabServer(wm::display);
}

XClient::~XClient()
{
	if (conf::debug) {
		std::cout << timer::gettime() << " [XClient::" << __func__
			<< "] Destroy Client window 0x" << std::hex << m_window << std::endl;
	}

	// Disable processing of X requests
	XGrabServer(wm::display);
	XUngrabButton(wm::display, AnyButton, AnyModifier, m_parent);

	// If the window manager is shutting down, revert tile to stacked geometry
	if ((wm::status != IsRunning) && has_state(State::Tiled)) {
		clear_states(State::Frozen);
		set_stacked_geom();
	}

	if (m_removed) {
		// The client has been unmapped
		wmh::set_wm_state(m_window, WithdrawnState);
		XDeleteProperty(wm::display, m_window, ewmh::hints[_NET_WM_DESKTOP]);
		XDeleteProperty(wm::display, m_window, ewmh::hints[_NET_WM_STATE]);
	}
	// Reparent the client window to root and destroy the parent
	XReparentWindow(wm::display, m_window, m_rootwin, m_geom.x, m_geom.y);
	XSetWindowBorderWidth(wm::display, m_window, m_border_orig);
	XRemoveFromSaveSet(wm::display, m_window);
	XDestroyWindow(wm::display, m_parent);

	// Resume processing of X requests
	XUngrabServer(wm::display);
	XSync(wm::display, False);
}

void XClient::reparent_window()
{
	XSetWindowAttributes wattr;
	wattr.border_pixel = m_screen->get_pixel(Color::WindowBorderInactive);
	wattr.override_redirect = True;
	wattr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
		| ButtonPressMask | EnterWindowMask;

	m_parent = XCreateWindow(wm::display, m_rootwin, m_geom.x, m_geom.y,
			m_geom.w, m_geom.h, m_border_w,
			DefaultDepth(wm::display, m_screen->get_screenid()),
			CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWBorderPixel|CWEventMask, &wattr);

	XAddToSaveSet(wm::display, m_window);
	XSetWindowBorderWidth(wm::display, m_window, 0);
	XReparentWindow(wm::display, m_window, m_parent, 0, 0);

	// Grab mouse bindings in the parent windoe
	for (Binding& mb : conf::mousebindings) {
		if (mb.context != Context::Window) continue;
		for (auto mod : wm::ignore_mods)
			XGrabButton(wm::display, mb.button, (mb.modmask | mod),
					m_parent, False, ButtonPressMask,
					GrabModeAsync, GrabModeAsync, None, None);
	}

}

void XClient::draw_window_border()
{
	unsigned long	pixel;

	if (has_state(State::Urgent))
		pixel = m_screen->get_pixel(Color::WindowBorderUrgent);
	else if (has_state(State::Active))
		pixel = m_screen->get_pixel(Color::WindowBorderActive);
	else
		pixel = m_screen->get_pixel(Color::WindowBorderInactive);

	XSetWindowBorderWidth(wm::display, m_parent, (unsigned int)m_border_w);
	XSetWindowBorder(wm::display, m_parent, pixel | (0xffu << 24));
}

bool XClient::has_window(Window w)
{
	if (w == None) return false;
	if (w == m_window) return true;
	if (w == m_parent) return true;
	return false;
}

bool XClient::ignore_unmap()
{
	if (m_ignore_unmap) {
		m_ignore_unmap = false;
		return true;
	}
	return false;
}

void XClient::get_net_wm_name()
{
	std::vector<char> text;
	if (!wmh::get_text_property(m_window, ewmh::hints[_NET_WM_NAME], text))
		wmh::get_text_property(m_window, XA_WM_NAME, text);
	m_name = std::string(text.begin(), text.end());
	if (!m_name.empty()) m_name.pop_back();
}

void XClient::update_net_wm_name()
{
	get_net_wm_name();
	update_statusbar_title();
}

void XClient::update_statusbar_title()
{
	if (!socket_out::defined()) return;
	std::string message = "window_active=" + m_name;
	socket_out::send(message);
}

void XClient::get_net_wm_window_type()
{
	std::vector<Atom> atoms;
	ewmh::get_net_wm_window_type(m_window, atoms);
	for (Atom atom : atoms) {
		if (atom == ewmh::hints[_NET_WM_WINDOW_TYPE_DOCK]) {
			set_states(State::Docked);
			break;
		}
		if (atom == ewmh::hints[_NET_WM_WINDOW_TYPE_DIALOG]) {
			set_states(State::NoTile);
			break;
		}
		if (atom == ewmh::hints[_NET_WM_WINDOW_TYPE_SPLASH]) {
			set_states(State::NoTile|State::NoResize);
			break;
		}
		if (atom == ewmh::hints[_NET_WM_WINDOW_TYPE_TOOLBAR]) {
			set_states(State::NoTile);
			break;
		}
		if (atom == ewmh::hints[_NET_WM_WINDOW_TYPE_UTILITY]) {
			set_states(State::NoTile);
			break;
		}
	}
}

void XClient::get_class_hint()
{
	XClassHint	hint;

	if (XGetClassHint(wm::display, m_window, &hint)) {
		if (hint.res_class) {
			m_res_class = hint.res_class;
			XFree(hint.res_class);
		}
		if (hint.res_name) {
			m_res_name = hint.res_name;
			XFree(hint.res_name);
		}
	}
}

long XClient::get_net_wm_desktop()
{
	long	index = -1;
	if (ewmh::get_net_wm_desktop(m_window, &index)) {
		index = std::min(index, (m_screen->get_num_desktops() - 1));
	}
	return index;
}

void XClient::get_wm_hints()
{
	XWMHints	*wmh;

	if ((wmh = XGetWMHints(wm::display, m_window)) != NULL) {
		if ((wmh->flags & InputHint) && (wmh->input))
			set_states(State::Input);
		if ((wmh->flags & XUrgencyHint))
			set_states(State::Urgent);
		if ((wmh->flags & StateHint))
			m_initial_state = wmh->initial_state;
		XFree(wmh);
	}
}

void XClient::get_wm_protocols()
{
	Atom	*protocol;
	int	 i, n;

	if (XGetWMProtocols(wm::display, m_window, &protocol, &n)) {
		for (i = 0; i < n; i++) {
			if (protocol[i] == wmh::hints[WM_DELETE_WINDOW])
				set_states(State::WMDeleteWindow);
			else if (protocol[i] == wmh::hints[WM_TAKE_FOCUS])
				set_states(State::WMTakeFocus);
		}
		XFree(protocol);
	}
}

// Set transient window state to ignored
void XClient::get_transient()
{
	XClient		*tc;
	Window		 trans;

	if (XGetTransientForHint(wm::display, m_window, &trans)) {
		if ((tc = XScreen::find_client(trans)) != NULL) {
			if (tc->has_state(State::Ignored)) {
				set_states(State::NoTile|State::Ignored);
				m_border_w = tc->m_border_w;
			}
		}
	}
}

// Some windows will signal they want no border through Motif hints
void XClient::get_motif_hints()
{
	unsigned long	 n;
	MotifHints	*hints;

	hints = (MotifHints *)wmh::get_window_property(m_window,
						wmh::hints[_MOTIF_WM_HINTS],
						wmh::hints[_MOTIF_WM_HINTS],
						Motif::HintElements, &n);
	if (!hints) return;

	if ((hints->flags & Motif::HintDecorations) &&
	    !(hints->decorations & Motif::DecorAll)) {
		if (!(hints->decorations & Motif::DecorBorder)) {
			set_states(State::NoTile|State::NoBorder);
		}
	}
	XFree(hints);
}

// Calculate initial placement of the window
void XClient::set_initial_placement()
{
	if (m_hints.flags & (USPosition | PPosition)) {
		Geometry view = m_screen->get_view();
		m_geom.set_user_placement(view, m_border_w);
		if (has_state(State::Ignored))
			m_geom.adjust_for_maximized(view, m_border_orig);
	} else {
		Position pos = ptr::get_pos(m_rootwin);
		Geometry area = m_screen->get_area(pos, true);
		m_geom.set_placement(pos, area, m_border_w);
	}
	XMoveResizeWindow(wm::display, m_window, m_geom.x, m_geom.y, m_geom.w, m_geom.h);
}

// Obtain size hints from the client window
void XClient::get_wm_normal_hints()
{
	long		 tmp;
	XSizeHints	 hints;

	if (!XGetWMNormalHints(wm::display, m_window, &hints, &tmp))
		hints.flags = 0;

	m_hints = SizeHints(hints);
}

// Apply user defined state configurations
void XClient::apply_user_states()
{
	for (DefaultStates &def : conf::defstateslist) {
		bool match = true;
		if (!def.resname.empty() && def.resname.compare(m_res_name))
			match = false;
		if (!def.resclass.empty() && def.resclass.compare(m_res_class))
			match = false;
		if (match) set_states(def.states);
	}
}

// Returns the configured desktop for the client if it exists
long XClient::get_configured_desktop()
{
	long index = -1;
	for (DefaultDesktop &defdesk : conf::defdesktoplist) {
		bool match = true;
		if (!defdesk.resclass.empty() && defdesk.resclass.compare(m_res_class))
				match = false;

		if (!defdesk.resname.empty() && defdesk.resname.compare(m_res_name))
				match = false;

		if (match) index = defdesk.index;
	}
	return index;
}

// Process event to Configure the size of the client window
void XClient::configure_window(XConfigureRequestEvent *e)
{
	if (has_state(State::Frozen)) return;

	if (e->value_mask & CWWidth)
		m_geom.w = e->width;
	if (e->value_mask & CWHeight)
		m_geom.h = e->height;

	resize_window();
}

void XClient::send_configure_event()
{
	XConfigureEvent	 xev;

	(void)memset(&xev, 0, sizeof(xev));
	xev.type = ConfigureNotify;
	xev.event = m_window;
	xev.window = m_window;
	xev.x = m_geom.x;
	xev.y = m_geom.y;
	xev.width = m_geom.w;
	xev.height = m_geom.h;
	xev.border_width = 0;
	xev.above = None;
	xev.override_redirect = False;

	XSendEvent(wm::display, m_window, False, StructureNotifyMask, (XEvent *)&xev);
}

void XClient::set_window_active()
{
	if (has_state(State::Hidden) || has_states(State::Docked))
		return;

	XInstallColormap(wm::display, m_colormap);

	if (has_state(State::Input) || !has_state(State::WMTakeFocus))
		XSetInputFocus(wm::display, m_window, RevertToPointerRoot, CurrentTime);

	if (has_state(State::WMTakeFocus))
		wmh::send_client_message(m_window, wmh::hints[WM_TAKE_FOCUS],
						wm::last_event_time);

	XClient *prev_client = m_screen->get_active_client();
	if (prev_client) {
		prev_client->clear_states(State::Active);
		prev_client->draw_window_border();
	}

	set_states(State::Active);
	clear_states(State::Urgent);
	draw_window_border();
	m_screen->raise_client(this);
	ewmh::set_net_active_window(m_rootwin, m_window);
	update_statusbar_title();
}

void XClient::show_window()
{
	clear_states(State::Hidden);
	ewmh::set_net_wm_states(m_window, m_states);
	wmh::set_wm_state(m_window, NormalState);
	XMapWindow(wm::display, m_parent);
	XMapWindow(wm::display, m_window);
	draw_window_border();
}

void XClient::hide_window()
{
	XUnmapWindow(wm::display, m_parent);
	if (has_state(State::Active)) {
		clear_states(State::Active);
		ewmh::set_net_active_window(m_rootwin, None);
	}
	set_states(State::Hidden);
	ewmh::set_net_wm_states(m_window, m_states);
	wmh::set_wm_state(m_window, IconicState);
}

void XClient::close_window()
{
	if (has_state(State::WMDeleteWindow))
		wmh::send_client_message(m_window, wmh::hints[WM_DELETE_WINDOW],
					CurrentTime);
	else
		XKillClient(wm::display, m_window);
}

void XClient::raise_window()
{
	m_screen->raise_client(this);
	XRaiseWindow(wm::display, m_parent);
}

void XClient::lower_window()
{
	XLowerWindow(wm::display, m_parent);
}

void XClient::move_window_with_keyboard(long direction)
{
	if (has_state(State::Frozen))
		return;

	Geometry view = m_screen->get_view();
	m_geom.move(direction, view, m_border_w);

	Position pos = m_geom.get_center(Coordinates::Root);
	Geometry area = m_screen->get_area(pos, true);
	m_geom.snap_to_edge(area);

	move_window();
	move_pointer_inside();
	m_geom_stack = m_geom;

	XSync(wm::display, True);
}

void XClient::move_window_with_pointer()
{
	XEvent		 ev;
	Time		 ltime = 0;
	Geometry	 area;
	Position	 pos;

	if (conf::debug) {
		std::cout << timer::gettime() << " [XClient::" << __func__
			<< "] Move window 0x" << std::hex << m_window << std::endl;
	}
	if (has_state(State::Frozen)) return;

	raise_window();
	move_pointer_inside();

	if (XGrabPointer(wm::display, m_parent, False, MouseMask,
		GrabModeAsync, GrabModeAsync, None, wm::cursors[Pointer::ShapeMove],
		 CurrentTime) != GrabSuccess) return;

	PropWindow propwin(m_screen, m_parent);
	std::string label = std::to_string(m_geom.x) + " . " + std::to_string(m_geom.y);
	propwin.draw(label, m_geom.w/2, m_geom.h/2);

	bool buttonpress = true;
	while (buttonpress) {
		XMaskEvent(wm::display, MouseMask, &ev);
		switch (ev.type) {
		case MotionNotify:
			// not more than 60 times / second
			if ((ev.xmotion.time - ltime) <= (1000 / 60))
				continue;
			ltime = ev.xmotion.time;

			m_geom.x = ev.xmotion.x_root - m_ptr.x - m_border_w;
			m_geom.y = ev.xmotion.y_root - m_ptr.y - m_border_w;

			pos = m_geom.get_center(Coordinates::Root);
			area = m_screen->get_area(pos, true);
			m_geom.snap_to_edge(area);
			move_window();

			label = std::to_string(m_geom.x) + " . "
						+ std::to_string(m_geom.y);
			propwin.draw(label, m_geom.w/2, m_geom.h/2);

			break;
		case ButtonRelease:
			buttonpress = false;
			break;
		}
	}
	if (ltime) move_window();
	XUngrabPointer(wm::display, CurrentTime);
	m_geom_stack = m_geom;
	XSync(wm::display, True);
}

void XClient::move_window()
{
	XMoveWindow(wm::display, m_parent, m_geom.x, m_geom.y);
	send_configure_event();
}

void XClient::resize_window_with_keyboard(long direction)
{
	if (has_state(State::Frozen|State::NoResize)) return;

	m_geom.resize(direction, m_hints, m_border_w);
	resize_window();
	move_pointer_inside();
	m_geom_stack = m_geom;
	XSync(wm::display, True);
}

void XClient::resize_window_with_pointer()
{
	if (has_state(State::Frozen|State::NoResize)) return;

	if (conf::debug>1) {
		std::cout << timer::gettime() << " [XClient::" << __func__ << "]\n";
	}

	raise_window();
	m_ptr = ptr::get_pos(m_parent);

	// Pointer position determines the direction of the resize
	Direction	direction;
	Cursor		cursor;
	int limitleft = m_geom.w/4;
	int limitright = 3 * m_geom.w/4;
	int limittop = m_geom.h/4;
	int limitbottom = 3 * m_geom.h/4;

	// Default to just move the window if not in a border area
	direction = Pointer;
	cursor = wm::cursors[Pointer::ShapeMove];
	if ((m_ptr.x > limitright) && (m_ptr.y > limitbottom)) {
		direction = SouthEast;
		cursor = wm::cursors[Pointer::ShapeSE];
	} else if ((m_ptr.x > limitright) && (m_ptr.y <= limittop)) {
		direction = NorthEast;
		cursor = wm::cursors[Pointer::ShapeNE];
	} else if ((m_ptr.x <= limitleft) && (m_ptr.y > limitbottom)) {
		direction = SouthWest;
		cursor = wm::cursors[Pointer::ShapeSW];
	} else if ((m_ptr.x <= limitleft) && (m_ptr.y <= limittop)) {
		direction = NorthWest;
		cursor = wm::cursors[Pointer::ShapeNW];
	} else if ((m_ptr.x > limitleft) && (m_ptr.x < limitright)
		&& (m_ptr.y < limittop)) {
		direction = North;
		cursor = wm::cursors[Pointer::ShapeNorth];
	} else if ((m_ptr.x > limitleft) && (m_ptr.x < limitright)
		&& (m_ptr.y > limitbottom)) {
		direction = South;
		cursor = wm::cursors[Pointer::ShapeSouth];
	} else if ((m_ptr.y > limittop) && (m_ptr.y < limitbottom)
		&& (m_ptr.x < limitleft)) {
		direction = West;
		cursor = wm::cursors[Pointer::ShapeWest];
	} else if ((m_ptr.y > limittop) && (m_ptr.y < limitbottom)
		&& (m_ptr.x > limitright)) {
		direction = East;
		cursor = wm::cursors[Pointer::ShapeEast];
	}

	if (XGrabPointer(wm::display, m_parent, False, MouseMask, GrabModeAsync,
		GrabModeAsync, None, cursor, CurrentTime) != GrabSuccess) return;

	PropWindow propwin(m_screen, m_parent);
	int width = (m_geom.w - m_hints.basew) / m_hints.incw;
	int height = (m_geom.h - m_hints.baseh) / m_hints.inch;
	std::string label = std::to_string(width) + " x " + std::to_string(height);
	propwin.draw(label, m_geom.w/2, m_geom.h/2);

	XEvent	ev;
	Time	ltime = 0;
	bool	buttonpress = true;
	int	xmax = m_geom.x + m_geom.w;
	int	ymax = m_geom.y + m_geom.h;
	while (buttonpress) {
		XMaskEvent(wm::display, MouseMask, &ev);
		switch (ev.type) {
		case MotionNotify:
			// not more than 60 times / second
			if ((ev.xmotion.time - ltime) <= (1000 / 60))
				continue;
			ltime = ev.xmotion.time;
			switch(direction) {
			case Direction::North:
				m_geom.y = ev.xmotion.y_root;
				m_geom.h = ymax - m_geom.y;
				break;
			case Direction::South:
				m_geom.h = ev.xmotion.y;
				break;
			case Direction::East:
				m_geom.w = ev.xmotion.x;
				break;
			case Direction::West:
				m_geom.x = ev.xmotion.x_root;
				m_geom.w = xmax - m_geom.x;
				break;
			case Direction::NorthEast:
				m_geom.w = ev.xmotion.x;
				m_geom.y = ev.xmotion.y_root;
				m_geom.h = ymax - m_geom.y;
				break;
			case Direction::SouthEast:
				m_geom.w = ev.xmotion.x;
				m_geom.h = ev.xmotion.y;
				break;
			case Direction::SouthWest:
				m_geom.x = ev.xmotion.x_root;
				m_geom.w = xmax - m_geom.x;
				m_geom.h = ev.xmotion.y;
				break;
			case Direction::NorthWest:
				m_geom.x = ev.xmotion.x_root;
				m_geom.y = ev.xmotion.y_root;
				m_geom.w = xmax - m_geom.x;
				m_geom.h = ymax - m_geom.y;
				break;
			default:
				m_geom.x = ev.xmotion.x_root - m_ptr.x - m_border_w;
				m_geom.y = ev.xmotion.y_root - m_ptr.y - m_border_w;
				break;
			}

			m_geom.apply_size_hints(m_hints);
			resize_window();
			m_geom_stack = m_geom;

			width = (m_geom.w - m_hints.basew) / m_hints.incw;
			height = (m_geom.h - m_hints.baseh) / m_hints.inch;
			label = std::to_string(width) + " x " + std::to_string(height);
			propwin.draw(label, m_geom.w/2, m_geom.h/2);
			break;
		case ButtonRelease:
			buttonpress = false;
			break;
		}
	}
	if (ltime) resize_window();
	XUngrabPointer(wm::display, CurrentTime);

	// Make sure the pointer stays within the window.
	move_pointer_inside();
	XSync(wm::display, True);
}

void XClient::resize_window()
{
	XMoveResizeWindow(wm::display, m_parent, m_geom.x, m_geom.y, m_geom.w, m_geom.h);
	XMoveResizeWindow(wm::display, m_window, 0, 0, m_geom.w, m_geom.h);
	draw_window_border();
	send_configure_event();
}

void XClient::snap_window(long direction)
{
	if (has_state(State::Frozen)) return;
	Position pos = m_geom.get_center(Coordinates::Root);
	Geometry area = m_screen->get_area(pos, true);
	m_geom.warp_to_edge(direction, area, m_border_w);
	move_window();
	move_pointer_inside();
}

void XClient::move_pointer_inside()
{
	m_ptr = ptr::get_pos(m_parent);
	m_ptr.move_inside(m_geom);
	ptr::set_pos(m_parent, m_ptr);
}

void XClient::warp_pointer()
{
	ptr::set_pos(m_parent, m_ptr);
}

void XClient::save_pointer()
{
	Position p = ptr::get_pos(m_parent);
	if (m_geom.contains(p, Coordinates::Window)) {
		m_ptr = p;
	} else {
		m_ptr = m_geom.get_center(Coordinates::Window);
	}
}

void XClient::set_stacked_geom()
{
	m_geom = m_geom_stack;
	if (has_state(State::NoBorder))
		m_border_w = 0;
	else
		m_border_w = conf::stacked_border;

	resize_window();
}

void XClient::set_tiled_geom(Geometry &tiled)
{
	m_geom = tiled;
	m_border_w = conf::tiled_border;
	resize_window();
}

void XClient::set_notile()
{
	if (has_state(State::FullScreen))
		remove_fullscreen();

	clear_states(State::Tiled|State::Frozen|State::Maximized);
	set_states(State::NoTile);
	m_geom = m_geom_stack;
	if (has_state(State::NoBorder))
		m_border_w = 0;
	else
		m_border_w = conf::stacked_border;

	resize_window();
}

void XClient::change_states(int action, Atom a, Atom b)
{
	for (const StateMap &sm : ewmh::statemaps) {
		if (a != ewmh::hints[sm.atom] &&
		    b != ewmh::hints[sm.atom])
			continue;
		switch(action) {
		case _NET_WM_STATE_ADD:
			if (!(has_state(sm.state)))
				toggle_state(sm.state);
			break;
		case _NET_WM_STATE_REMOVE:
			if (has_state(sm.state))
				toggle_state(sm.state);
			break;
		case _NET_WM_STATE_TOGGLE:
			toggle_state(sm.state);
			break;
		}
		break;
	}
}

void XClient::toggle_state(long flags)
{
	switch(flags) {
	case State::Urgent:
		if (!has_state(State::Active))
			set_states(State::Urgent);
		break;
	case State::Hidden:
	case State::SkipPager:
	case State::SkipTaskbar:
		m_states ^= flags;
		break;
	case State::Sticky:
		if (has_state(State::Sticky))
			m_screen->assign_client_to_desktop(this,
				 m_screen->get_active_desktop(), true);
		else
			m_screen->assign_client_to_desktop(this, -1, true);
		m_states ^= flags;
		break;
	case State::NoTile:
		if (has_state(State::NoTile)) {
			clear_states(State::NoTile);
			m_screen->add_window_tile_to_desktop(this);
		} else {
			m_screen->remove_window_tile_from_desktop(this);
			set_notile();
		}
		break;
	case State::FullScreen:
		toggle_fullscreen();
		break;
	}
	ewmh::set_net_wm_states(m_window, m_states);
}

void XClient::toggle_fullscreen()
{
	if (has_state(State::Frozen) && !has_state(State::FullScreen|State::Tiled))
		return;

	if (has_state(State::FullScreen))
		remove_fullscreen();
	else {
		Position pos = m_geom.get_center(Coordinates::Root);
		Geometry area = m_screen->get_area(pos, false);
		m_geom_save = m_geom;
		m_border_w = 0;

		m_geom = area;
		set_states(State::FullScreen|State::Frozen);
		raise_window();
	}

	resize_window();
	move_pointer_inside();
}

void XClient::remove_fullscreen()
{
	m_border_w = conf::stacked_border;
	if (has_state(State::NoBorder))
		m_border_w = 0;
	m_geom = m_geom_save;
	if (has_state(State::Tiled)) {
		m_border_w = conf::tiled_border;
	} else {
		clear_states(State::Frozen);
	}
	clear_states(State::FullScreen);
}
