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

#include <X11/Xatom.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "util.h"
#include "binding.h"
#include "config.h"
#include "winmgr.h"
#include "xscreen.h"
#include "xclient.h"

const long XClient::ButtonMask	= ButtonPressMask|ButtonReleaseMask;
const long XClient::MouseMask	= ButtonMask|PointerMotionMask;

XClient::XClient(Window w, XScreen *s, bool query): m_window(w), m_screen(s)
{
	XWindowAttributes	 wattr;

	m_removed = false;
	m_ignore_unmap = false;

	if (conf::debug) {
		std::cout << util::gettime() << " [XClient::" << __func__
			<< "] Create Client window 0x" << std::hex << m_window << std::endl;
	}
	// For existing clienta, reparent will send an unmap request which should be ignored.
	if (query)
		m_ignore_unmap = true;

	m_rootwin = m_screen->get_window();
	m_font = m_screen->get_window_font();
	m_border_w = 1;
	m_header_h = m_font->ascent + m_font->descent;
	m_footer_h = conf::footerheight;
	m_button_w = m_header_h;
	m_handle_w = m_header_h * 1.5;
	m_parent = None;
	m_states = 0;
	m_initial_state = 0;

	// Disable processing of X requests
	XGrabServer(wm::display);

	// Get window informations
	XGetWindowAttributes(wm::display, m_window, &wattr);
	m_geom_child.x = wattr.x;
	m_geom_child.y = wattr.y;
	m_geom_child.w = wattr.width;
	m_geom_child.h = wattr.height;
	m_colormap = wattr.colormap;
	m_old_border = wattr.border_width;

	get_net_wm_name(); 		// Get window name
	get_net_wm_window_type(); 	// Get window type
	get_wm_hints(); 		// Get input, urgency and initial status hints
	get_class_hint(); 		// Get class and name hint
	get_wm_protocols(); 		// Get wm defined protocols.
	get_wm_normal_hints(); 		// Get hints from the window geometry
	get_transient(); 		// Set transient windows to ignore
	get_motif_hints();		// Check if window wants no border

	apply_configured_states();	// Apply user configured states

	// Enable tiling on non-floated windows
	if (!(m_states & State::Floated)) {
		m_states |= State::Tiled|State::Frozen;
	}

	// No border or headers on ignored windows
	if (m_states & State::Ignored) {
		m_states |= State::NoDecor;
		m_border_w = 0;
	}

	if (m_states & State::NoHeader)
		m_header_h = 0;

	if (m_states & State::NoFooter)
		m_footer_h = 0;

	// Pointer position
	m_ptr = m_geom_child.get_center(Coordinates::Window);

	// New window starts as hidden until reparent
	if (wattr.map_state != IsViewable) {
		set_initial_placement();
		wm::set_wm_state(m_window, IconicState);
	}

	// Request some types of event from X server
	XSelectInput(wm::display, m_window, EnterWindowMask|PropertyChangeMask|KeyReleaseMask);

	send_configure_event();
	m_states = wm::get_net_wm_states(m_window, m_states);

	// Set the desktop index.
	m_deskindex = -1;
	if (!(m_states & State::Sticky)) {
		if (!query) m_deskindex = get_configured_desktop();
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
		std::cout << util::gettime() << " [XClient::" << __func__
			<< "] Destroy Client window 0x" << std::hex << m_window << std::endl;
	}

	// Disable processing of X requests
	XGrabServer(wm::display);

	if (m_removed) {
		// The client has been unmapped
		wm::set_wm_state(m_window, WithdrawnState);
		XDeleteProperty(wm::display, m_window, wm::ewmh[_NET_WM_DESKTOP]);
		XDeleteProperty(wm::display, m_window, wm::ewmh[_NET_WM_STATE]);
	}
	// Reparent the client window to root and destroy the parent
	XReparentWindow(wm::display, m_window, m_rootwin, m_geom.x, m_geom.y);
	XSetWindowBorderWidth(wm::display, m_window, m_old_border);
	XRemoveFromSaveSet(wm::display, m_window);
	if (m_header !=  None) {
		XFreeGC(wm::display, m_left_gc);
		XFreeGC(wm::display, m_right_gc);
		XftDrawDestroy(m_title_draw);
		XFreePixmap(wm::display, m_left_pixmap);
		XFreePixmap(wm::display, m_right_pixmap);
		XFreePixmap(wm::display, m_title_pixmap);

		XDestroyWindow(wm::display, m_left_button);
		XDestroyWindow(wm::display, m_title_bar);
		XDestroyWindow(wm::display, m_right_button);
		XDestroyWindow(wm::display, m_header);
	}
	if (m_footer != None) {
		XDestroyWindow(wm::display, m_left_handle);
		XDestroyWindow(wm::display, m_middle_handle);
		XDestroyWindow(wm::display, m_right_handle);
		XDestroyWindow(wm::display, m_footer);
	}
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
	wattr.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|
			ExposureMask|ButtonPressMask|EnterWindowMask;

	m_geom = m_geom_child;
	m_geom_floated = m_geom;
	m_geom.h += (m_header_h + m_footer_h);

	m_parent = XCreateWindow(wm::display, m_rootwin,
			m_geom.x, m_geom.y, m_geom.w, m_geom.h, m_border_w,
			DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWBorderPixel|CWEventMask, &wattr);

	m_header = None;
	if (m_header_h)
		create_header();

	m_footer = None;
	if (m_footer_h)
		create_footer();

	XAddToSaveSet(wm::display, m_window);
	XSetWindowBorderWidth(wm::display, m_window, 0);
	XReparentWindow(wm::display, m_window, m_parent, 0, m_header_h);
	XMapWindow(wm::display, m_parent);
	XMapWindow(wm::display, m_window);
}

void XClient::create_header()
{
	XGCValues gcvalues;
	XSetWindowAttributes wattr;
	wattr.override_redirect = True;
	wattr.background_pixel = m_screen->get_pixel(Color::WindowBackground);
	wattr.event_mask = ExposureMask;

	m_header = XCreateWindow(wm::display, m_parent,
			0, 0, m_geom.w, m_header_h, 0,
			DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWBackPixel|CWEventMask, &wattr);

	wattr.event_mask = ExposureMask|ButtonPressMask;

	// Left button
	m_left_button = XCreateWindow(wm::display, m_header, 0, 0,
			m_button_w, m_header_h, 0,
			DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWEventMask, &wattr);

	m_left_pixmap = XCreatePixmap(wm::display, m_left_button, m_button_w,
			m_header_h, DefaultDepth(wm::display, m_screen->get_screenid()));

	m_left_gc = XCreateGC(wm::display, m_left_pixmap, 0, &gcvalues);
	XSetWindowBackgroundPixmap(wm::display, m_left_button, m_left_pixmap);

	// Title bar
	m_title_bar = XCreateWindow(wm::display, m_header, m_button_w + 2, 0,
			m_geom.w - 2 * m_button_w - 4, m_header_h, 0,
			DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWEventMask, &wattr);

	m_title_width = m_geom.w;
	m_title_pixmap = XCreatePixmap(wm::display, m_title_bar, m_title_width,
			m_header_h, DefaultDepth(wm::display, m_screen->get_screenid()));

	m_title_draw = XftDrawCreate(wm::display, m_title_pixmap, m_screen->get_visual(),
			m_screen->get_colormap());
	XSetWindowBackgroundPixmap(wm::display, m_title_bar, m_title_pixmap);

	// Right button
	m_right_button = XCreateWindow(wm::display, m_header, m_geom.w - m_button_w, 0,
			m_button_w, m_header_h, 0,
			DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
			DefaultVisual(wm::display, m_screen->get_screenid()),
			CWOverrideRedirect|CWEventMask, &wattr);

	m_right_pixmap = XCreatePixmap(wm::display, m_right_button, m_button_w,
			m_header_h, DefaultDepth(wm::display, m_screen->get_screenid()));

	m_right_gc = XCreateGC(wm::display, m_right_pixmap, 0, &gcvalues);
	XSetWindowBackgroundPixmap(wm::display, m_right_button, m_right_pixmap);

	draw_title();
	draw_left_button(false);
	draw_right_button(false);
	XMapWindow(wm::display, m_left_button);
	XMapWindow(wm::display, m_title_bar);
	XMapWindow(wm::display, m_right_button);
	XMapWindow(wm::display, m_header);
}

void XClient::create_footer()
{
	XSetWindowAttributes wattr;
	wattr.override_redirect = True;
	wattr.background_pixel = m_screen->get_pixel(Color::WindowBackground);
	wattr.event_mask = ExposureMask;

	m_footer = XCreateWindow(wm::display, m_parent,
		0, m_geom.h - m_footer_h, m_geom.w, m_footer_h, 0,
		DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
		DefaultVisual(wm::display, m_screen->get_screenid()),
		CWOverrideRedirect|CWBackPixel|CWEventMask, &wattr);

	wattr.background_pixel = m_screen->get_pixel(Color::WindowDecorActive);
	wattr.event_mask = ExposureMask|ButtonPressMask;

	wattr.cursor = wm::cursors[Pointer::ShapeResizeSW];
	m_left_handle = XCreateWindow(wm::display, m_footer,
		0, 0, m_handle_w, m_footer_h, 0,
		DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
		DefaultVisual(wm::display, m_screen->get_screenid()),
		CWOverrideRedirect|CWBackPixel|CWCursor|CWEventMask, &wattr);

	wattr.cursor = wm::cursors[Pointer::ShapeResizeSE];
	m_right_handle = XCreateWindow(wm::display, m_footer,
		m_geom.w - m_handle_w, 0, m_handle_w, m_footer_h, 0,
		DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
		DefaultVisual(wm::display, m_screen->get_screenid()),
		CWOverrideRedirect|CWBackPixel|CWCursor|CWEventMask, &wattr);

	wattr.cursor = wm::cursors[Pointer::ShapeResizeSouth];
	m_middle_handle = XCreateWindow(wm::display, m_footer,
		m_handle_w + 4, 0, m_geom.w - 2 * m_handle_w - 8, m_footer_h, 0,
		DefaultDepth(wm::display, m_screen->get_screenid()), CopyFromParent,
		DefaultVisual(wm::display, m_screen->get_screenid()),
		CWOverrideRedirect|CWBackPixel|CWCursor|CWEventMask, &wattr);

	XMapWindow(wm::display, m_left_handle);
	XMapWindow(wm::display, m_middle_handle);
	XMapWindow(wm::display, m_right_handle);
	XMapWindow(wm::display, m_footer);
}

void XClient::draw_window()
{
	draw_window_border();
	if (m_header_h)
		draw_window_header();
	if (m_footer_h)
		draw_window_footer();
}

void XClient::draw_window_header()
{
	if (m_states & State::NoHeader) return;
	draw_title();
	draw_left_button(false);
	draw_right_button(false);
}

void XClient::draw_title()
{
	XGlyphInfo 	 extents;
	XftColor 	*bgcolor, *textcolor;
	unsigned long	 pixel;

	if (m_states & State::Active) {
		pixel = m_screen->get_pixel(Color::WindowDecorActive);
		bgcolor = m_screen->get_color(Color::WindowDecorActive);
		textcolor = m_screen->get_color(Color::WindowTextActive);
	} else {
		pixel = m_screen->get_pixel(Color::WindowDecorInactive);
		bgcolor = m_screen->get_color(Color::WindowDecorInactive);
		textcolor = m_screen->get_color(Color::WindowTextInactive);
	}

	int len = m_geom.w - 2 * m_header_h - 4;
	XftDrawRect(m_title_draw, bgcolor, 0, 0, len, m_header_h);
	XftTextExtentsUtf8(wm::display, m_font, (XftChar8 *)(m_name.c_str()),
					m_name.size(), &extents);
	int xpos = (len - extents.width) / 2;
	if (xpos < 0) xpos = 4;
	XftDrawStringUtf8(m_title_draw, textcolor, m_font, xpos, m_font->ascent,
	    		(const FcChar8*)m_name.c_str(), m_name.size());
	XClearWindow(wm::display, m_title_bar);
}

void XClient::draw_left_button(bool highlight)
{
	XGCValues gcvalues;
	if (highlight)
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorHighlight);
	else if (m_states & State::Active)
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorActive);
	else
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorInactive);

	XChangeGC(wm::display, m_left_gc, GCForeground, &gcvalues);
	XFillRectangle(wm::display, m_left_pixmap, m_left_gc, 0, 0, m_button_w, m_header_h);

	gcvalues.foreground = m_screen->get_pixel(Color::WindowBackground);
	gcvalues.line_width = 4;
	gcvalues.line_style = LineSolid;
	XChangeGC(wm::display, m_left_gc, GCForeground|GCLineWidth|GCLineStyle, &gcvalues);
	XDrawRectangle(wm::display, m_left_pixmap, m_left_gc, 5, 5, m_button_w - 10 , m_header_h - 10);
	XDrawLine(wm::display, m_left_pixmap, m_left_gc, 5, 8, m_button_w - 5 , 8);
	XClearWindow(wm::display, m_left_button);
}

void XClient::draw_right_button(bool highlight)
{
	XGCValues gcvalues;
	if (highlight)
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorHighlight);
	else if (m_states & State::Active)
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorActive);
	else
		gcvalues.foreground = m_screen->get_pixel(Color::WindowDecorInactive);

	XChangeGC(wm::display, m_right_gc, GCForeground, &gcvalues);
	XFillRectangle(wm::display, m_right_pixmap, m_right_gc, 0, 0, m_button_w, m_header_h);

	gcvalues.foreground = m_screen->get_pixel(Color::WindowBackground);
	gcvalues.line_width = 4;
	gcvalues.line_style = LineSolid;
	XChangeGC(wm::display, m_right_gc, GCForeground|GCLineWidth|GCLineStyle, &gcvalues);
	XDrawLine(wm::display, m_right_pixmap, m_right_gc, 4, 4, m_button_w - 6 , m_header_h - 6);
	XDrawLine(wm::display, m_right_pixmap, m_right_gc, 4, m_header_h - 6, m_button_w - 6 , 4);
	XClearWindow(wm::display, m_right_button);
}

void XClient::draw_window_footer()
{
	if (m_states & State::NoFooter) return;
	unsigned long	pixel;

	if (m_states & State::Active) {
		pixel = m_screen->get_pixel(Color::WindowDecorActive);
	} else {
		pixel = m_screen->get_pixel(Color::WindowDecorInactive);
	}
	XSetWindowBackground(wm::display, m_left_handle, pixel);
	XClearWindow(wm::display, m_left_handle);
	XSetWindowBackground(wm::display, m_middle_handle, pixel);
	XClearWindow(wm::display, m_middle_handle);
	XSetWindowBackground(wm::display, m_right_handle, pixel);
	XClearWindow(wm::display, m_right_handle);
}

void XClient::draw_window_border()
{
	unsigned long	pixel;

	if (m_states & State::Tiled) {
		if (m_states & State::Urgent)
			pixel = m_screen->get_pixel(Color::WindowBorderUrgent);
		else if (m_states & State::Active)
			pixel = m_screen->get_pixel(Color::WindowBorderActive);
		else
			pixel = m_screen->get_pixel(Color::WindowBorderInactive);
	} else {
		if (m_states & State::Urgent)
			pixel = m_screen->get_pixel(Color::WindowBorderUrgent);
		else if (m_states & State::Active)
			pixel = m_screen->get_pixel(Color::WindowDecorActive);
		else
			pixel = m_screen->get_pixel(Color::WindowDecorInactive);
	}

	XSetWindowBorderWidth(wm::display, m_parent, (unsigned int)m_border_w);
	XSetWindowBorder(wm::display, m_parent, pixel | (0xffu << 24));
}

bool XClient::has_window(Window w)
{
	if (w == None) return false;
	if (w == m_window) return true;
	if (w == m_parent) return true;
	if (w == m_header) return true;
	if (w == m_left_button) return true;
	if (w == m_title_bar) return true;
	if (w == m_right_button) return true;
	if (w == m_footer) return true;
	if (w == m_left_handle) return true;
	if (w == m_middle_handle) return true;
	if (w == m_right_handle) return true;
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
	if (!wm::get_text_property(m_window, wm::ewmh[_NET_WM_NAME], text))
		wm::get_text_property(m_window, XA_WM_NAME, text);
	m_name = std::string(text.begin(), text.end());
	if (!m_name.empty()) m_name.pop_back();
}

void XClient::update_net_wm_name()
{
	get_net_wm_name();
	draw_window_header();
	update_statusbar_title();
}

void XClient::update_statusbar_title()
{
	if (!conf::clientsocket.length())
		return;
	std::string message = "window_active=" + m_name;
	util::send_message(message);
}

void XClient::get_net_wm_window_type()
{
	std::vector<Atom> atoms;
	wm::get_net_wm_window_type(m_window, atoms);
	for (Atom atom : atoms) {
		if (atom == wm::ewmh[_NET_WM_WINDOW_TYPE_DOCK]) {
			m_states |= (State::Docked|State::NoDecor);
			break;
		}
		if (atom == wm::ewmh[_NET_WM_WINDOW_TYPE_DIALOG]) {
			m_states |= (State::Floated|State::NoDecor);
			break;
		}
		if (atom == wm::ewmh[_NET_WM_WINDOW_TYPE_SPLASH]) {
			m_states |= (State::Floated|State::NoDecor);
			break;
		}
		if (atom == wm::ewmh[_NET_WM_WINDOW_TYPE_TOOLBAR]) {
			m_states |= (State::Floated|State::NoDecor);
			break;
		}
		if (atom == wm::ewmh[_NET_WM_WINDOW_TYPE_UTILITY]) {
			m_states |= (State::Floated|State::NoDecor);
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
	if (wm::get_net_wm_desktop(m_window, &index)) {
		index = std::min(index, (m_screen->get_num_desktops() - 1));
	}
	return index;
}

void XClient::get_wm_hints()
{
	XWMHints	*wmh;

	if ((wmh = XGetWMHints(wm::display, m_window)) != NULL) {
		if ((wmh->flags & InputHint) && (wmh->input))
			m_states |= State::Input;
		if ((wmh->flags & XUrgencyHint))
			m_states |= State::Urgent;	
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
			if (protocol[i] == wm::hints[WM_DELETE_WINDOW])
				m_states |= State::WMDeleteWindow;
			else if (protocol[i] == wm::hints[WM_TAKE_FOCUS])
				m_states |= State::WMTakeFocus;
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
			if (tc->m_states & State::Ignored) {
				m_states |= State::Floated|State::Ignored;
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

	hints = (MotifHints *)wm::get_window_property(m_window,
						wm::hints[_MOTIF_WM_HINTS],
			 			wm::hints[_MOTIF_WM_HINTS],
						Motif::HintElements, &n);
	if (!hints) return;

	if ((hints->flags & Motif::HintDecorations) &&
	    !(hints->decorations & Motif::DecorAll)) {
		if (!(hints->decorations & Motif::DecorBorder)) {
			m_states |= State::Floated|State::NoDecor;
			m_border_w = 0;
			m_header_h = 0;
			m_footer_h = 0;
		}
	}
	XFree(hints);
}

// Calculate initial placement of the window
void XClient::set_initial_placement()
{
	if (m_hints.flags & (USPosition | PPosition)) {
		Geometry view = m_screen->get_view();
		m_geom_child.set_user_placement(view, m_border_w);
		if (m_states & State::Ignored)
			m_geom_child.adjust_for_maximized(view, m_old_border);
	} else {
		Position pos = xutil::get_pointer_pos(m_rootwin);
		Geometry area = m_screen->get_area(pos, true);
		m_geom_child.set_placement(pos, area, m_border_w);
	}
	XMoveResizeWindow(wm::display, m_window, 0, 0, m_geom_child.w, m_geom_child.h);
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
void XClient::apply_configured_states()
{
	for (DefaultStates &def : conf::defstateslist) {
		bool match = true;
		if (!def.resname.empty() && def.resname.compare(m_res_name))
			match = false;
		if (!def.resclass.empty() && def.resclass.compare(m_res_class))
			match = false;
		if (match) m_states |= def.states;
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
	if (e->value_mask & CWWidth)
		m_geom_child.w = e->width;
	if (e->value_mask & CWHeight)
		m_geom_child.h = e->height;

	m_geom.w = m_geom_child.w;
	m_geom.h = m_geom_child.h + m_header_h + m_footer_h;

	resize_window();
}

void XClient::send_configure_event()
{
	XConfigureEvent	 xev;

	(void)memset(&xev, 0, sizeof(xev));
	xev.type = ConfigureNotify;
	xev.event = m_window;
	xev.window = m_window;
	xev.x = m_geom_child.x;
	xev.y = m_geom_child.y;
	xev.width = m_geom_child.w;
	xev.height = m_geom_child.h;
	xev.border_width = 0;
	xev.above = None;
	xev.override_redirect = False;

	XSendEvent(wm::display, m_window, False, StructureNotifyMask, (XEvent *)&xev);
}

void XClient::set_window_active()
{

	if ((m_states & State::Active) || has_states(State::Docked) || (m_states & State::Hidden))
		return;

	XInstallColormap(wm::display, m_colormap);

	if ((m_states & State::Input) || (!(m_states & State::WMTakeFocus)))
		XSetInputFocus(wm::display, m_window, RevertToPointerRoot, CurrentTime);
	
	if (m_states & State::WMTakeFocus)
		wm::send_client_message(m_window, wm::hints[WM_TAKE_FOCUS],
						wm::last_event_time);

	XClient *prev_client = m_screen->get_active_client();
	if (prev_client) {
		prev_client->m_states &= ~State::Active;
		prev_client->draw_window();
	}

	m_states |= State::Active;
	m_states &= ~State::Urgent;
	draw_window();
	m_screen->raise_client(this);
	wm::set_net_active_window(m_rootwin, m_window);
	update_statusbar_title();
}

void XClient::show_window()
{
	m_states &= ~State::Hidden;
	wm::set_net_wm_states(m_window, m_states);
	wm::set_wm_state(m_window, NormalState);
	XMapWindow(wm::display, m_parent);
	draw_window();
}

void XClient::hide_window()
{
	XUnmapWindow(wm::display, m_parent);
	if (m_states & State::Active) {
		m_states &= ~State::Active;
		wm::set_net_active_window(m_rootwin, None);
	}
	m_states |= State::Hidden;
	wm::set_net_wm_states(m_window, m_states);
	wm::set_wm_state(m_window, IconicState);
}

void XClient::close_window()
{
	if (m_states & State::WMDeleteWindow)
		wm::send_client_message(m_window, wm::hints[WM_DELETE_WINDOW], CurrentTime);
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
	if (m_states & State::Frozen)
		return;

	Geometry view = m_screen->get_view();
	m_geom.move(direction, view, m_border_w);

	Position pos = m_geom.get_center(Coordinates::Root);
	Geometry area = m_screen->get_area(pos, true);
	m_geom.snap_to_edge(area);

	move_window();
	move_pointer_inside();
	XSync(wm::display, True);
}

void XClient::hide_window_with_button()
{
	XEvent		 ev;
	Position	 p;
	bool		 is_inside = true;
	bool		 is_pressed = true;

	if (conf::debug) {
		std::cout << util::gettime() << " [XClient::" << __func__
			<< "]" << std::endl;
	}
	raise_window();
	draw_left_button(true);

	if (XGrabPointer(wm::display, m_left_button, False, MouseMask,
	    	GrabModeAsync, GrabModeAsync, None, wm::cursors[Pointer::ShapeNormal],
	   	 CurrentTime) != GrabSuccess) return;

	while (is_inside && is_pressed) {
		XMaskEvent(wm::display, MouseMask, &ev);
		switch (ev.type) {
		case MotionNotify:
			p.x = ev.xmotion.x;
			p.y = ev.xmotion.y;
			if ((ev.xmotion.x < 0) || (ev.xmotion.y < 0) ||
				(ev.xmotion.x > m_button_w) || (ev.xmotion.y > m_header_h))
				is_inside = false;
			break;
		case ButtonRelease:
			is_pressed = false;
			break;
		}
	}
	XUngrabPointer(wm::display, CurrentTime);
	draw_left_button(false);
	if (!is_inside) return;
	hide_window();
}

void XClient::close_window_with_button()
{
	XEvent		 ev;
	Position	 p;
	bool		 is_inside = true;
	bool		 is_pressed = true;

	if (conf::debug) {
		std::cout << util::gettime() << " [XClient::" << __func__
			<< "]" << std::endl;
	}
	raise_window();
	draw_right_button(true);

	if (XGrabPointer(wm::display, m_right_button, False, MouseMask,
	    	GrabModeAsync, GrabModeAsync, None, wm::cursors[Pointer::ShapeNormal],
	   	 CurrentTime) != GrabSuccess) return;

	while (is_inside && is_pressed) {
		XMaskEvent(wm::display, MouseMask, &ev);
		switch (ev.type) {
		case MotionNotify:
			p.x = ev.xmotion.x;
			p.y = ev.xmotion.y;
			if ((ev.xmotion.x < 0) || (ev.xmotion.y < 0) ||
				(ev.xmotion.x > m_button_w) || (ev.xmotion.y > m_header_h))
				is_inside = false;
			break;
		case ButtonRelease:
			is_pressed = false;
			break;
		}
	}
	XUngrabPointer(wm::display, CurrentTime);
	draw_right_button(false);
	if (!is_inside) return;
	close_window();
}

void XClient::move_window_with_pointer()
{
	XEvent		 ev;
	Time		 ltime = 0;
	int		 move = 1;
	Geometry 	 area;
	Position 	 pos;

	if (m_states & State::Frozen)
		return;

	raise_window();
	move_pointer_inside();

	if (XGrabPointer(wm::display, m_parent, False, MouseMask,
	    	GrabModeAsync, GrabModeAsync, None, wm::cursors[Pointer::ShapeMove],
	   	 CurrentTime) != GrabSuccess) return;

	PropWindow propwin(m_screen, m_parent);
	std::string label = std::to_string(m_geom.x) + " . " + std::to_string(m_geom.y);
	propwin.draw(label, m_geom.w/2, m_geom.h/2);

	while (move) {
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
			move = 0;
			break;
		}
	}
	if (ltime) move_window();
	XUngrabPointer(wm::display, CurrentTime);
	XSync(wm::display, True);
}

void XClient::move_window()
{
	XMoveWindow(wm::display, m_parent, m_geom.x, m_geom.y);
	send_configure_event();
}

void XClient::resize_window_with_keyboard(long direction)
{
	if (m_states & State::Frozen)
		return;

	m_geom.resize(direction, m_hints, m_border_w);
	resize_window();
	move_pointer_inside();
	XSync(wm::display, True);
}

void XClient::resize_window_with_pointer(Handle h)
{
	XEvent	 	ev;
	Time	 	ltime = 0;
	Cursor		cursor;

	if (m_states & State::Frozen) return;

	if (conf::debug>1) {
		std::cout << util::gettime() << " [XClient::" << __func__ << "]\n";
	}

	raise_window();

	switch(h) {
	case Handle::Left:
		cursor = wm::cursors[Pointer::ShapeResizeSW];
		break;
	case Handle::Middle:
		cursor = wm::cursors[Pointer::ShapeResizeSouth];
		break;
	case Handle::Right:
		cursor = wm::cursors[Pointer::ShapeResizeSE];
		break;
	}

	if (XGrabPointer(wm::display, m_parent, False, MouseMask,
	    	GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime) != GrabSuccess) return;

	PropWindow propwin(m_screen, m_parent);
	int width = (m_geom.w - m_hints.basew) / m_hints.incw;
	int height = (m_geom.h - m_hints.baseh) / m_hints.inch;
	std::string label = std::to_string(width) + " x " + std::to_string(height);
	propwin.draw(label, m_geom.w/2, m_geom.h/2);

	int xmax = m_geom.x + m_geom.w;
	bool resize = true;
	while (resize) {
		XMaskEvent(wm::display, MouseMask, &ev);
		switch (ev.type) {
		case MotionNotify:
			// not more than 60 times / second
			if ((ev.xmotion.time - ltime) <= (1000 / 60))
				continue;
			ltime = ev.xmotion.time;

			switch(h) {
			case Handle::Left:
				m_geom.x = ev.xmotion.x_root;
				m_geom.w = xmax - m_geom.x;
				m_geom.h = ev.xmotion.y;
				break;
			case Handle::Middle:
				m_geom.h = ev.xmotion.y;
				break;
			case Handle::Right:
				m_geom.w = ev.xmotion.x;
				m_geom.h = ev.xmotion.y;
				break;
			}
			m_geom.apply_size_hints(m_hints);
			resize_window();

			width = (m_geom.w - m_hints.basew) / m_hints.incw;
			height = (m_geom.h - m_hints.baseh) / m_hints.inch;
			label = std::to_string(width) + " x " + std::to_string(height);
			propwin.draw(label, m_geom.w/2, m_geom.h/2);
			break;
		case ButtonRelease:
			resize = false;
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

	if (m_header_h) {
		XMoveResizeWindow(wm::display, m_left_button, 0, 0, m_button_w, m_header_h);
		XMoveResizeWindow(wm::display, m_title_bar, m_button_w + 2, 0,
					m_geom.w - 2 * m_button_w - 4, m_header_h);
		XMoveResizeWindow(wm::display, m_right_button, m_geom.w - m_button_w, 0,
					m_button_w, m_header_h);

		if (m_title_width < m_geom.w - 2 * m_button_w - 4) {
			m_title_width = m_geom.w + 50;
			XFreePixmap(wm::display, m_title_pixmap);
			m_title_pixmap = XCreatePixmap(wm::display, m_title_bar, m_title_width,
				m_header_h, DefaultDepth(wm::display, m_screen->get_screenid()));
			XSetWindowBackgroundPixmap(wm::display, m_title_bar, m_title_pixmap);
			XftDrawChange(m_title_draw, m_title_pixmap);
		}
		XMoveResizeWindow(wm::display, m_header, 0, 0, m_geom.w, m_header_h);
	}

	m_geom_child.x = 0;
	m_geom_child.y = m_header_h;
	m_geom_child.w = m_geom.w;
	m_geom_child.h = m_geom.h - (m_header_h + m_footer_h);
	XMoveResizeWindow(wm::display, m_window, 0, m_header_h, m_geom_child.w, m_geom_child.h);

	if (m_footer_h) {
		XMoveResizeWindow(wm::display, m_left_handle, 0, 0, m_handle_w, m_footer_h);
		XMoveResizeWindow(wm::display, m_middle_handle, m_handle_w + 4, 0,
					m_geom.w - 2 * m_handle_w - 8, m_footer_h);
		XMoveResizeWindow(wm::display, m_right_handle, m_geom.w - m_handle_w, 0,
					m_handle_w, m_footer_h);
		XMoveResizeWindow(wm::display, m_footer, 0, m_geom.h - m_footer_h,
					m_geom.w, m_footer_h);
	}
	m_geom_floated = m_geom;

	draw_window();
	send_configure_event();
}

void XClient::snap_window(long direction)
{
	if (m_states & State::Frozen) return;
	Position pos = m_geom.get_center(Coordinates::Root);
	Geometry area = m_screen->get_area(pos, true);
	m_geom.warp_to_edge(direction, area, m_border_w);
	move_window();
	move_pointer_inside();
}

void XClient::move_pointer_inside()
{
	m_ptr = xutil::get_pointer_pos(m_parent);
	m_ptr.move_inside(m_geom);
	xutil::set_pointer_pos(m_parent, m_ptr);
}

void XClient::warp_pointer()
{
	xutil::set_pointer_pos(m_parent, m_ptr);
}

void XClient::save_pointer()
{
	Position p = xutil::get_pointer_pos(m_parent);
	if (m_geom.contains(p, Coordinates::Window)) {
		m_ptr = p;
	} else {
		m_ptr = m_geom.get_center(Coordinates::Window);
	}
}

void XClient::set_tiling(Geometry &tiled)
{
	if (!(m_states & State::Tiled)) return;
	if (m_states & State::FullScreen)
		remove_fullscreen();

	if (!conf::tileheader)
		m_header_h = 0;
	m_footer_h = 0;
	m_geom = tiled;

	m_states |= State::Frozen;	
	wm::set_net_wm_states(m_window, m_states);
	resize_window();
}

void XClient::set_floated()
{
	if (m_states & State::FullScreen)
		remove_fullscreen();

	m_states &= ~(State::Tiled|State::Frozen|State::Maximized);
	m_geom = m_geom_floated;
	if (!(m_states & State::NoHeader))
		m_header_h = m_font->ascent + m_font->descent;
	if (!(m_states & State::NoFooter))
		m_footer_h = conf::footerheight;
	if (!(m_states & State::NoDecor))
		m_border_w = conf::borderwidth;

	resize_window();
}

void XClient::change_states(int action, Atom a, Atom b)
{
	for (const StateMap &sm : wm::statemaps) {
		if (a != wm::ewmh[sm.atom] &&
		    b != wm::ewmh[sm.atom])
			continue;
		switch(action) {
		case _NET_WM_STATE_ADD:
			if (!(m_states & sm.state))
				toggle_state(sm.state);
			break;
		case _NET_WM_STATE_REMOVE:
			if (m_states & sm.state)
				toggle_state(sm.state);
			break;
		case _NET_WM_STATE_TOGGLE:
			toggle_state(sm.state);
			break;
		}
		break;
	}
}

void XClient::toggle_state(long flag)
{
	switch(flag) {
	case State::Urgent:
		if (!(m_states & State::Active))
			m_states |= State::Urgent;
		break;
	case State::Hidden:
	case State::SkipPager:
	case State::SkipTaskbar:
		m_states ^= flag;
		break;
	case State::Sticky:
		if (m_states & State::Sticky)
			m_screen->assign_client_to_desktop(this, 
				 m_screen->get_active_desktop(), true);
		else
			m_screen->assign_client_to_desktop(this, -1, true);
		m_states ^= flag;
		break;
	case State::Tiled:
		if (m_states & State::Tiled) {
			m_screen->remove_window_tile_from_desktop(this);
			set_floated();
		} else {
			m_states |= (State::Tiled|State::Frozen);
			m_screen->add_window_tile_to_desktop(this);
		}	
		break;
	case State::FullScreen:
		toggle_fullscreen();
		break;
	}
	wm::set_net_wm_states(m_window, m_states);
}

void XClient::toggle_fullscreen()
{
	if ((m_states & State::Frozen) && !(m_states & (State::FullScreen|State::Tiled)))
		return;

	if (m_states & State::FullScreen)
		remove_fullscreen();
	else {
		if (m_states & State::Tiled)
			m_geom_tiling = m_geom;
		else
			m_geom_floated = m_geom;

		Position pos = m_geom.get_center(Coordinates::Root);
		Geometry area = m_screen->get_area(pos, false);
		m_border_w = 0;
		m_header_h = 0;
		m_footer_h = 0;
		m_geom = area;
		m_states |= (State::FullScreen|State::Frozen);
		raise_window();
	}

	resize_window();
	move_pointer_inside();
}

void XClient::remove_fullscreen()
{
	if (!(m_states & State::NoHeader))
		m_header_h = m_font->ascent + m_font->descent;
	if (!(m_states & State::NoFooter))
		m_footer_h = conf::footerheight;
	if (!(m_states & State::NoDecor))
		m_border_w = 1;

	if (m_states & State::Tiled) {
		m_border_w = conf::borderwidth;	
		m_footer_h = 0;	
		if (!conf::tileheader)
			m_header_h = 0;
		m_geom = m_geom_tiling;
	} else {
		m_geom = m_geom_floated;
		m_states &= ~State::Frozen;
	}
	m_states &= ~State::FullScreen;
}
