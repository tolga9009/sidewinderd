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

#ifndef KEYBOARD_CLASS_H
#define KEYBOARD_CLASS_H

#include <string>

#include <pwd.h>

#include <libconfig.h++>

#include <linux/uinput.h>

#include <sys/epoll.h>

class Keyboard {
	public:
		int profile, auto_led, record_led, max_skeys, macropad;
		std::string device_id;
		std::string devnode_hidraw;
		std::string devnode_input;
		void toggle_macropad();
		void switch_profile();
		void listen_key();
		Keyboard(libconfig::Config *config, struct passwd *pw);
		~Keyboard();
	private:
		libconfig::Config *config;
		struct passwd *pw;
		int fd, uifd, epfd, evfd;
		struct epoll_event epev;
		struct uinput_user_dev uidev;
		int get_input();
		void process_input(int key);
		std::string get_xmlpath(int key);
		void setup_udev();
		void setup_uidev();
		void feature_request(unsigned char data = 0x04);
		void setup_epoll();
		void send_key(short type, short code, int value);
		void play_macro(std::string path);
		void record_macro();
};

#endif
