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

#ifndef _DESKTOP_H_
#define _DESKTOP_H_
#include <string>
#include <vector>
#include "definitions.h"

class XClient;
class XScreen;

class Desktop {
	std::string		 m_name;
	XScreen			*m_screen;
	long		 	 m_index;
	std::vector<XClient*>	 m_clientstack;
	std::vector<XClient*>	 m_clienttile;
	long			 m_mode;
	float			 m_split;
public:
	Desktop(XScreen *, long, std::string&, long, float);
	std::string		&get_name() { return m_name; }
	long			 get_index() const { return m_index; }
	std::vector<XClient*>	&get_clients() { return m_clientstack; }
	bool			 is_empty() const { return m_clientstack.empty(); }
	void 			 add_window(XClient *);
	void 			 remove_window(XClient *);
	void			 add_window_tile(XClient *);
	void			 remove_window_tile(XClient *);
	void			 raise_window(XClient *);
	void			 rotate_windows(long);
	void 			 cycle_windows(XClient *, long);
	void			 master_split(long);
	void			 show();
	void			 hide();
	void			 close();
	void			 toggle();
	void			 rotate_mode(long);
private:
	void			 restack_windows();
	void 			 tile_horizontal();
	void 			 tile_vertical();
	void 			 tile_maximized();
	bool			 has_hidden_only();
	XClient 		*next_window(XClient *);
	XClient 		*prev_window(XClient *);
};

#endif /* _DESKTOP_H_ */
