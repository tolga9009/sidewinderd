/*
 * This source file is part of Sidewinder daemon.
 *
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdlib>
#include <csignal>
#include <iostream>

#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>

#include <libconfig.h++>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <systemd/sd-daemon.h>

#include "keyboard.hpp"

/* global variables */
namespace sidewinderd {
	volatile sig_atomic_t run;
};

void sig_handler(int sig) {
	switch (sig) {
		case SIGINT:
			sidewinderd::run = 0;
			break;
		case SIGTERM:
			sidewinderd::run = 0;
			break;
		default:
			std::cout << "Unknown signal received." << std::endl;
	}
}

int create_pid(std::string pid_file) {
	int pid_fd = open(pid_file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (pid_fd < 0) {
		std::cout << "PID file could not be created." << std::endl;
		return -1;
	}

	if (flock(pid_fd, LOCK_EX | LOCK_NB) < 0) {
		std::cout << "Could not lock PID file, another instance is already running. Terminating." << std::endl;
		close(pid_fd);
		return -1;
	}

	return pid_fd;
}

void close_pid(int pid_fd, std::string pid_file) {
	flock(pid_fd, LOCK_UN);
	close(pid_fd);
	unlink(pid_file.c_str());
}

void setup_config(libconfig::Config *config, std::string config_file = "/etc/sidewinderd.conf") {
	try {
		config->readFile(config_file.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
	} catch (const libconfig::ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
	}

	libconfig::Setting &root = config->getRoot();

	if (!root.exists("user")) {
		root.add("user", libconfig::Setting::TypeString) = "nobody";
	}

	if (!root.exists("profile")) {
		root.add("profile", libconfig::Setting::TypeInt) = 1;
	}

	if (!root.exists("capture_delays")) {
		root.add("capture_delays", libconfig::Setting::TypeBoolean) = true;
	}

	if (!root.exists("pid-file")) {
		root.add("pid-file", libconfig::Setting::TypeString) = "/var/run/sidewinderd.pid";
	}

	if (!root.exists("save_profile")) {
		root.add("save_profile", libconfig::Setting::TypeBoolean) = false;
	}

	try {
		config->writeFile(config_file.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while writing file." << std::endl;
	}
}

int main(int argc, char *argv[]) {
	/* signal handling */
	struct sigaction action;

	action.sa_handler = sig_handler;

	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);

	sidewinderd::run = 1;

	/* handling command-line options */
	static struct option long_options[] = {
		{"config", required_argument, 0, 'c'},
		{"daemon", no_argument, 0, 'd'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt, index;
	index = 0;

	std::string config_file;

	while ((opt = getopt_long(argc, argv, ":c:dv", long_options, &index)) != -1) {
		switch (opt) {
			case 'c':
				std::cout << "Option --config" << std::endl;
				config_file = optarg;
				break;
			case 'd':
				/* TODO: add optional old fashioned fork() daemon */
				std::cout << "Option --daemon" << std::endl;
				break;
			case 'v':
				std::cout << "Option --verbose" << std::endl;
				break;
			case ':':
				std::cout << "Missing argument." << std::endl;
				break;
			case '?':
				std::cout << "Unrecognized option." << std::endl;
				break;
			default:
				std::cout << "Unexpected error." << std::endl;
				return EXIT_FAILURE;
		}
	}

	/* reading config file */
	libconfig::Config config;

	if (config_file.empty()) {
		setup_config(&config);
	} else {
		setup_config(&config, config_file);
	}

	/* TODO: check values for validity and throw exceptions, if invalid */
	std::string user = config.lookup("user");
	int profile = config.lookup("profile");

	if (profile > 3) {
		std::cout << "Invalid profile" << std::endl;
		sidewinderd::run = 0;
	}

	bool capture_delays = config.lookup("capture_delays");
	std::string pid_file = config.lookup("pid-file");
	bool save_profile = config.lookup("save_profile");

	/* TODO: change directory to getenv("HOME")/<user>/.sidewinderd */

	/* creating pid file for single instance mechanism */
	int pid_fd = create_pid(pid_file);

	if (pid_fd < 0) {
		return EXIT_FAILURE;
	}

	Keyboard::Keyboard kbd(profile, capture_delays);

	/* main loop */
	/* TODO: exit loop, if keyboards gets unplugged */
	while (sidewinderd::run) {
		kbd.listen_key();
	}

	if (save_profile) {
		/* TODO: get actual profile and save to config */
	}

	close_pid(pid_fd, pid_file);

	return EXIT_SUCCESS;
}
