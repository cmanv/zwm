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

#ifndef _XCLIENT_H_
#define _XCLIENT_H_
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include "definitions.h"
#include "xobjects.h"

//class XClient;
class XScreen;

class XClient {
	Window	  	 	 m_rootwin;
	Window	  	 	 m_parent;
	Window	  	 	 m_window;

	XScreen	  		*m_screen;
	long			 m_deskindex;

	long			 m_states;
	int			 m_initial_state;

	Colormap		 m_colormap;
	XftFont			*m_font;

	Geometry		 m_geom;
	Geometry 		 m_geom_stack;
	Geometry 		 m_geom_save;

	Position		 m_ptr;
	SizeHints		 m_hints;

	int			 m_border_w;
	int			 m_old_border;		// original border width
	bool			 m_removed;
	bool			 m_ignore_unmap;

	std::string		 m_name;
	std::string		 m_res_class;
	std::string		 m_res_name;
public:
	XClient(Window, XScreen*, bool);
	~XClient();
	Window			 get_window() const { return m_window; }
	Window			 get_parent() const { return m_parent; }
	Window			 get_rootwin() const { return m_rootwin; }
	XScreen  		*get_screen() const { return m_screen; }

	std::string		&get_name() { return m_name; }
	Position		&get_saved_pointer() { return m_ptr; }
	Geometry		&get_geometry() { return m_geom; }
	void			 clear_states(long s) { m_states &= ~s; }
	long			 get_states() const { return m_states; }
	bool			 has_states(long s) const { return ((m_states&s) == s); }
	bool			 has_state(long s) const { return (m_states&s); }
	void			 set_states(long s) { m_states |= s; }
	int			 get_border() const { return m_border_w; }
	void			 set_border(int b) { m_border_w = b; }
	long			 get_desktop_index() const { return m_deskindex; }
	void			 set_desktop_index(long i) { m_deskindex = i; }
	std::string		&get_res_name() { return m_res_name; }
	std::string		&get_res_class() { return m_res_class; }
	void			 set_removed() { m_removed = true; }
	bool 			 ignore_unmap();
	bool			 has_window(Window);
	
	void			 get_net_wm_name();
	void			 update_net_wm_name();
	void			 get_transient();
	void			 get_wm_hints();
	void			 get_wm_normal_hints();

	void			 send_configure_event();
	void			 configure_window(XConfigureRequestEvent *);

	void			 draw_window_border();
	void 			 set_window_active();
	void			 show_window();
	void			 hide_window();
	void			 close_window();
	void			 raise_window();
	void			 lower_window();
	void			 move_window_with_keyboard(long);
	void			 move_window_with_pointer();
	void			 move_window();
	void			 resize_window_with_keyboard(long);
	void			 resize_window_with_pointer();
	void			 snap_window(long);

	void			 save_pointer();
	void			 warp_pointer();
	void			 grab_pointer();
	void			 move_pointer_inside();

	void			 toggle_fullscreen();
	void			 set_stacked_geom();
	void			 set_tiled_geom(Geometry &);
	void 			 set_notile();

	void			 update_statusbar_title();
	void 			 change_states(int, Atom, Atom);
	void			 toggle_state(long);

private:
	void			 reparent_window();
	void			 resize_window();

	void			 set_initial_placement();
 	void			 apply_user_states();
 	long			 get_configured_desktop();
	void			 get_net_wm_window_type();
	long			 get_net_wm_desktop();
	void			 get_class_hint();
	void			 get_motif_hints();
	void			 get_wm_protocols();
	void 			 remove_fullscreen();

	static const long 	 ButtonMask;
	static const long 	 MouseMask;
	static const long 	 ModifierMask;
};

#endif /* _XCLIENT_H_ */
