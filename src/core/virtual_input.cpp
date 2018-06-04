/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
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

/**
 * Method for sending input events to the operating system.
 *
 * @param type type of input event, e.g. EV_KEY
 * @param code keycode defined in header file input.h
 * @param value value the event carries, e.g. EV_KEY: 0 represents release, 1
 * keypress and 2 autorepeat
 */
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

/**
 * Constructor setting up operating system specific back-ends.
 */
VirtualInput::VirtualInput(struct Device *device, sidewinderd::DevNode *devNode, Process *process) {
	process_ = process;
	device_ = device;
	devNode_ = devNode;
	/* for Linux */
	createUidev();
}

VirtualInput::~VirtualInput() {
	close(uifd_);
}

/**
 * Creating a uinput virtual input device under Linux.
 */
void VirtualInput::createUidev() {
	/* open uinput device with root privileges */
	process_->privilege();
	uifd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (uifd_ < 0) {
		uifd_ = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

		if (uifd_ < 0) {
			std::cout << "Can't open uinput" << std::endl;
		}
	}

	process_->unprivilege();
	/* set all keybits */
	ioctl(uifd_, UI_SET_EVBIT, EV_KEY);
	ioctl(uifd_, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(uifd_, UI_SET_KEYBIT, BTN_RIGHT);

	ioctl(uifd_, UI_SET_EVBIT, EV_REL);
	ioctl(uifd_, UI_SET_RELBIT, REL_X);
	ioctl(uifd_, UI_SET_RELBIT, REL_Y);

	for (int i = KEY_ESC; i <= KEY_KPDOT; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	for (int i = KEY_PLAYCD; i <= KEY_MICMUTE; i++) {
		ioctl(uifd_, UI_SET_KEYBIT, i);
	}

	/* uinput device details */
	struct uinput_user_dev uidev = uinput_user_dev();
	/* TODO: copy device's name */
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Sidewinderd");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = std::stoi(device_->vendor, nullptr, 16);
	uidev.id.product = std::stoi(device_->product, nullptr, 16);
	uidev.id.version = 1;
	/* write uinput device details */
	write(uifd_, &uidev, sizeof(struct uinput_user_dev));
	/* create uinput device */
	ioctl(uifd_, UI_DEV_CREATE);
}
