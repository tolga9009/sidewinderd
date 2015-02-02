/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef VIRTUALINPUT_CLASS_H
#define VIRTUALINPUT_CLASS_H

#include <string>

#include <pwd.h>

#include <linux/uinput.h>

class VirtualInput {
	public:
		void send_event(short type, short code, int value);
		VirtualInput(struct passwd *pw);
		~VirtualInput();
	private:
		int uifd;
		struct passwd *pw;
		struct uinput_user_dev uidev;
		void create_uidev();
};

#endif
