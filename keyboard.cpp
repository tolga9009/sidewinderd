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

#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <libudev.h>
#include <unistd.h>

#include <linux/hidraw.h>

#include <sys/ioctl.h>

#include "tinyxml2.h"
#include "keyboard.hpp"

/* constants */
#define MAX_BUF		8
#define MAX_EVENTS	2
#define MIN_PROFILE	0
#define MAX_PROFILE	2
#define SIDEWINDER_X6_MAX_SKEYS	30
#define SIDEWINDER_X4_MAX_SKEYS	6

/* media keys */
#define MKEY_GAMECENTER		0x10
#define MKEY_RECORD			0x11
#define MKEY_PROFILE		0x14

void Keyboard::setup_udev() {
	struct udev *udev;
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	std::string vid_microsoft("045e");
	std::string pid_sidewinder_x6("074b");
	std::string pid_sidewinder_x4("0768");

	udev = udev_new();

	if (!udev) {
		std::cout << "Can't create udev" << std::endl;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *syspath, *devnode_path;
		syspath = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, syspath);

		if (strcmp(udev_device_get_subsystem(dev), "hidraw") == 0) {
			devnode_path = udev_device_get_devnode(dev);
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

			if (!dev) {
				std::cout << "Unable to find parent device" << std::endl;
			}

			if (strcmp(udev_device_get_sysattr_value(dev, "bInterfaceNumber"), "01") == 0) {
				dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

				if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vid_microsoft.c_str()) == 0) {
					if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x6.c_str()) == 0) {
						devnode_hidraw = strdup(devnode_path);
						max_skeys = SIDEWINDER_X6_MAX_SKEYS;
					} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x4.c_str()) == 0) {
						devnode_hidraw = strdup(devnode_path);
						max_skeys = SIDEWINDER_X4_MAX_SKEYS;
					}
				}
			}
		}

		/* find correct /dev/input/event* file */
		if (strcmp(udev_device_get_subsystem(dev), "input") == 0
			&& udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD") != NULL
			&& strstr(syspath, "event")
			&& udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
				devnode_input = strdup(udev_device_get_devnode(dev));
		}

		udev_device_unref(dev);
	}

	/* free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

/* Returns path to XML, containing macro instructions. */
std::string Keyboard::get_xmlpath(int key) {
	std::stringstream path;

	if (key && key < max_skeys) {
		path << "profile_" << profile + 1 << "/" << "s" << key << ".xml";
	}

	return path.str();
}

void Keyboard::setup_uidev() {
	int i;
	uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (uifd < 0) {
		uifd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);

		if (uifd < 0) {
			std::cout << "Can't open uinput" << std::endl;
		}
	}

	/*
	 * TODO: dynamically get and setbit needed keys by play_macro()
	 *
	 * Currently, we set all keybits, to make things easier.
	 */
	ioctl(uifd, UI_SET_EVBIT, EV_KEY);

	for (i = KEY_ESC; i <= KEY_KPDOT; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (i = KEY_PLAYCD; i <= KEY_MICMUTE; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	/* our uinput device's details */
	/* TODO: read keyboard information via udev and set config here */
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "sidewinderd");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;
	write(uifd, &uidev, sizeof(struct uinput_user_dev));
	ioctl(uifd, UI_DEV_CREATE);
}

void Keyboard::feature_request(unsigned char data) {
	unsigned char buf[2];
	/* buf[0] is Report Number, buf[1] is the control byte */
	buf[0] = 0x7;
	buf[1] = data << profile;
	buf[1] |= macropad;
	buf[1] |= record_led << 5;

	ioctl(fd, HIDIOCSFEATURE(sizeof(buf)), buf);
}

void Keyboard::setup_epoll() {
	epfd = epoll_create1(0);
	epev.data.fd = fd;
	epev.events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epev);
}

void Keyboard::toggle_macropad() {
	macropad ^= 1;
	feature_request();
}

void Keyboard::switch_profile() {
	profile++;

	if (profile > MAX_PROFILE) {
		profile = MIN_PROFILE;
	}

	feature_request();
}

/*
 * get_input() checks, which keys were pressed. The macro keys are
 * packed in a 5-byte buffer, media keys (including Bank Switch and
 * Record) use 8-bytes.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been
 * pressed at the same time.
 */
int Keyboard::get_input() {
	int key, nbytes;
	unsigned char buf[MAX_BUF];

	nbytes = read(fd, buf, MAX_BUF);

	if (nbytes == 5 && buf[0] == 8) {
		/*
		 * cutting off buf[0], which is used to differentiate between
		 * macro and media keys. Our task is now to translate the buffer
		 * codes to something we can work with. Here is a table, where
		 * you can look up the keys and buffer, if you want to improve
		 * the current method:
		 *
		 * S1	0x08 0x01 0x00 0x00 0x00 - buf[1]
		 * S2	0x08 0x02 0x00 0x00 0x00 - buf[1]
		 * S3	0x08 0x04 0x00 0x00 0x00 - buf[1]
		 * S4	0x08 0x08 0x00 0x00 0x00 - buf[1]
		 * S5	0x08 0x10 0x00 0x00 0x00 - buf[1]
		 * S6	0x08 0x20 0x00 0x00 0x00 - buf[1]
		 * S7	0x08 0x40 0x00 0x00 0x00 - buf[1]
		 * S8	0x08 0x80 0x00 0x00 0x00 - buf[1]
		 * S9	0x08 0x00 0x01 0x00 0x00 - buf[2]
		 * S10	0x08 0x00 0x02 0x00 0x00 - buf[2]
		 * S11	0x08 0x00 0x04 0x00 0x00 - buf[2]
		 * S12	0x08 0x00 0x08 0x00 0x00 - buf[2]
		 * S13	0x08 0x00 0x10 0x00 0x00 - buf[2]
		 * S14	0x08 0x00 0x20 0x00 0x00 - buf[2]
		 * S15	0x08 0x00 0x40 0x00 0x00 - buf[2]
		 * S16	0x08 0x00 0x80 0x00 0x00 - buf[2]
		 * S17	0x08 0x00 0x00 0x01 0x00 - buf[3]
		 * S18	0x08 0x00 0x00 0x02 0x00 - buf[3]
		 * S19	0x08 0x00 0x00 0x04 0x00 - buf[3]
		 * S20	0x08 0x00 0x00 0x08 0x00 - buf[3]
		 * S21	0x08 0x00 0x00 0x10 0x00 - buf[3]
		 * S22	0x08 0x00 0x00 0x20 0x00 - buf[3]
		 * S23	0x08 0x00 0x00 0x40 0x00 - buf[3]
		 * S24	0x08 0x00 0x00 0x80 0x00 - buf[3]
		 * S25	0x08 0x00 0x00 0x00 0x01 - buf[4]
		 * S26	0x08 0x00 0x00 0x00 0x02 - buf[4]
		 * S27	0x08 0x00 0x00 0x00 0x04 - buf[4]
		 * S28	0x08 0x00 0x00 0x00 0x08 - buf[4]
		 * S29	0x08 0x00 0x00 0x00 0x10 - buf[4]
		 * S30	0x08 0x00 0x00 0x00 0x20 - buf[4]
		 */
		for (int i = 1; i < nbytes; i++) {
			for (int j = 0; buf[i]; j++) {
				key = ((i - 1) * 8) + ffs(buf[i]);

				return key;

				buf[i] &= buf[i] - 1;
			}
		}
	} else if (nbytes == 8 && buf[0] == 1 && buf[6]) {
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		return max_skeys + buf[6];
	}

	return 0;
}

void Keyboard::process_input(int key) {
	if (key == max_skeys + MKEY_GAMECENTER) {
		toggle_macropad();
	} else if (key == max_skeys + MKEY_RECORD) {
		record_macro();
	} else if (key == max_skeys + MKEY_PROFILE) {
		switch_profile();
	} else if (key) {
		play_macro(get_xmlpath(key));
	}
}

void Keyboard::send_key(short type, short code, int value) {
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

/* TODO: interrupt and exit play_macro when a key is pressed */
/* BUG(?): if Bank Switch is pressed during play_macro(), crazy things happen */
/* TODO: block play_macro, if any key has been pressed during execution */
void Keyboard::play_macro(std::string path) {
	tinyxml2::XMLDocument doc;

	doc.LoadFile(path.c_str());
	tinyxml2::XMLElement* root = doc.FirstChildElement("Macro");

	for (tinyxml2::XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (child->Name() == std::string("KeyBoardEvent")) {
			bool boolDown;
			int key = std::atoi(child->GetText());

			child->QueryBoolAttribute("Down", &boolDown);
			send_key(EV_KEY, key, boolDown);
		} else if (child->Name() == std::string("DelayEvent")) {
			int delay = std::atoi(child->GetText());
			struct timespec request, remain;
			/*
			 * value is given in milliseconds, so we need to split
			 * it into seconds and nanoseconds. nanosleep() is
			 * interruptable and saves the remaining sleep time.
			 */
			request.tv_sec = delay / 1000;
			delay = delay - (request.tv_sec * 1000);
			request.tv_nsec = 1000000L * delay;
			nanosleep(&request, &remain);
		}
	}
}

/*
 * Macro recording captures delays by default. Use the configuration
 * to disable capturing delays.
 */
/*
 * TODO: abort Macro recording, if Record key is pressed again.
 */
void Keyboard::record_macro() {
	std::string path;
	int run;
	struct timeval prev;

	prev.tv_usec = 0;
	prev.tv_sec = 0;
	run = 1;
	record_led = 3;
	feature_request();

	while (run) {
		int key;
		epoll_wait(epfd, &epev, MAX_EVENTS, -1);
		key = get_input();

		if (key) {
			path = get_xmlpath(key);
			record_led = 2;
			run = 0;
			feature_request();
		}
	}

	std::cout << "Start Macro Recording" << std::endl;

	evfd = open(devnode_input.c_str(), O_RDONLY | O_NONBLOCK);

	if (evfd < 0) {
		std::cout << "Can't open input event file" << std::endl;
	}

	/* add /dev/input/event* to epoll watchlist */
	epoll_ctl(epfd, EPOLL_CTL_ADD, evfd, &epev);

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLNode* root = doc.NewElement("Macro");

	/* start root element "Macro" */
	doc.InsertFirstChild(root);

	run = 1;

	while (run) {
		int key;
		epoll_wait(epfd, &epev, MAX_EVENTS, -1);

		key = get_input();

		if (key == max_skeys + MKEY_RECORD) {
			record_led = 0;
			run = 0;
			feature_request();
		}

		struct input_event inev;

		read(evfd, &inev, sizeof(struct input_event));

		if (inev.type == 1 && inev.value != 2) {

			/* TODO: read config, if capture_delays is true */
			if (prev.tv_usec) {
				long int diff = (inev.time.tv_usec + 1000000 * inev.time.tv_sec) - (prev.tv_usec + 1000000 * prev.tv_sec);
				int delay = diff / 1000;

				/* start element "DelayEvent" */
				tinyxml2::XMLElement* DelayEvent = doc.NewElement("DelayEvent");

				DelayEvent->SetText(delay);
				root->InsertEndChild(DelayEvent);
			}
			/* start element "KeyBoardEvent" */
			tinyxml2::XMLElement* KeyBoardEvent = doc.NewElement("KeyBoardEvent");

			if (inev.value) {
				KeyBoardEvent->SetAttribute("Down", true);
			} else {
				KeyBoardEvent->SetAttribute("Down", false);
			}

			KeyBoardEvent->SetText(inev.code);
			root->InsertEndChild(KeyBoardEvent);

			prev = inev.time;
		}
	}

	/* write XML document */
	doc.SaveFile(path.c_str());

	std::cout << "Exit Macro Recording" << std::endl;

	/* remove event file from epoll watchlist */
	epoll_ctl(epfd, EPOLL_CTL_DEL, evfd, &epev);
	close(evfd);
}

void Keyboard::listen_key() {
	/*
	 * epoll_wait() checks our device for any changes and blocks the
	 * loop. This leads to a very efficient polling mechanism.
	 */
	epoll_wait(epfd, &epev, MAX_EVENTS, -1);
	process_input(get_input());
}

Keyboard::Keyboard() {
	profile = 0;
	auto_led = 0;
	record_led = 0;
	macropad = 0;

	setup_udev();

	fd = open(devnode_hidraw.c_str(), O_RDWR | O_NONBLOCK);

	if (fd < 0) {
		std::cout << "Can't open hidraw interface" << std::endl;
	}

	feature_request();

	setup_epoll();
	setup_uidev();
}

Keyboard::~Keyboard() {
	feature_request(0);

	close(uifd);
	close(fd);
}
