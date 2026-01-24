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

#include <getopt.h>
#include <sys/wait.h>
#include <cerrno>
#include <clocale>
#include <csignal>
#include <iostream>
#include <string>
#include "version.h"
#include "config.h"
#include "wmcore.h"

static std::string appname(APP_NAME);
static std::string version(APP_VERSION);
static void	 show_version(void);
static void	 usage(int);
static void	 signal_handler(int);

int main(int argc, char **argv)
{
	int	 ch, parse_only = 0;

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		std::cerr << "Warning: locale is not supported";

	wm::set_param_restart(argc, argv);

	while ((ch = getopt(argc, argv, "D:c:m:t:hdpv")) != -1) {
		switch (ch) {
		case 'D':
			wm::displayname = optarg;
			break;
		case 'c':
			conf::user_config = optarg;
			break;
		case 'm':
			conf::message_socket = optarg;
			break;
		case 't':
			conf::default_theme = optarg;
			break;
		case 'h':
			usage(0);
			break;
		case 'd':
			conf::debug++;
			break;
		case 'p':
			parse_only = 1;
			break;
		case 'v':
			show_version();
			break;
		default:
			usage(1);
		}
	}

	if (signal(SIGCHLD, signal_handler) == SIG_ERR ||
	    signal(SIGHUP, signal_handler) == SIG_ERR ||
	    signal(SIGINT, signal_handler) == SIG_ERR ||
	    signal(SIGTERM, signal_handler) == SIG_ERR) {
		std::cerr << "signal: " << std::strerror(errno) << std::endl;
		return 1;
	}

	conf::init();
	if (parse_only)
		return 0;

	wm::run();
	return 0;
}

static void signal_handler(int sig)
{
	pid_t	 pid;
	int	 save_errno = errno, status;

	switch (sig) {
	case SIGCHLD:
		// Collect dead children.
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0 ||
		    (pid < 0 && errno == EINTR))
			;
		break;
	case SIGHUP:
		wm::status = IsRestarting;
		break;
	case SIGINT:
	case SIGTERM:
		wm::status = IsQuitting;
		break;
	}

	errno = save_errno;
}

static void show_version(void)
{
	std::cout << appname << " version " << version << std::endl;
	exit(0);
}

static void usage(int rc)
{
	std::cerr << "Usage: " << appname << " [-X display] [-c filename] [-hdpv]\n";
	std::cerr << "  -X display 	: Name of X display.\n";
	std::cerr << "  -c filename	: Path of configuration file.\n";
	std::cerr << "  -m socket	: Path of message socket.\n";
	std::cerr << "  -h 		: Show this help and exit.\n";
	std::cerr << "  -d 		: Run in debug mode. Repeat to increase verbosity.\n";
	std::cerr << "  -p 		: Parse configuration file and exit.\n";
	std::cerr << "  -v 		: Show version and exit.\n";
	exit(rc);
}
