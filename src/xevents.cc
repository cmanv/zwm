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

#include <X11/extensions/Xrandr.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <iostream>
#include "misc.h"
#include "config.h"
#include "binding.h"
#include "wmhints.h"
#include "winmgr.h"
#include "xclient.h"
#include "xscreen.h"
#include "xevents.h"

namespace XEvents {
	static void key_press(XEvent *);
	static void key_release(XEvent *);
	static void button_press(XEvent *);
	static void enter_notify(XEvent *);
	static void expose(XEvent *);
	static void destroy_notify(XEvent *);
	static void ummap_notify(XEvent *);
	static void map_request(XEvent *);
	static void configure_request(XEvent *);
	static void property_notify(XEvent *);
	static void client_message(XEvent *);
	static void mappping_notify(XEvent *);
	static void screen_change_notify(XEvent *);

	static const long IgnoreModMask	= LockMask|Mod2Mask|0x2000;

	static std::vector<KeySym> modkeys = {
		XK_Alt_L,
		XK_Alt_R,
		XK_Super_L,
		XK_Super_R,
		XK_Control_L,
		XK_Control_R,
		XK_ISO_Level3_Shift
	};
}

static void XEvents::key_press(XEvent *ee)
{
	XKeyEvent	*e = &ee->xkey;

	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] root 0x" << std::hex << e->root << " window 0x" << e->window << '\n';
	}

	XScreen *screen = XScreen::find_screen(e->root);
	if (!screen) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] screen not found for root window\n";
		}
		return;
	}

	XClient	*client = XScreen::find_client(e->window);
	if (!client) client = screen->get_active_client();

	KeySym keysym = XkbKeycodeToKeysym(wm::display, e->keycode, 0, 0);
	KeySym skeysym = XkbKeycodeToKeysym(wm::display, e->keycode, 0, 1);

	e->state &= ~IgnoreModMask;

	Binding *kb = NULL;
	for (Binding& ckb : conf::keybindings) {
		if (!client && ckb.context == Context::Window)
			continue;

		unsigned int	 modshift = 0;
		if (keysym != ckb.keysym && skeysym == ckb.keysym)
			modshift = ShiftMask;

		if ((ckb.modmask | modshift) != e->state)
			continue;

		if (ckb.keysym == ((modshift == 0) ? keysym : skeysym)) {
			kb = &ckb;
			break;
		}
	}
	if (!kb) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] keybinding not matched!\n";
		}
		return;
	}

	switch (kb->context) {
	case Context::Root:
		(*kb->fscreen)(screen, kb->param);
		break;
	case Context::Window:
		(*kb->fclient)(client, kb->param);
		break;
	case Context::FuncCall:
		(*kb->fcall)(kb->param);
		break;
	case Context::Launcher:
		(*kb->flaunch)(kb->path);
		break;
	default: ;
	}
}

// This is only used for the modifier suppression detection.
static void XEvents::key_release(XEvent *ee)
{
	XKeyEvent		*e = &ee->xkey;

	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] root 0x" << std::hex << e->root << " window 0x" << e->window << '\n';
	}

	XScreen *screen = XScreen::find_screen(e->root);
	if (!screen) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] screen not found for root window\n";
		}
		return;
	}

	KeySym keysym = XkbKeycodeToKeysym(wm::display, e->keycode, 0, 0);
	for (auto modkey : modkeys) {
		if (keysym == modkey) {
			XClient *client = screen->get_active_client();
			if (client && screen->is_cycling()) {
				screen->stop_cycling();
				screen->raise_client(client);
			}
			XUngrabKeyboard(wm::display, CurrentTime);
			break;
		}
	}
}

static void XEvents::button_press(XEvent *ee)
{
	XButtonEvent	*e = &ee->xbutton;

	if (conf::debug>2)
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] root 0x" << std::hex << e->root << " window 0x" << e->window << '\n';

	XScreen *screen = XScreen::find_screen(e->root);
	if (!screen) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] screen not found for root window\n";
		}
		return;
	}
	XClient	*client = XScreen::find_client(e->window);
	e->state &= ~IgnoreModMask;

	Binding *mb = NULL;
	for (Binding& cmb : conf::mousebindings) {
		if (e->button != cmb.button || e->state != cmb.modmask) continue;
		if (client && cmb.context == Context::Root) continue;
		if (!client && cmb.context == Context::Window) continue;
		mb = &cmb;
		break;
	}
	if (!mb) return;

	switch (mb->context) {
	case Context::Root:
		(*mb->froot)(screen);
		break;
	case Context::Window:
		(*mb->fclient)(client, mb->param);
		break;
	default: ;
	}
}

// Set the window active whenever the pointer enters
static void XEvents::enter_notify(XEvent *ee)
{
	XCrossingEvent	*e = &ee->xcrossing;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	wm::last_event_time = e->time;
	XClient *client = XScreen::find_client(e->window);
	if (client)
		client->set_window_active();
}

static void XEvents::expose(XEvent *ee)
{
	XExposeEvent	*e = &ee->xexpose;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	XClient *client = XScreen::find_client(e->window);
	if ((client) && e->count == 0)
		client->draw_window_border();
}

static void XEvents::destroy_notify(XEvent *ee)
{
	XDestroyWindowEvent	*e = &ee->xdestroywindow;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	XClient *client = XScreen::find_client(e->window);
	if (client) {
		XScreen *screen = client->get_screen();
		screen->remove_client(client);
	}
}

static void XEvents::ummap_notify(XEvent *ee)
{
	XUnmapEvent		*e = &ee->xunmap;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	XClient *client = XScreen::find_client(e->window);
	if (client) {
		// This was generated by a XSendEvent
		if (e->send_event) {
			wmh::set_wm_state(e->window, WithdrawnState);
		} else {
			if (!(client->has_state(State::Hidden))) {
				if (client->ignore_unmap()) return;
				XScreen *screen = client->get_screen();
				screen->remove_client(client);
			}
		}
	}
}

static void XEvents::map_request(XEvent *ee)
{
	XMapRequestEvent	*e = &ee->xmaprequest;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] parent 0x" << std::hex << e->parent << " window 0x" << e->window << '\n';
	}

	XScreen *screen = XScreen::find_screen(e->parent);
	if (!screen) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] screen not found for root window\n";
		}
		return;
	}

	XClient *active_client = screen->get_active_client();
	if (active_client)
		active_client->save_pointer();

	XClient *client = XScreen::find_client(e->window);
	if (!client)
		screen->add_client(e->window);
}

static void XEvents::configure_request(XEvent *ee)
{
	XConfigureRequestEvent	*e = &ee->xconfigurerequest;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	XClient *client = XScreen::find_client(e->window);
	if (client) {
		client->configure_window(e);
	} else {
		// We do not manage this window at this time.
		XWindowChanges	 wc;
		wc.x = e->x;
		wc.y = e->y;
		wc.width = e->width;
		wc.height = e->height;
		wc.border_width = e->border_width;
		wc.stack_mode = Above;
		e->value_mask &= ~CWStackMode;

		XConfigureWindow(wm::display, e->window, e->value_mask, &wc);
	}
}

static void XEvents::property_notify(XEvent *ee)
{
	XPropertyEvent	*e = &ee->xproperty;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << e->window << '\n';
	}

	XClient *client = XScreen::find_client(e->window);
	if (client) {
		XScreen *screen = client->get_screen();
		long index;
		switch (e->atom) {
		case XA_WM_NORMAL_HINTS:
			client->get_wm_normal_hints();
			break;
		case XA_WM_NAME:
			client->update_net_wm_name();
			break;
		case XA_WM_HINTS:
			client->get_wm_hints();
			client->draw_window_border();
			break;
		case XA_WM_TRANSIENT_FOR:
			client->get_transient();
			client->draw_window_border();
			index = client->get_desktop_index();
			if (index > -1)
				screen->move_client_to_desktop(client, index);
			break;
		default:
			if (e->atom == ewmh::hints[_NET_WM_NAME])
				client->update_net_wm_name();
			break;
		}
	} else {
		if (e->atom == ewmh::hints[_NET_DESKTOP_NAMES])  {
			XScreen *screen = XScreen::find_screen(e->window);
			if (screen)
				screen->set_net_desktop_names();
		}
	}
}

static void XEvents::client_message(XEvent *e)
{
	XClientMessageEvent	*xev = &e->xclient;
	XClient 		*client, *active_client;
	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << xev->window << '\n';
	}

	if (xev->message_type == wmh::hints[WM_CHANGE_STATE]) {
		if ((client = XScreen::find_client(xev->window)) != NULL) {
    			if (xev->data.l[0] == IconicState)
				client->hide_window();
		}
	} else if (xev->message_type == ewmh::hints[_NET_CLOSE_WINDOW]) {
		if ((client = XScreen::find_client(xev->window)) != NULL) {
			client->close_window();
		}
	} else if (xev->message_type == ewmh::hints[_NET_ACTIVE_WINDOW]) {
		if ((client = XScreen::find_client(xev->window)) != NULL) {
			XScreen *screen = client->get_screen();
			if ((active_client = screen->get_active_client()) != NULL)
				active_client->save_pointer();
			client->show_window();
			client->warp_pointer();
		}
	} else if (xev->message_type == ewmh::hints[_NET_WM_DESKTOP]) {
		if ((client = XScreen::find_client(xev->window)) != NULL) {
			long index = xev->data.l[0];
			if (index == (unsigned long)-1)
				client->set_states(State::Sticky);
			else {
				if (index >= 0 && index < conf::ndesktops) {
					XScreen *screen = client->get_screen();
					screen->move_client_to_desktop(client, index);
				}
			}
		}
	} else if (xev->message_type == ewmh::hints[_NET_WM_STATE]) {
		if ((client = XScreen::find_client(xev->window)) != NULL) {
			client->change_states(xev->data.l[0], xev->data.l[1],
						xev->data.l[2]);
		}
	} else if (xev->message_type == ewmh::hints[_NET_CURRENT_DESKTOP]) {
		XScreen *screen = XScreen::find_screen(xev->window);
		if ((screen) && xev->data.l[0] >= 0 &&
			xev->data.l[0] < screen->get_num_desktops()) {
			screen->switch_to_desktop(xev->data.l[0]);
		}
	}
}

// Called when the keymap has changed.
// Ungrab all keys, reload keymap and then regrab
static void XEvents::mappping_notify(XEvent *e)
{
	XMappingEvent		*xev = &e->xmapping;

	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] window 0x" << std::hex << xev->window << '\n';
	}

	XRefreshKeyboardMapping(xev);
	if (xev->request == MappingKeyboard) {
		for (XScreen *screen : wm::screenlist)
			screen->grab_keybindings();
	}
}

static void XEvents::screen_change_notify(XEvent *e)
{

	XRRScreenChangeNotifyEvent	*xev = (XRRScreenChangeNotifyEvent *)e;

	if (conf::debug>2) {
		std::cout << debug::gettime() << " [XEvents::" << __func__
			<< "] root 0x" << std::hex << xev->root
			<< "size:(" << std::dec << xev->width << ", " << xev->height << ")\n";
	}

	XScreen *screen = XScreen::find_screen(xev->root);
	if (!screen) {
		if (conf::debug>1) {
			std::cout << debug::gettime() << " [XEvents::" << __func__
				<< "] screen not found for root window\n";
		}
		return;
	}

	XRRUpdateConfiguration(e);
	screen->update_geometry();
	screen->ensure_clients_are_visible();
}

void XEvents::process(void)
{
	XEvent	 e;

	while (XPending(wm::display)) {
		XNextEvent(wm::display, &e);
		if ((e.type - wm::xrandr_event_base) == RRScreenChangeNotify) {
			screen_change_notify(&e);
			continue;
		}
		if (e.type >= LASTEvent) continue;

		switch(e.type) {
		case KeyPress:
			key_press(&e);
			break;
		case KeyRelease:
			key_release(&e);
			break;
		case ButtonPress:
			button_press(&e);
			break;
		case EnterNotify:
			enter_notify(&e);
			break;
		case Expose:
			expose(&e);
			break;
		case DestroyNotify:
			destroy_notify(&e);
			break;
		case UnmapNotify:
			ummap_notify(&e);
			break;
		case MapRequest:
			map_request(&e);
			break;
		case ConfigureRequest:
			configure_request(&e);
			break;
		case PropertyNotify:
			property_notify(&e);
			break;
		case ClientMessage:
			client_message(&e);
			break;
		case MappingNotify:
			mappping_notify(&e);
			break;
		}
	}
}
