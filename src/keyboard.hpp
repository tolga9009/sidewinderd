/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef KEYBOARD_CLASS_H
#define KEYBOARD_CLASS_H

#include <string>

#include <poll.h>
#include <pwd.h>

#include <libconfig.h++>

#include "virtual_input.hpp"

class Keyboard {
	public:
		void listen();
		Keyboard(std::string devnode_hidraw, std::string devnode_input_event, libconfig::Config *config, struct passwd *pw);
		~Keyboard();
	private:
		int profile, auto_led, record_led, macropad, max_skeys;
		int fd, evfd;
		struct passwd *pw;
		struct pollfd fds[2];
		std::string devnode_hidraw, devnode_input_event;
		libconfig::Config *config;
		VirtualInput *virtinput;
		int get_input();
		void process_input(int key);
		std::string get_xmlpath(int key);
		void feature_request(unsigned char data = 0x04);
		void setup_poll();
		void play_macro(std::string path);
		void record_macro();
		void toggle_macropad();
		void switch_profile();
};

#endif
