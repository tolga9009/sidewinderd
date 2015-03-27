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

#include "device_data.hpp"
#include "key.hpp"
#include "virtual_input.hpp"

class Keyboard {
	public:
		void listen();
		Keyboard(sidewinderd::DeviceData *data, libconfig::Config *config, struct passwd *pw);
		~Keyboard();
	private:
		int profile, auto_led, record_led, macropad;
		int fd, evfd;
		struct passwd *pw;
		struct pollfd fds[2];
		libconfig::Config *config;
		sidewinderd::DeviceData *data;
		VirtualInput *virtinput;
		struct KeyData get_input();
		void feature_request(unsigned char data = 0x04);
		void setup_poll();
		static void play_macro(std::string path, VirtualInput *virtinput);
		void record_macro();
		void toggle_macropad();
		void switch_profile();
		struct KeyData check();
		void process_input(struct KeyData *kd);
};

#endif
