/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <linux/uinput.h>

#include <sys/ioctl.h>

#include "virtual_input.hpp"

void VirtualInput::createUidev() {
	/* open uinput device with root privileges */
	seteuid(0);
	uifd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (uifd_ < 0) {
		uifd_ = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

		if (uifd_ < 0) {
			std::cout << "Can't open uinput" << std::endl;
		}
	}

	seteuid(pw_->pw_uid);
	/* TODO: copy original keyboard's keybits. */
	/* Currently, we set all keybits, to make things easier. */
	ioctl(uifd_, UI_SET_EVBIT, EV_KEY);

	for (int i = KEY_ESC; i <= KEY_KPDOT; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_PLAYCD; i <= KEY_MICMUTE; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	struct uinput_user_dev uidev = uinput_user_dev();
	/* our uinput device's details */
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Sidewinderd");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = std::stoi(deviceData_->vid, nullptr, 16);
	uidev.id.product = std::stoi(deviceData_->pid, nullptr, 16);
	uidev.id.version = 1;
	write(uifd_, &uidev, sizeof(struct uinput_user_dev));
	ioctl(uifd_, UI_DEV_CREATE);
}

void VirtualInput::sendEvent(short type, short code, int value) {
	struct input_event inev = input_event();
	inev.type = type;
	inev.code = code;
	inev.value = value;
	write(uifd_, &inev, sizeof(struct input_event));
	inev.type = EV_SYN;
	inev.code = 0;
	inev.value = 0;
	write(uifd_, &inev, sizeof(struct input_event));
}

VirtualInput::VirtualInput(sidewinderd::DeviceData *deviceData, struct passwd *pw) {
	pw_ = pw;
	deviceData_ = deviceData;
	/* for Linux */
	createUidev();
}

VirtualInput::~VirtualInput() {
	close(uifd_);
}
