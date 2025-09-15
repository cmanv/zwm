% ZWM(1) zwm version alpha16 | zwm user's manual
% cmanv
% September 2025

# NAME

zwm — a simple stacking / tiling window manager for X11

# SYNOPSIS

**zwm** \[-D display\] \[-c configfile\] \[-m socket\] \[-t theme\] \[-hdpv\]

# DESCRIPTION

**zwm** is an hybrid stacking/tiling window manager for X11. It supports a stacking
mode and 3 tiling modes. It features a number of configurable window manager
functions, workspaces and a configurable menu.
It is also able to send and receive messages through sockets.

# COMMAND LINE OPTIONS

**-D** _display_

> Use _display_ as the default X display.

**-c** _configfile_

> Use _configfile_ as the configuration file.

**-m** _socket_

> Soecify a socket on which the window manager will send some status messages. This
overrides any value defined in the configuration file. If this is unset no messages
are sent.

**-t** _theme_

> Use _theme_ (dark/light) at startup. _light_ is the default.

**-h**

> Print brief usage information.

**-d**

> Send debug information to the standard output. It can be used multiple times
> to increase verbosity.

**-p**

> Parse configuration file and exit.

**-v**

> Print the current version number.

# CONFIGURATION OPTIONS

This section describes all options that can be set in the configuration file.

## GENERAL WINDOW MANAGER OPTIONS

* **color** _element color1 color2_

> Sets the color of the UI elements. _color1_ applies in light mode
> and _color2_ applies in dark mode.

> The elements can be:

> _menu-background_, _menu-border_, _menu-highlight_,
> _menu-item-text_, _menu-item-text-selected_, _menu-title_,
> _menu-title-background_, _window-border-active_,
> _window-border-inactive_, _window-border-urgent_

- **debug-level** _level_

> A non zero value causes the window manager to print debug information
> on the standard output. Increasing its value increases verbosity. (default: 0)

- **desktop-defaults** _number_ _name_ _mode_ _proportion_

> Define the default name and tiling mode of the desktop_.

> * _number_ is the desktop number from 1 to 10.
> * _name_ is a string to identify the desktop.
> * _mode_ can be any of:

> > * _Stacked_: Windows are stacked and can be moved/resized by the user.
> > * _Monocle_: Only one window (maximized) is visible at a time.
> > * _VTiled_: Master / slaves with the slaves tiled vertically on the right.
> > * _HTiled_: Master / slaves with the slaves tiled horizontally on the bottom.
> > * _RxC_: Windows are arranged in a grid of _R_ rows and _C_ columns.
> > The number of rows and columns can be anywhere from 1 to 9.

> * _proportion_ is the proportion of the screeen used by the master window. (HTiled/VTiled modes only)

- **desktop-modes** _mode1_,_mode2,..

> Define a comma separated list of active desktop modes. The order is used
> to rotate mode with the functions _desktop-mode-next_ and _desktop-mode-prev_.

> Default is : Stacked,Monocle,VTiled,HTiled

- **message-socket** _[host:port|path]_

> Specifies a TCP socket or UNIX domain socket to which the window manager will
> connect to send IPC messages.This can be overriden by a command line argument.
> (default is unset)

- **menu-font** _font_

> Sets the font of the text in menus. (default: "Mono:size=10")

- **menu-client-label** _text_

> Sets the label of the client menu. (default is "X Clients")

- **menu-desktop-label** _text_

> Sets the label of the active desktops menu. (default is "Active desktops")

- **menu-launcher-label** _text_

> Sets the label of the launcher menu. (default is "Launchers").
> To define the launcher menu, see the _MENU DEFINITIONS_ section.

- **screen-border-gap** _top bottom left right_

> Reserves space at the edges of the screen. This space will not
> be used for tiling windows by the window manager. (default: 1 1 1 1)

- **shutdown-script** _path_

> Defines a script that is to be run when the window manager terminates.
> (default is unset)

- **startup-script** _path_

> Defines a script that is to be run when the window manager starts.
> (default is unset)

- **terminal** _path_

> Define the default terminal program. (default is "xterm")

- **window-stacked-border** _width_

> Specifies the border width of stacked windows. (default: 7)

- **window-tiled-border** _width_

> Specifies the border width of tiled windows. (default: 2)

## PER APPLICATION OPTIONS

These are options to set the default desktop and default states of an application
based on its _instance_/_class_ properties.

- **app-default-desktop** _instance:class number_

> Use this configuration option to specify that an application with class _instance:class_
> is to open on the desktop _number_.

- **app-default-state** _instance:class_ _state1_ [,_state2_ ..,_stateN_]

> Set the default state of an application with instance/class _instance:class_.
> The applicable states are:

> - _docked_: Equivalent to _frozen_,_sticky_,_ignore_,_noborder_. Any client with the property _\_NET\_WM\_WINDOW\_TYPE\_DOCK_ will have this state set.

> - _frozen_: The window is locked at its current position.

> - _ignored_: Do not add the window to the task list or the window list.

> - _noborder_: The window has no border (_stacked_ windows only).

> - _noresize_: The window cannot be resized (_stacked_ windows only).

> - _notile_: The window is never tiled.

> - _sticky_: The window appears on all desktops.

## MENU DEFINITIONS

These options allows to define a menu hierarchy that can be activated by
the _menu-launcher_ function. The label of the top level menu must match
the _menu-launcher-label_ option. (Default: "Launchers")

Any menu can contains a list of commands and submenus. A menu
definition starts by _menu-start_ statement, followed by a list
of _menu-item_ statements, and ends with a _menu-end_ stetement.

> _menu-start_ _text_

> _menu-item_ _text_ _function_ \[_arg_\]

> ,,,

> _menu-end_

- **menu-start** _text_

> Starts the definition of a menu with label _text_. This must be followed by a
series of _menu-item_ lines and end with a _menu-end_ line.

- **menu-item**  _text function \[arg\]_

> Define an item in a menu. The _text_ is the label of the menu item.
> The _function_ and _arg_ are usually one of these:

> - _exec_ _path_: Execute the program at the specified _path_.

> - _menu_  _text_: Open a menu as a submenu. _text_ is the label of a defined menu.

> - _quit_: Terminates the window manager application.

> - _restart_: Restarts the window manager application. Any changes in the configuration file will be applied.

* **menu-end**

> Ends the definition of the menu.

## BINDING OPTIONS

These options allow to bind or unbind a key/buttpn shortcut to a window manager function.
A binding consists of a set of modifier keys and a regular key or button mouse.
Any combination of these modifiers are allowed:

> - **C** for the Control key
> - **M** for the Alt key
> - **4** for the Super (Windows) key
> - **S** for the Shift key

The bindings options are:

- **bind-key** _modifiers-key function_

> Bind a key pressed with modifiers to a window manager function.

- **bind-mouse** _modifiers-button function_

> Bind a mouse button clicked with modifiers to a window manager function.

- **unbind-key** _modifiers-key_

> Unassigns a particular modifiers/key combination. The special form _unbind\_key all_ clears
> all previously defined key bindings.

- **unbind-mouse** _modifiers-button_

> Unassigns a particular modifiers/button combination. The special form _unbind\_mouse all_ clears
> all previously defined mouse bindings.

# WINDOW MANAGER FUNCTIONS
This sections list all window manager functions that can be accessed through a key
or mouse binding.

- **desktop-close**: Close all windows on the desktop.
- **desktop-hide**: Hide all windows on the desktop.
- **desktop-client-menu**: Show the list of X11 clients. (keyboard driven).
- **desktop-desktop-menu**: Show the list of active desktops. (keyboard driven).
- **desktop-launcher-menu**: Show the launcher menu. (keyboard driven).
- **desktop-mode-{_number_}**: Switch to mode _number_ (1-9).
The _number_ refers to the order of appearance of the mode in **_desktop_modes_**.
- **desktop-mode-next**: Switch the desktop to the next tiling mode in the order defined by _desktop-modes_.
- **desktop-mode-prev**: Switch the desktop to the previous tiling mode in ther order defined by _desktop-modes_.
- **desktop-set-dark-theme**: Switch the window manager colors to the dark theme.
- **desktop-set-light-theme**: Switch the window manager colors to the light theme.
- **desktop-switch-{_number_}**: Go to desktop _number_.
- **desktop-switch-last**: Move back to the last used desktop.
- **desktop-switch-next**: Go to the next active desktop. Last desktop wraps to first.
- **desktop-switch-prev**: Go to the previous active desktop. First desktop wraps to last.
- **desktop-window-focus-next**: Move the focus to the next window. (All modes except Monocle)
- **desktop-window-focus-prev**: Move the focus to the previous window. (All modes except Monocle)
- **desktop-window-master-decr**: Decrease the proportion of the screen occupied by the master window. (HTiled, VTIled)
- **desktop-window-master-incr**: Increase the proportion of the screen occupied by the master window. (HTiled, VTIled)
- **desktop-window-rotate-next**: Rotate the position of the windows counterclockwise while keeping the focus at the same position. (All tiled modes)
- **desktop-window-rotate-prev**: Rotate the position of the windows clockwise while keeping the focus at the same position. (All tiled modes)
- **desktop-window-swap-next**: Swap the position of the active window and the next window. (HTiled, VTiled)
- **desktop-window-swap-prev**: Swap the position of the active window and the previous window. (HTiled, VTiled)
- **exec**  _path_: Execute a program defined by _path_.
- **client-menu**: Shows the list of X clients (pointer driver).
- **desktop-menu**: Show the list of active desktops (pointer driven).
- **launcher-menu**: Show the launcher menu. (pointer driven).
- **quit**: Terminate the window manager.
- **restart**: Restart the window manager.
- **terminal**: Open the default terminal.
- **window-close**: Close the current window.
- **window-hide**: Hide the current window.
- **window-lower**: Lower the position of the current window in the stack. (_stacked_ windows only).
- **window-move**: Move the current window with the pointer. (_stacked_ windows only)
- **window-move-down**: Move the current window toward to the bottom of the screen. (_stacked_ windows only)
- **window-move-left**: Move the current window toward to the left of the screen. (_stacked_ windows only)
- **window-move-right**: Move the current window toward to the right of the screen. (_stacked_ windows only)
- **window-move-up**: Move the current window toward to the top of the screen. (_stacked_ windows only)
- **window-move-to-desktop-{_number_}**: Move the current window to the desktop _number_.
- **window-raise**: Moves the current window to the top the stack. (_stacked_ windows only).
- **window-resize**: Resize the current window with the pointer.  (_stacked_ windows only)
- **window-resize-down**: Resize the current window toward the bottom.  (_stacked_ windows only)
- **window-resize-left**: Resize the current window toward the left.  (_stacked_ window only)
- **window-resize-right**: Resize the current window toward the right.  (_stacked_ window only)
- **window-resize-up**: Resize the current window toward the top.  (_stacked_ windows only)
- **window-snap-down**: Snap the current window to the bottom  edge of the screen. (_stacked_ windows only)
- **window-snap-left**: Snap the current window to the left edge of the screen. (_stacked_ windows only)
- **window-snap-up**: Snap the current window to the top edge of the screen. (_stacked_ windows only)
- **window-snap-right**: Snap the current window to the right edge of the screen. (_stacked_ windows only)
- **window-toggle-fullscreen**: Toggle the fullscreen state of the current window.
- **window-toggle-sticky**: Toggle the _sticky_ state of the current window. (_stacked_ windows only)
- **window-toggle-tiled**: Toggle the _tiled_/_stacked_ state of the current window. Switching off the _stacked_ state also switch off the _sticky_ state.

# DEFAULT BINDINGS
This sections list all key and mouse bindings defined by default.

## Key bindings

- **C-Return**	:	_terminal_
- **CM-r**	:	_restart_
- **CM-q**	:	_quit_
- **M-1**	:	_desktop-mode-1_
- **M-2**	:	_desktop-mode-2_
- **M-3**	:	_desktop-mode-3_
- **M-4**	:	_desktop-mode-4_
- **M-Up**	:	_desktop-mode-next_
- **M-Down**	:	_desktop-mode-prev_
- **CM-1**	:	_desktop-switch-1_
- **CM-2**	:	_desktop-switch-2_
- **CM-3**	:	_desktop-switch-3_
- **CM-4**	:	_desktop-switch-4_
- **CM-5**	:	_desktop-switch-5_
- **CM-6**	:	_desktop-switch-6_
- **CM-7**	:	_desktop-switch-7_
- **CM-8**	:	_desktop-switch-8_
- **CM-9**	:	_desktop-switch-9_
- **CM-0**	:	_desktop-switch-10_
- **CM-Right**	:	_desktop-switch-next_
- **CM-Left**	:	_desktop-switch-prev_
- **M-Tab**	:	_desktop-window-focus-next_
- **SM-Tab**	:	_desktop-window-focus-prev_
- **M-greater** :	_desktop-window-master-incr_
- **M-less**	:	_desktop-window-master-decr_
- **SM-Right**	:	_desktop-window-rotate-next_
- **SM-Left**	:	_desktop-window-rotate-prev_
- **M-Right**	:	_desktop-window-swap-next_
- **M-Left**	:	_desktop-window-swap-prev_
- **SM-1**	:	_window-move-to-desktop-1_
- **SM-2**	:	_window-move-to-desktop-2_
- **SM-3**	:	_window-move-to-desktop-3_
- **SM-4**	:	_window-move-to-desktop-4_
- **SM-5**	:	_window-move-to-desktop-5_
- **SM-6**	:	_window-move-to-desktop-6_
- **SM-7**	:	_window-move-to-desktop-7_
- **SM-8**	:	_window-move-to-desktop-8_
- **SM-9**	:	_window-move-to-desktop-9_
- **SM-0**	:	_window-move-to-desktop-10_
- **M-h**	:	_window-move-left_
- **M-l**	:	_window-move-right_
- **M-j**	:	_window-move-up_
- **M-k**	:	_window-move-down_
- **SM-h**	:	_window-resize-left_
- **SM-l**	:	_window-resize-right_
- **SM-j**	:	_window-resize-up_
- **SM-k**	:	_window-resize-down_
- **CM-h**	:	_window-snap-left_
- **CM-l**	:	_window-snap-right_
- **CM-j**	:	_window-snap-up_
- **CM-k**	:	_window-snap-down_
- **SM-f**	:	_window-toggle-fullscreen_
- **SM-s**	:	_window-toggle-sticky_
- **SM-t**	:	_window-toggle-tiled_
- **SM-i**	:	_window-hide_
- **SM-x**	:	_window-close_

## Mouse buttons bindings

- **1**	:	_client-menu_
- **2**	:	_desktop-menu_
- **3**	:	_launcher-menu_
- **M+1**:	_window-move_
- **M+3**:	_window-resize_
- **M+4**:	_window-lower_
- **M+5**:	_window-raise_

# SOCKETS
This section describes the use of sockets by the window manager.

## Command socket:

Commands can be sent programmatically to the window manager through a UNIX socket.
This socket is located at $XDG\_CACHE\_HOME/zwm/socket

All window manager desktop functions are accepted. These are the functions starting with "desktop-".

The accepted format of the command is: "_screen_:_function_", where:

* _screen_ is the applicable X screen number
* _function_ the name of the window manager function.

Any message not complying with the format will be ignored.

## Message socket:

The window manager can send status messages to a UNIX socket. This can be useful for some programs such as status bars.

This is the list of messages that can be sent by the window manager:

- _window\_active="current title of active windowa"_

> Sent when there is a change of active window title.

- _no\_window\_active=_

> Sent when there is no longer an active window.on the desktop.

- _desktop\_name="desktop name"_

> Sent when the active desktop has changed.

- _desktop\_mode="desktop mode"_

> Sent when the desktop mode has changed.

- _desktop\_list="space separated list of desktops numbers"_

> Sent where there is a change in the list of active desktops.
> The active desktop number is prepended by a '*' in the list.

To activate this feature, set _message-socket_ to the path of the destination socket in the configuration file. Alternatively, use the _-m_ command line option to specify its value. If used, the command line option overrides the value defined in the configuration file.

# FILES

If not specified at the command line, the configuration file _~/.config/zwm/config_ is read at startup.

# BUGS

See GitHub Issues: <https://github.com/cmanv/zwm/issues>
