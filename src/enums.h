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

#ifndef _ENUMS_H_
#define _ENUMS_H_
enum _ewmhints {
	_NET_SUPPORTED,
	_NET_SUPPORTING_WM_CHECK,
	_NET_ACTIVE_WINDOW,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_CURRENT_DESKTOP,
	_NET_DESKTOP_VIEWPORT,
	_NET_DESKTOP_GEOMETRY,
	_NET_VIRTUAL_ROOTS,
	_NET_SHOWING_DESKTOP,
	_NET_DESKTOP_NAMES,
	_NET_WORKAREA,
	_NET_WM_NAME,
	_NET_WM_DESKTOP,
	_NET_CLOSE_WINDOW,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_NET_WM_STATE,
	_NET_WM_STATE_STICKY,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_DEMANDS_ATTENTION,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	NUM_EWMHINTS
};

enum _wmhints {
	WM_STATE,
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_TAKE_FOCUS,
	WM_CHANGE_STATE,
	_MOTIF_WM_HINTS,
	UTF8_STRING,
	NUM_WMHINTS
};

enum _actions {
	_NET_WM_STATE_REMOVE,
	_NET_WM_STATE_ADD,
	_NET_WM_STATE_TOGGLE
};

enum State {
	Active		= 0x000001,
	Hidden		= 0x000002,
	Sticky		= 0x000004,
	Urgent		= 0x000008,
	Frozen		= 0x000010,
	SkipPager	= 0x000020,
	SkipTaskbar	= 0x000040,
	Input		= 0x000080,
	FullScreen	= 0x000100,
	HMaximized	= 0x000200,
	VMaximized	= 0x000400,
	Tiled		= 0x000800,
	NoTile		= 0x001000,
	NoResize	= 0x002000,
	NoBorder	= 0x004000,
	WMDeleteWindow	= 0x010000,
	WMTakeFocus	= 0x020000,
	Maximized	= HMaximized|VMaximized,
	Ignored		= SkipPager|SkipTaskbar,
	SkipCycle	= Hidden|Ignored,
	Docked		= Sticky|Frozen|Ignored|NoBorder,
};

enum Mode {
	Stacked		= 0x01,
	Monocle		= 0x02,
	HTiled		= 0x04,
	VTiled		= 0x08,
	Grid		= 0x10,
	Tiling		= Monocle|HTiled|VTiled|Grid,
	MasterSlave	= VTiled|HTiled,
	Swapable	= VTiled|HTiled|Grid,
};

enum Motif {
	HintElements	= 5L,
	HintFunctions	= (1L << 0),
	HintDecorations	= (1L << 1),
	HintInputMode	= (1L << 2),
	HintStatus	= (1L << 3),
	DecorAll	= (1L << 0),
	DecorBorder	= (1L << 1),
	DecorResizeh	= (1L << 2),
	DecorTitle	= (1L << 3),
	DecorMenu	= (1L << 4),
	DecorMinimize	= (1L << 5),
	DecorMaximize	= (1L << 6),
};

enum Color {
	WindowBorderActive,
	WindowBorderInactive,
	WindowBorderUrgent,
	MenuBackground,
	MenuBorder,
	MenuHighlight,
	MenuItemText,
	MenuItemTextSelected,
	MenuTitle,
	MenuTitleBackground,
	NumColors
};

enum Pointer {
	ShapeNormal,
	ShapeMove,
	ShapeNorth,
	ShapeEast,
	ShapeSouth,
	ShapeWest,
	ShapeNE,
	ShapeSE,
	ShapeSW,
	ShapeNW,
	NumShapes
};

enum ProgramStatus {
	IsRunning,
	IsQuitting,
	IsRestarting
};

enum Direction {
	Pointer		= 0x00L,
	North		= 0x01L,
	South		= 0x02L,
	West		= 0x04L,
	East		= 0x08L,
	NorthWest	= North|West,
	NorthEast	= North|East,
	SouthWest	= South|West,
	SouthEast	= South|East,
};

enum class Coordinates {
	Root,
	Window,
};

enum class Context {
	Launcher,
	FuncCall,
	Root,
	Window,
};

enum class EventType {
	Key,
	Button
};

enum class MenuType {
	Client,
	Desktop,
	Launcher
};
#endif // _ENUMS_H_
