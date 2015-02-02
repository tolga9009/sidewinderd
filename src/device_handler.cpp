/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
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

#include "device_handler.hpp"

void DeviceHandler::search_device(std::string vid, std::string pid) {
	/* for Linux */
	udev(vid, pid);
}

void DeviceHandler::udev(std::string vid, std::string pid) {
	struct udev *udev;
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

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

				if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vid.c_str()) == 0) {
					if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid.c_str()) == 0) {
						std::cout << "Found device: " << vid << ":" << pid << std::endl;
						found = 1;
						devnode_hidraw = strdup(devnode_path);
					}
				}
			}
		}

		/* find correct /dev/input/event* file */
		if (strcmp(udev_device_get_subsystem(dev), "input") == 0
			&& udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD") != NULL
			&& strstr(syspath, "event")
			&& udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
				devnode_input_event = strdup(udev_device_get_devnode(dev));
		}

		udev_device_unref(dev);
	}

	/* free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

DeviceHandler::DeviceHandler() {
}
