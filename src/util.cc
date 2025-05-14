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

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "winmgr.h"
#include "desktop.h"
#include "xclient.h"
#include "xscreen.h"
#include "util.h"

namespace util {
	static void execute(std::string &);
	struct addrinfo *sock_addr;
	bool valid_addr = true;
}

Position xutil::get_pointer_pos(Window window)
{
	Window		 root, child;
	int		 rx, ry, wx, wy;
	unsigned int	 mask;

	XQueryPointer(wm::display, window, &root, &child, &rx,  &ry, &wx, &wy, &mask);	
	return Position(wx, wy);
}

void xutil::set_pointer_pos(Window window, Position p)
{
	XWarpPointer(wm::display, None, window, 0, 0, 0, 0, p.x, p.y);
}

std::string util::gettime() 
{
	auto t = std::chrono::system_clock::now();
	std::time_t now = std::chrono::system_clock::to_time_t(t);
	std::string s(8, '\0');
	std::strftime(&s[0], s.size(), "%H:%M:%S", std::localtime(&now));
	return s;
}

void util::exec_child(std::string &path)
{
	pid_t pid = fork();
	switch(pid) {
	case 0:
		closefrom(3);
		execute(path);
		exit(1);
	case -1:
		std::cerr << "fork\n";
	default:
		break;
	}

	int status = 0;
	waitpid(pid, &status, 0);
}

void util::spawn_process(std::string &path)
{
	switch (fork()) {
	case 0:
		closefrom(3);
		execute(path);
		exit(1);
	case -1:
		std::cerr << "fork\n";
	default:
		break;
	}
}

static void util::execute(std::string &path)
{
	std::vector<std::string> arglist;
	std::string word;

	// Split the command into a array of space or quote delimited strings 	
	std::istringstream iss(path);
	while (iss >> std::quoted(word))
		arglist.push_back(word);

	// Setup an array of pointers to these strings.
	int n = 0;
	std::vector<char *> argv(arglist.size()+1);
	for (std::string &s : arglist)
		argv[n++] = (char *)s.c_str();
	argv[n] = NULL;

	// Execute the command
	setsid();
	execvp((char *)argv[0], (char **)argv.data());
	std::cerr << "Error exec: " << path << std::endl;
}

int util::init_server_socket(std::string &name)
{
	int res = 0, optval = 1;
	bool unix_socket = false;
	struct addrinfo hint, *result;
	std::filesystem::path socket_name;
	std::filesystem::path socket_path;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;

	int pos = name.find(":");
	if (pos != name.npos) {
		std::string hostname = name.substr(0, pos); 
		std::string port = name.substr(pos+1); 
		hint.ai_family = AF_UNSPEC;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_ADDRCONFIG;	
		res = getaddrinfo(hostname.c_str(), port.c_str(), &hint, &result);
	} else {
		unix_socket = true;
		hint.ai_family = PF_LOCAL;
		res = getaddrinfo(name.c_str(), NULL, &hint, &result);
	}
	if (res) {
		std::cerr << "getaddrinfo error on socket [" << name << "}: " 
			<< gai_strerror(res) << "\n";
		return -1;
	}

	if (unix_socket) {
		socket_name = name;
		socket_path = socket_name.parent_path();
		if (!std::filesystem::exists(socket_path)) {
			std::filesystem::create_directories(socket_path);
		}
		if (std::filesystem::exists(socket_name)) {
			std::filesystem::remove(socket_name);
		}
	}

	int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (fd < 0) {
		std::cerr << "socket: " << std::strerror(errno) << std::endl;
		freeaddrinfo(result);
		return -1;
	}

	if (bind(fd, result->ai_addr, result->ai_addrlen) < 0) {
		std::cerr << "bind: " << std::strerror(errno) << std::endl;
		freeaddrinfo(result);
		return -1;
	}

	if (unix_socket) 
		std::filesystem::permissions(socket_name, std::filesystem::perms::owner_all);

	if (listen(fd, 5) < 0) {
		std::cerr << "listen failed on socket [" << name << "]!\n";
	}

	freeaddrinfo(result);
	return fd;
}

void util::get_message(int fd, std::string &msg)
{
	int fd2 = accept(fd, NULL, NULL);
	if (fd2 < 0) {
		std::cerr << "error on accept" << std::endl;
		close(fd2);
		return;
	}
	char buffer[1024];
	bzero(buffer, 1024);
	if (read(fd2, buffer, 1023) < 0) {
		std::cerr << "error on read" << std::endl;
		close(fd2);
		return;
	}
	close(fd2);
	msg = std::regex_replace(std::string(buffer), std::regex("\\r\\n|\\r|\\n"), "");
	return;
}

void util::init_client_socket(std::string &socket_name)
{
	int rc;
	struct addrinfo hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;

	int pos = socket_name.find(":");
	if (pos != socket_name.npos) {
		std::string hostname = socket_name.substr(0, pos); 
		std::string port = socket_name.substr(pos+1); 
		hint.ai_family = AF_UNSPEC;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_ADDRCONFIG;	
		rc = getaddrinfo(hostname.c_str(), port.c_str(), &hint, &sock_addr);
	} else {
		hint.ai_family = PF_LOCAL;
		rc = getaddrinfo(socket_name.c_str(), NULL, &hint, &sock_addr);
	}
	if (rc) {
		valid_addr = false;
		std::cerr << "getaddrinfo error on socket [" << socket_name << "}: " 
			<< gai_strerror(rc) << "\n";
	}
}

void util::free_client_socket()
{
	if (!valid_addr) return;
	freeaddrinfo(sock_addr);
}

int util::send_message(std::string &message)
{
	if (!valid_addr) return 0;
	
	int fd = socket(sock_addr->ai_family, sock_addr->ai_socktype, sock_addr->ai_protocol);
	if (fd < 0) {
		std::cerr << "Cannot create socket!\n";
		return -1;
	}	

	if (connect(fd, sock_addr->ai_addr, sock_addr->ai_addrlen) < 0) {
		std::cerr << "Cannot connect to server socket!\n";
		close(fd);
		return -1;
	}

	message += '\n';
	int rc = write(fd, message.c_str(), message.length());
	if (rc < 0) {
		std::cerr << "Failed to send message!\n";
	}
	
	close(fd);
	return rc;
}
