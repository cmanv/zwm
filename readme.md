% ZWM(1) zwm version alpha13 | zwm user's manual
% cmanv
% August 2025

# NAME

zwm — a simple stacking / tiling window manager for X11

# SYNOPSIS

**zwm** \[OPTIONS]

# DESCRIPTION

**zwm** is an hybrid stacking/tiling window manager for X11. It supports a stacking
mode and 3 tiling modes. It features a number of configurable keyboards control
functions, workspaces and a configurable menu.
It is also able to send and receive messages through sockets.

# COMMAND LINE OPTIONS

**-c** _filename_

> Use _filename_ as configuration file.

**-d**

> Send debug information to the standard output. It can be used multiple times
> to increase verbosity.

**-D** _display_

> Use _display_ as default X display.

**-m** _message_socket_

> Soecify a socket on which the window manager will send some status messages. This
overrides any value defined in the configuration file. If this is unset no messages
are sent.

**-h**

> Prints brief usage information.

**-p**

> Only parse configuration file and exit.

**-v**

> Prints the current version number.

# CONFIGURATION OPTIONS
This section describe all options that can be set in the configuration files.

## GENERAL OPTIONS

* **screen-border-gap** _top bottom left right_

> Sets reserved spaces at the edges of the screen. This space will not
> be used for tiling windows by the window manager. (default: 1 1 1 1)

* **message-socket** _[hoat:port|path]_

> Specify a TCP socket or UNIX domain socket to which the window manager will
> connect to send IPC messages.This can be overriden by a command line argument. If
> unset (default), then no messages are sent.

* **debug-level** _level_

> A non zero value causes the window manager to print debug information
> on the standard output. Increasing its value increases verbosity. (default: 0)

* **desktop** _num name mode_

> Sets the default name and tiling mode of the desktop_.
> _num_ goes from 1 to 10
> _name_ is a string to identify the desktop.
> _mode_ can be any of: _vtile_, _htile_ or _monocle_. (default: _vtile_)

* **desktop-modes** _mode1_,_mode2,..

> Sets a comma separated list of active desktop modes. The order is used
> to rotate mode with the functions _desktop_mode_next_ and _desktop_mode_prev_
> The list of currently valid modes is:
> > _Stacked_ : Windows are stacked and can be moved/resized by the user.
> > _Monocle_ : Only one window (maximized) is visible at a time.
> > _Vtile_ : Master / slaves with the slaves tiled vertically on the right.
> > _Htile_ : Master / slaves with the slaves tiled horizontally on the bottom.

* **shutdown-script** _path_

> Defines a script that is to be run when the window manager terminates.
> (default is unset)

* **startup-script** _path_

> Defines a script that is to be run when the window manager starts.
> (default is unset)

* **terminal** _path_

> Defines the default terminal program. (default is "xterm")

## WINDOWS OPTIONS

* **window-stacked-border** _width_

> Specifies the border width of stacked windows. (default: 7)

* **window-tiled-border** _width_

> Specifies the border width of tiled windows. (default: 2)

* **color** _element color_

> Sets the color of the windows border.

> > _window-border-active_
> > _window-border-inactive_
> > _window-border-urgent_

## MENU OPTIONS

* **color** _element color_

> Sets the color of various UI elements.

> > _menu-background_
> > _menu-border_
> > _menu-text_
> > _menu-text-selected_
> > _menu-text-highlight_
> > _menu-highlight_
> > _menu-title_
> > _menu-title-background_

* **menu-font** _font_

> Sets the font of the text in menus. (default: "Mono:size=10")

* **menu-start** _text_

> Starts the definition of a menu with title _text_. This must be followed by a
series of _menu-item_ lines and end with a _menu-end_ line.

* **menu-item**  _text \[function \[arg\]\]_

> Define an item in a menu. The _text_ is what appears in the menu. The _function_
> and _arg_ can be any window manager function, but usually one of these:

> > _exec_ _path_

> > > Execute the program at the specified _path_.

> > _menu_  _text_

> > > Open a menu as a submenu. The _text_ is the title of a defined menu.

> > _quit_

> > > Terminates the window manager application.

> > _restart_

> > > Restarts the window manager application. Any changes in the configuration
> > > file will be applied.

* **menu-end**

> Ends the definition of the menu.

* **menu-launcher-title** _text_

> Sets the title of the launcher menu. (default is "Applications").

* **menu-desktop-title** _text_

> Sets the title of the active desktops menu. (default is "Active desktops")

* **menu-client-title** _text_

> Sets the title of the client menu. (default is "Clients")

## APPLICATION OPTIONS

* **default-desktop** _appclass num_

> Use this configuration option to specify that an application with class _appclass_
> is to open on desktop _num_ (1-10).


* **window-state** _appclass_ _state1_ [,_state2_ ..,_stateN_]

> Set the default state of an application with class _appclaas_.
> The applicable states are:

> > _docked_

> > > Equivalent to _frozen_,_sticky_,_ignore_,_noborder_. Any client with the property _\_NET\_WM\_WINDOW\_TYPE\_DOCK_ will have this state set.

> > _frozen_

> > > The window is locked at its current position.

> > _ignored_

> > > Do not add the window to the task list or the window list.

> > _noborder_

> > > The window has no border (_stacked_ windows only).

> > _noresize_

> > > The window cannot be resized (applicable to _stacked_ windows only).

> > _notile_

> > > The window is never tiled.

> > _sticky_

> > > The window appears on all desktops.

## BINDING OPTIONS

* **bind-key** _modifiers-key function_

> Bind a key pressed with modifiers to a window manager function.
Modifiers include:

> > _C_ for the Control key
> > _M_ for the Alt key
> > _4_ for the Super (Windows) key
> > _S_ for the Shift key

* **bind-mouse** _modifiers-button function_

> Bind a mouse button click with modifiers to a window manager function. _C_,_M_'_4_,_S_ are the applicable modifiers.

* **unbind-key** _modifiers-key_

> Unassigns a particular modifiers/key combination. The special form _unbind\_key all_ clears
> all key bindings defined in the window manager.

* **unbind-mouse** _modifiers-button_

> Unassigns a particular modifiers/button combination.

# WINDOW MANAGER FUNCTIONS
This sections list all window manager functions that can be accessed through a key
or mouse binding.

* **terminal**

> Open the default terminal.

* **exec**  _path_

> Execute a program defined by _path_.

* **restart**

> Restart the window manager.

* **quit**

> Terminate the window manager.

* **desktop-close**

> Close all windows on the desktop.

* **desktop-hide**

> Hide all windows on the desktop.

* **desktop-last**

> Move to the last desktop.

* **desktop-master-decr**

> Decrease the proportion of the screen occupied by the master window. (HTiled,VTIled)

* **desktop-master-incr**

> Increase the proportion of the screen occupied by the master window. (HTiled,VTIled)

* **desktop-mode-stacked**

> Switch to the desktop stacked mode. (Stacked)

* **desktop-mode-monocle**

> Switch to the desktop monocle mode. (Monocle)

* **desktop-mode-htile**

> Switch to the master/slave horizontal tiling mode. (HTiled)

* **desktop-mode-vtile**

> Switch to the master/slave vertital tiling mode. (VTile)

* **desktop-mode-next**

> Switch to the next desktop tiling mode in the order defined by _desktop_modes_.

* **desktop-mode-prev**

> Switch to the previous desktop tiling mode in ther order defined by _desktop_modes_..

* **desktop-next**

> Change to the next desktop. Desktop 1 follows desktop 10.

* **desktop-prev**

> Change to the previous desktop. Desktop 10 follows desktop 1.

* **desktop-rotate-next**

> Rotate the position of the tiled windows counterclockwise.

* **desktop-rotate-prev**

> Rotate the position of the tiled windows clockwise.

* **desktop-select-_num_**

> Move to desktop _num_

* **desktop-window-next**

> Move the focus to the next tiled window.

* **desktop-window-prev**

> Move the focus to the previous tiled window.

* **menu-client**

> Shows the list of X clients.

* **menu-desktop**

> Show the list of active desktops.

* **menu-launcher**

> Show the launcher menu as defined by the user.

* **window-close**

> Closes the current window.

* **window-hide**

> Hides the current window.

* **window-lower**

> Lower the position of the current window in the stack. (_stacked_ windows only).

* **window-move**

> Move the current window with the pointer. (_stacked_ windows only)

* **window-move-down**

> Move the current window toward to the bottom of the screen. (_stacked_ windows only)

* **window-move-left**

> Move the current window toward to the left of the screen. (_stacked_ windows only)

* **window-move-right**

> Move the current window toward to the right of the screen. (_stacked_ windows only)

* **window-move-up**

> Move the current window toward to the top of the screen. (_stacked_ windows only)

* **window-move-to-desktop-_num_**

> Moves the current window to the desktop _num_. (1 <= _num_ <= 10)

* **window-raise**

> Moves the current window to the top the stack. (_stacked_ windows only).

* **window-resize**

> Resize the current window with the pointer.  (_stacked_ windows only)

* **window-resize-down**

> Resize the current window toward the bottom.  (_stacked_ windows only)

* **window-resize-left**

> Resize the current window toward the left.  (_stacked_ window only)

* **window-resize-right**

> Resize the current window toward the right.  (_stacked_ window only)

* **window-resize-up**

> Resize the current window toward the top.  (_stacked_ windows only)

* **window-snap-down**

> Snap the current window to the bottom  edge of the screen. (_stacked_ windows only)

* **window-snap-left**

> Snap the current window to the left edge of the screen. (_stacked_ windows only)

* **window-snap-up**

> Snap the current window to the top edge of the screen. (_stacked_ windows only)

* **window-snap-right**

> Snap the current window to the right edge of the screen. (_stacked_ windows only)

* **window-toggle-fullscreen**

> Toggles the fullscreen state of the current window.

* **window-toggle-sticky**

> Toggle the _sticky_ state of the current window.

* **window-toggle-tiled**

> Toggle the _tiled_/_stacked_ state of the current window.

# DEFAULT BINDINGS

## Key bindings

  * **CM-Return**	->	_terminal_
  * **CM-r**		->	_restart_
  * **CM-q**		->	_quit_
  * **M-1**		->	_desktop-select-1_
  * **M-2**		->	_desktop-select-2_
  * **M-3**		->	_desktop-select-3_
  * **M-4**		->	_desktop-select-4_
  * **M-5**		->	_desktop-select-5_
  * **M-6**		->	_desktop-select-6_
  * **M-7**		->	_desktop-select-7_
  * **M-8**		->	_desktop-select-8_
  * **M-9**		->	_desktop-select-9_
  * **M-0**		->	_desktop-select-10_
  * **SM-s**		->	_desktop_mode_stacked_
  * **SM-m**		->	_desktop_mode_monocle_
  * **SM-h**		->	_desktop_mode_htiled_
  * **SM-v**		->	_desktop_mode_vtiled_
  * **SM-Down**		->	_desktop-mode-next_
  * **SM-Up**		->	_desktop-mode-prev_
  * **CM-Right**	->	_desktop-next_
  * **CM-Left**		->	_desktop-prev_
  * **M-Tab**		->	_desktop-rotate-next_
  * **SM-Tab**		->	_desktop-rotate-prec_
  * **M-Right**		->	_desktop-window-next_
  * **M-Left**		->	_desktop-window-prec_
  * **M-Greater**	->	_desktop-master-incr_
  * **M-Less**		->	_desktop-master-decr_

  * **SM-1**		->	_window-move-to-desktop-1_
  * **SM-2**		->	_window-move-to-desktop-2_
  * **SM-3**		->	_window-move-to-desktop-3_
  * **SM-4**		->	_window-move-to-desktop-4_
  * **SM-5**		->	_window-move-to-desktop-5_
  * **SM-6**		->	_window-move-to-desktop-6_
  * **SM-7**		->	_window-move-to-desktop-7_
  * **SM-8**		->	_window-move-to-desktop-8_
  * **SM-9**		->	_window-move-to-desktop-9_
  * **SM-0**		->	_window-move-to-desktop-10_
  * **SM-f**		->	_window-toggle-fullscreen_
  * **SM-s**		->	_window-toggle-sticky_
  * **SM-t**		->	_window-toggle-tiled_
  * **SM-i**		->	_window-hide_
  * **SM-x**		->	_window-close_
  * **M-h**		->	_window-move-left_
  * **M-l**		->	_window-move-right_
  * **M-j**		->	_window-move-up_
  * **M-k**		->	_window-move-down_
  * **SM-h**		->	_window-resize-left_
  * **SM-l**		->	_window-resize-right_
  * **SM-j**		->	_window-resize-up_
  * **SM-k**		->	_window-resize-down_
  * **CM-h**		->	_window-snap-left_
  * **CM-l**		->	_window-snap-right_
  * **CM-j**		->	_window-snap-up_
  * **CM-k**		->	_window-snap-down_

## Mouse buttons bindings

  * **1**	->	_menu-client_
  * **2**	->	_menu-desktop_
  * **3**	->	_menu-launcher_
  * **M+1**	->	_window_move_
  * **M+3**	->	_window_resize_
  * **M+4**	->	_window_lower_
  * **M+5**	->	_window_raise_

# SOCKETS

Command socket:

> Commands can be sent programmatically to the window manager through a UNIX socket.
> The command socket is located at \${XDG\_CACHE\_HOME}/zwm/socket

> All window manager desktop functions are accepted. These are the functions starting with "desktop-".

> The accepted format of the command is: "_screen_:_function_", where:
> >  _screen_ is the applicable X screen number
> >  _function_ the name of the window manager function.

> Any message not complying with the format will be ignored.

Message socket:

> The window manager can send status messages to a UNIX socket. This can be useful for some programs such as status bars.
> This is the list of message that can be sent by the window manager:

> > - Change of active window title.
> > > Format:
> > > "window\_active=_current title of active window_"

> > - Absence of active window.
> > > Format:
> > > "no\_window\_active="

> > - Change of desktop mode.
> > > Format:
> > > "desktop\_mode=_desktop mode letter_"

> > - Change of active desktop list.
> > > Format:
> > > "desktop\_list=_space separated list of desktops numbers_"

> > > The active desktop number is prepended by '*'.

> To activate this feature, set _message-socket_ to the path of the destination socket in the configuration file. Alternatively, use the _-m_ command line option to specify its value. If used, the command line option overrides the value defined in the configuration file.

# FILES

If not specified at the command line, **zwm** read the configuration file _~/.config/zwm/config_

# BUGS

See GitHub Issues: <https://github.com/cmanv/zwm/issues>
