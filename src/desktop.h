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
	long			 m_mode_index;
	float			 m_split;
public:
	Desktop(XScreen *, long, std::string&, std::string&, float);
	std::string		&get_name() { return m_name; }
	long			 get_index() const { return m_index; }
	void			 rotate_windows(std::vector<XClient*>&, long);
	void 			 cycle_windows(std::vector<XClient*>&, XClient *, long);
	void			 master_split(std::vector<XClient*>&, long);
	void			 show(std::vector<XClient*>&);
	void			 hide(std::vector<XClient*>&);
	void			 close(std::vector<XClient*>&);
	void			 toggle();
	void			 select_mode(std::vector<XClient*>&, const std::string&);
	void			 rotate_mode(std::vector<XClient*>&, long);
private:
	void			 restack_windows(std::vector<XClient*>&);
	void 			 tile_horizontal(std::vector<XClient*>&);
	void 			 tile_vertical(std::vector<XClient*>&);
	void 			 tile_maximized(std::vector<XClient*>&);
	void 			 stacked_desktop(std::vector<XClient*>&);
	bool			 has_hidden_only();
	XClient 		*next_window(std::vector<XClient*>&, XClient *);
	XClient 		*prev_window(std::vector<XClient*>&, XClient *);
};
#endif /* _DESKTOP_H_ */
