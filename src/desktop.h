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

#ifndef _DESKTOP_H_
#define _DESKTOP_H_
#include <string>
#include <vector>
#include "enums.h"

class XClient;
class XScreen;

class Desktop {
	std::string		 m_name;
	XScreen			*m_screen;
	long		 	 m_index;
	long			 m_layout;
	long			 m_layout_index;
	float			 m_master_split;
	long			 m_cols;
	long			 m_rows;
public:
	Desktop(std::string&, XScreen *, long, std::string&, float);
	std::string		&get_name() { return m_name; }
	void			 rotate_windows(std::vector<XClient*>&, long);
	void 			 cycle_windows(std::vector<XClient*>&, XClient *, long);
	void 			 swap_windows(std::vector<XClient*>&, XClient *, long);
	void			 master_resize(std::vector<XClient*>&, long);
	void			 show(std::vector<XClient*>&);
	void			 hide(std::vector<XClient*>&);
	void			 close(std::vector<XClient*>&);
	void			 select_layout(std::vector<XClient*>&, long);
	void			 rotate_layout(std::vector<XClient*>&, long);
	void			 panel_update_layout();
private:
	void			 restack_windows(std::vector<XClient*>&);
	void 			 tile_grid(std::vector<XClient*>&);
	void 			 tile_horizontal(std::vector<XClient*>&);
	void 			 tile_vertical(std::vector<XClient*>&);
	void 			 tile_maximized(std::vector<XClient*>&);
	void 			 stacked_desktop(std::vector<XClient*>&);
	std::vector<XClient*>::iterator next_desktop_client(std::vector<XClient*>&,
								XClient *);
	std::vector<XClient*>::reverse_iterator prev_desktop_client(std::vector<XClient*>&, 
								XClient *);
};
#endif /* _DESKTOP_H_ */
