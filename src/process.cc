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

#include <sys/wait.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "process.h"

namespace process {
	static void execute(std::string &);
}

void process::exec(std::string &path)
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

void process::spawn(std::string &path)
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

static void process::execute(std::string &path)
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
