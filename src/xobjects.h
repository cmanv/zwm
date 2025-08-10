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

#ifndef _XOBJECTS_H_
#define _XOBJECTS_H_
#include <X11/Xft/Xft.h>
#include <string>
#include "definitions.h"

class XScreen;
struct Geometry;

struct MotifHints {
	unsigned long	flags;
	unsigned long	functions;
	unsigned long	decorations;
	long		inputMode;
	unsigned long	status;
};

struct SizeHints {
	long	flags;		// defined hints
	int	basew;		// desired width
	int	baseh;		// desired height
	int	minw;		// minimum width
	int	minh;		// minimum height
	int	maxw;		// maximum width
	int	maxh;		// maximum height
	int	incw;		// width increment progression
	int	inch;		// height increment progression
	float	mina;		// minimum aspect ratio
	float	maxa;		// maximum aspect ratio
	SizeHints() {}
	SizeHints(XSizeHints &);
};

struct Position {
	int	 x;
	int	 y;
	Position() { x = 0; y = 0; }
	Position(int px, int py): x(px), y(py) {}
	bool 	operator==(Position p) { return ((x == p.x) && (y == p.y)); }
	bool 	operator!=(Position p) { return ((x != p.x) || (y != p.y)); }
	void 	move_inside(Geometry &);
	void 	move(long);
};

struct BorderGap {
	int	 top;
	int	 bottom;
	int	 left;
	int	 right;
	BorderGap() { top=0; bottom=0; left=0; right=0; }
	BorderGap(int t, int b, int l, int r):
		 top(t), bottom(b), left(l), right(r) {}
};

struct Geometry {
	int	 x;
	int	 y;
	int	 w;
	int	 h;
	Geometry() { x=0; y=0; w=0; h=0; }
	Geometry(int px, int py, int pw, int ph):
		x(px), y(py), w(pw), h(ph) {}
	Position	get_center(Coordinates);
	bool	 	contains(Position, Coordinates);
	bool	 	intersects(Geometry &, int);
	void 	 	set_pos(int, int);
	void 	 	set_pos(Position);
	void 		set_menu_placement(Position, Geometry &, int);
	void 		set_menu_placement(Geometry &, Geometry &, int, int);
	void 		set_placement(Position, Geometry &, int);
	void 		set_user_placement(Geometry&, int);
	void 		adjust_for_maximized(Geometry&, int);
	void 	 	move(long, Geometry &, int);
	void 	 	resize(long, SizeHints &, int);
	void 	 	warp_to_edge(long, Geometry &, int);
	void 	 	snap_to_edge(Geometry &);
	void	 	apply_border_gap(BorderGap &);
	void 		apply_size_hints(SizeHints &);
};

class Viewport {
	int		num;
	Geometry 	view;
	Geometry 	work;
public:
	Viewport(int id, int x, int y, int w, int h, BorderGap&);
	Viewport(int id, Geometry&, BorderGap&);
	int		get_num() const { return num; }
	Geometry	get_view() const { return view; }
	bool 		contains(Position);
};

class PropWindow {
	Window		 m_window;
	XftDraw		*m_xftdraw;
	XftFont		*m_font;
	XftColor	*m_color;
	unsigned long	 m_pixel;
public:
	PropWindow(XScreen *, Window);
	~PropWindow();
	void 		draw(std::string &, int, int);
};

struct DefaultStates {
	std::string 	resname;
	std::string 	resclass;
	long		states;
	DefaultStates(std::string &n, std::string &c, long s)
		:resname(n), resclass(c), states(s) {}
};

struct DefaultDesktop {
	std::string 	resname;
	std::string 	resclass;
	long 		index;
	DefaultDesktop(std::string &n, std::string &c, long i)
		:resname(n), resclass(c), index(i) {}
};

struct DesktopDef {
	long		index;
	std::string 	name;
	std::string	mode;
	float		split;
};

struct DesktopMode {
	long		index;
	std::string	name;
	std::string	letter;
	DesktopMode(long i, std::string n, std::string l)
		:index(i), name(n), letter(l) {}
};
#endif /* _XOBJECTS_H_ */
