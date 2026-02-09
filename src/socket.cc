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

#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include "socket.h"

namespace socket_in {
	int socket_fd = -1;
	std::string message = "";
}

int socket_in::init(std::string &name)
{
	int res = 0;
	bool unix_socket = false;
	struct addrinfo hint, *result;
	std::filesystem::path socket_name;
	std::filesystem::path socket_path;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;

	unsigned long pos = name.find(":");
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

	socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (socket_fd < 0) {
		std::cerr << "socket: " << std::strerror(errno) << std::endl;
		freeaddrinfo(result);
		return -1;
	}

	if (bind(socket_fd, result->ai_addr, result->ai_addrlen) < 0) {
		std::cerr << "bind: " << std::strerror(errno) << std::endl;
		freeaddrinfo(result);
		return -1;
	}

	if (unix_socket)
		std::filesystem::permissions(socket_name, std::filesystem::perms::owner_all);

	if (listen(socket_fd, 5) < 0) {
		std::cerr << "listen failed on socket [" << name << "]!\n";
	}

	freeaddrinfo(result);
	return socket_fd;
}

const std::string& socket_in::get_message()
{
	message = "";
	int fd2 = accept(socket_fd, NULL, NULL);
	if (fd2 < 0) {
		std::cerr << "error on accept" << std::endl;
		close(fd2);
		return message;
	}
	char buffer[1024];
	bzero(buffer, 1024);
	if (read(fd2, buffer, 1023) < 0) {
		std::cerr << "error on read" << std::endl;
		close(fd2);
		return message;
	}
	close(fd2);
	message = std::regex_replace(std::string(buffer), std::regex("\\r\\n|\\r|\\n"), "");
	return message;
}

namespace socket_out {
	struct addrinfo *address;
	bool valid_addr = false;
}

void socket_out::init(std::string &socket_name)
{
	int rc = -1;
	struct addrinfo hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;

	if (valid_addr) freeaddrinfo(address);
	unsigned long pos = socket_name.find(":");
	if (pos != socket_name.npos) {
		std::string hostname = socket_name.substr(0, pos);
		std::string port = socket_name.substr(pos+1);
		hint.ai_family = AF_UNSPEC;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_flags = AI_ADDRCONFIG;
		rc = getaddrinfo(hostname.c_str(), port.c_str(), &hint, &address);
	} else {
		hint.ai_family = PF_LOCAL;
		rc = getaddrinfo(socket_name.c_str(), NULL, &hint, &address);
	}
	if (rc) {
		std::cerr << "getaddrinfo error on socket [" << socket_name << "}: "
			<< gai_strerror(rc) << "\n";
	} else {
		valid_addr = true;
	}
}

bool socket_out::defined()
{
	return valid_addr;
}

void socket_out::clear()
{
	if (valid_addr) freeaddrinfo(address);
	valid_addr = false;
}

int socket_out::send(std::string &message)
{
	if (!valid_addr) return 0;

	int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	if (fd < 0) {
		std::cerr << "Cannot create socket!\n";
		return -1;
	}

	if (connect(fd, address->ai_addr, address->ai_addrlen) < 0) {
		std::cerr << "Cannot connect to message socket!\n";
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
