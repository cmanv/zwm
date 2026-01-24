// zwm - a minimal stacking/tiling window manager for X11
//
// Copyright (c) 2026 cmanv
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
#include <string>
#include "config.h"
#include "wmcore.h"
#include "xscreen.h"
#include "xwinprop.h"

XWinProp::XWinProp(XScreen *screen, Window parent)
{
	unsigned long pixel;
	m_font = screen->get_menu_font();
	m_color = screen->get_color(Color::MenuItemText);
	pixel = screen->get_pixel(Color::MenuBackground);
	m_window = XCreateSimpleWindow(wm::display, parent, 0, 0, 1, 1, 0,
					pixel, pixel);
	m_xftdraw = XftDrawCreate(wm::display, m_window, screen->get_visual(),
					screen->get_colormap());
	XMapWindow(wm::display, m_window);
}

XWinProp::~XWinProp()
{
	XftDrawDestroy(m_xftdraw);
	XDestroyWindow(wm::display, m_window);
}

void XWinProp::draw(std::string &text, int x, int y)
{
	XGlyphInfo	extents;
	int 		len = text.size();

	XftTextExtentsUtf8(wm::display, m_font, (const FcChar8*)text.c_str(),
				len, &extents);
	XMoveResizeWindow(wm::display, m_window, x - extents.width/2, y,
				extents.xOff, m_font->height);
	XClearWindow(wm::display, m_window);
	XftDrawStringUtf8(m_xftdraw, m_color, m_font, 0, m_font->ascent + 1,
				(const FcChar8*)text.c_str(), len);
}
