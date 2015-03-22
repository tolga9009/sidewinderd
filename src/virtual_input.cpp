/*
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "virtual_input.hpp"

void VirtualInput::create_uidev() {
	/* open uinput device with root privileges */
	seteuid(0);
	uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (uifd < 0) {
		uifd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

		if (uifd < 0) {
			std::cout << "Can't open uinput" << std::endl;
		}
	}
	seteuid(pw->pw_uid);

	/* TODO: copy original keyboard's keybits. */
	/* Currently, we set all keybits, to make things easier. */
	ioctl(uifd, UI_SET_EVBIT, EV_KEY);

	for (int i = KEY_ESC; i <= KEY_KPDOT; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_PLAYCD; i <= KEY_MICMUTE; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	/* our uinput device's details */
	/* TODO: read keyboard information and set here */
	snprintf(uidev->name, UINPUT_MAX_NAME_SIZE, "sidewinderd");
	uidev->id.bustype = BUS_USB;
	uidev->id.vendor = 0x1;
	uidev->id.product = 0x1;
	uidev->id.version = 1;
	write(uifd, &uidev, sizeof(struct uinput_user_dev));
	ioctl(uifd, UI_DEV_CREATE);
}

void VirtualInput::send_event(short type, short code, int value) {
	struct input_event inev;

	inev.type = type;
	inev.code = code;
	inev.value = value;
	write(uifd, &inev, sizeof(struct input_event));

	inev.type = EV_SYN;
	inev.code = 0;
	inev.value = 0;
	write(uifd, &inev, sizeof(struct input_event));
}

VirtualInput::VirtualInput(struct passwd *pw) {
	VirtualInput::pw = pw;
	struct uinput_user_dev uidev;
	VirtualInput::uidev = &uidev;

	/* for Linux */
	create_uidev();
}

VirtualInput::~VirtualInput() {
	close(uifd);
}
