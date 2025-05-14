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

#include "definitions.h"
#include "winmgr.h"
#include "config.h"
#include "xscreen.h"
#include "xobjects.h"

SizeHints::SizeHints(XSizeHints &hints)
{
	flags = hints.flags;

	basew = 0;
	baseh = 0;
	if (hints.flags & PBaseSize) {
		basew = hints.base_width;
		baseh = hints.base_height;
	} else if (hints.flags & PMinSize) {
		basew = hints.min_width;
		baseh = hints.min_height;
	}

	minw = 0;
	minh = 0;
	if (hints.flags & PMinSize) {
		minw = hints.min_width;
		minh = hints.min_height;
	} else if (hints.flags & PBaseSize) {
		minw = hints.base_width;
		minh = hints.base_height;
	}

	maxw = 0;
	maxh = 0;
	if (hints.flags & PMaxSize) {
		maxw = hints.max_width;
		maxh = hints.max_height;
	}

	incw = 0;
	inch = 0;
	if (hints.flags & PResizeInc) {
		incw = hints.width_inc;
		inch = hints.height_inc;
	}

	incw = std::max(1, incw);
	inch = std::max(1, inch);
	minw = std::max(1, minw);
	minh = std::max(1, minh);

	mina = 0;
	maxa = 0;
	if (hints.flags & PAspect) {
		if (hints.min_aspect.x > 0)
			mina = (float)hints.min_aspect.y /
			    hints.min_aspect.x;
		if (hints.max_aspect.y > 0)
			maxa = (float)hints.max_aspect.x /
			    hints.max_aspect.y;
	}
}

void Position::move_inside(Geometry &geom)
{
	if (x < 0) x = 0;
	else if (x > geom.w - 1) x = geom.w - 1;
	if (y < 0) y = 0;
	else if (y > geom.h - 1) y = geom.h - 1;
}

void Position::move(long direction)
{
	int amt = conf::moveamount;
	if (direction & Direction::West)
		x -= amt;
	if (direction & Direction::East)
		x += amt;
	if (direction & Direction::North)
		y -= amt;
	if (direction & Direction::South)
		y += amt;
}

Position Geometry::get_center(Coordinates c)
{
	if (c == Coordinates::Root) 
		return Position(x + w/2, y + h/2);
	else
		return Position(w/2, h/2);
}

bool Geometry::contains(Position p, Coordinates c)
{
	if (c == Coordinates::Root)
		return ((p.x < x + w) && (p.x >= x) && (p.y < y + h) && (p.y >= y));
	else		
		return ((p.x < w) && (p.x >= 0) && (p.y < h) && (p.y >= 0));
}

bool Geometry::intersects(Geometry &view, int border)
{
	if (x + w + (2*border) - 1 < view.x) return false;
	if (y + h + (2*border) - 1 < view.y) return false;
	if (view.x + view.w < x) return false;
	if (view.y + view.h < y) return false;
	return true; 
}

void Geometry::set_pos(int px, int py) 
{ 
	x = px; 
	y = py; 
}

void Geometry::set_pos(Position p) 
{ 
	x = p.x; 
	y = p.y; 
}

void Geometry::set_menu_placement(Position p, Geometry &area, int border)
{
	x = p.x;
	y = p.y;
	if (x + w + 2 * border > area.x + area.w)
		x = area.x + area.w - w - 2 * border;
	if (y + h + 2 * border > area.y + area.h)
		y = area.y + area.h - h - 2 * border;
}

void Geometry::set_menu_placement(Geometry &parent, Geometry &area, int ypos, int border)
{
	x = parent.x + parent.w - 2 * border;
	y = parent.y + ypos;
	if (x + w > area.x + area.w)
		x = parent.x - w + 2 * border;
	if (y + h > area.y + area.h)
		y = area.y + area.h - h;
}

void Geometry::set_placement(Position p, Geometry &area, int border)
{
	int xpos = std::max(std::max(p.x, area.x) - w / 2, area.x) + 10;
	int ypos = std::max(std::max(p.y, area.y) - h / 2, area.y) + 10;

	int xspace = area.x + area.w - w - border * 2;
	int yspace = area.y + area.h - h - border * 2;

	if (xspace >= area.x)
		x = std::max(std::min(xpos, xspace), area.x);
	else
		x = area.x;

	if (yspace >= area.y)
		y = std::max(std::min(ypos, yspace), area.y);
	else 
		y = area.y;
}

void Geometry::set_user_placement(Geometry &area, int border)
{
	if (x >= area.w)
		x = area.w - border - 1;
	if (x + w + border <= 0)
		x = -(w - border - 1);
	if (y >= area.h)
		y = area.h - border - 1;
	if (y + h + border <= 0)
		y = -(h - border - 1);
}

void Geometry::adjust_for_maximized(Geometry &area, int border)
{
	if (x + w + (border * 2) == area.w)
		w += border * 2;
	if (y + h + (border * 2) == area.h)
		h += border * 2;
}

void Geometry::move(long direction, Geometry &area, int border)
{
	int amt = conf::moveamount;

	if (direction & Direction::West)
		x -= amt;
	if (direction & Direction::East)
		x += amt;
	if (direction & Direction::North)
		y -= amt;
	if (direction & Direction::South)
		y += amt;

	if (x < -(w - border - 1))
		x = -(w - border - 1);
	if (x > (area.w - border - 1))
		x = area.w - border - 1;
	if (y < -(h - border - 1))
		y = -(h - border - 1);
	if (y > (area.h - border - 1))
		y = area.h - border - 1;
}

void Geometry::resize(long direction, SizeHints &hints, int border)
{
	int amt = 1;
	if (!(hints.flags & PResizeInc))
		amt = conf::moveamount;

	int mx = 0, my = 0;

	if (direction & Direction::West)
		mx = -amt;
	if (direction & Direction::East)
		mx = amt;
	if (direction & Direction::North)
		my = -amt;
	if (direction & Direction::South)
		my = amt;

	if ((w += mx * hints.incw) < hints.minw)
		w = hints.minw;
	if ((h += my * hints.inch) < hints.minh)
		h = hints.minh;
	if (x + w + border - 1 < 0)
		x = -(w + border - 1);
	if (y + h + border - 1 < 0)
		y = -(h + border - 1);
}

void Geometry::warp_to_edge(long direction, Geometry &area, int border)
{
	if (direction & Direction::West)
		x = area.x;
	if (direction & Direction::East)
		x = area.x + area.w - w - border;
	if (direction & Direction::North)
		y = area.y;
	if (direction & Direction::South)
		y = area.y + area.h - h - border;
}

void Geometry::snap_to_edge(Geometry &area)
{
	int leftsnap = 0;
	int rightsnap = 0;
	int topsnap = 0;
	int bottomsnap = 0;

	if (abs(x - area.x) <= conf::snapdist) 
		leftsnap = area.x - x;
	if (abs(y - area.y) <= conf::snapdist) 
		topsnap = area.y - y;

	if (abs(x + w - area.x - area.w) <= conf::snapdist) 
		rightsnap = area.x + area.w  - x  - w;
	if (abs(y + h - area.y - area.h) <= conf::snapdist) 
		bottomsnap = area.y + area.h  - y  - h;

	if ((rightsnap) && (leftsnap))
		if (abs(leftsnap) < abs(rightsnap))
			x += leftsnap;
		else
			x += rightsnap;
	else if (leftsnap)
			x += leftsnap;
	else if (rightsnap)
			x += rightsnap;

	if ((topsnap) && (bottomsnap))
		if (abs(topsnap) < abs(bottomsnap))
			y += topsnap;
		else
			y += bottomsnap;
	else if (topsnap)
			y += topsnap;
	else if (bottomsnap)
			y += bottomsnap;
}

void Geometry::apply_border_gap(BorderGap &b) 
{
	x += b.left;
	y += b.top;
	w -= (b.left + b.right);
	h -= (b.top + b.bottom);
}

// Apply size hints to window geometry when resizing with pointer
void Geometry::apply_size_hints(SizeHints &hints)
{
	bool baseismin = (hints.basew == hints.minw) 
			&& (hints.baseh == hints.minh);

	// temporarily remove base dimensions, ICCCM 4.1.2.3
	if (!baseismin) {
		w -= hints.basew;
		h -= hints.baseh;
	}

	// adjust for aspect limits
	if (hints.mina && hints.maxa) {
		if (hints.maxa < (float)w / h)
			w = h * hints.maxa;
		else if (hints.mina < (float)h / w)
			h = w * hints.mina;
	}

	// remove base dimensions for increment
	if (baseismin) {
		w -= hints.basew;
		h -= hints.baseh;
	}

	// adjust for increment value
	w -= w % hints.incw;
	h -= h % hints.inch;

	// restore base dimensions
	w += hints.basew;
	h += hints.baseh;

	// adjust for min width/height
	w = std::max(w, hints.minw);
	h = std::max(h, hints.minh);

	// adjust for max width/height
	if (hints.maxw)
		w = std::min(w, hints.maxw);
	if (hints.maxh)
		h = std::min(h, hints.maxh);
}

Viewport::Viewport(int id, int x, int y, int w, int h, BorderGap &b)
{
	num = id;
	view.x = x; 
	view.y = y; 
	view.w = w; 
	view.h = h;
	work = view;
	work.apply_border_gap(b);
}

Viewport::Viewport(int id, Geometry &g, BorderGap &b)
{
	num = id;
	view = g;
	work = g;
	work.apply_border_gap(b);
}

bool Viewport::contains(Position p)
{
	return ((p.x >= view.x) && (p.x < (view.x + view.w)) &&
		(p.y >= view.y) && (p.y < (view.y + view.h)));
}
	
PropWindow::PropWindow(XScreen *screen, Window parent)
{
	m_font = screen->get_menu_font();
	m_color = screen->get_color(Color::MenuText);
	m_pixel = screen->get_pixel(Color::MenuBackground);

	m_window = XCreateSimpleWindow(wm::display, parent, 0, 0, 1, 1, 0, m_pixel, m_pixel);
	m_xftdraw = XftDrawCreate(wm::display, m_window, screen->get_visual(), 
					screen->get_colormap());
	XMapWindow(wm::display, m_window);
}

PropWindow::~PropWindow()
{
	XftDrawDestroy(m_xftdraw);
	XDestroyWindow(wm::display, m_window);
}

void PropWindow::draw(std::string &text, int x, int y)
{
	XGlyphInfo	extents;
	int 		len = text.size();

	XftTextExtentsUtf8(wm::display, m_font, (const FcChar8*)text.c_str(), len, &extents);
	XMoveResizeWindow(wm::display, m_window, x - extents.width/2, y,
				extents.xOff, m_font->height);
	XClearWindow(wm::display, m_window);
	XftDrawStringUtf8(m_xftdraw, m_color, m_font, 0, m_font->ascent + 1, 
				(const FcChar8*)text.c_str(), len);
}
