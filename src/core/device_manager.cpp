/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstring>
#include <iostream>

#include <libudev.h>

#include <core/device_manager.hpp>
#include <vendor/logitech/g105.hpp>
#include <vendor/logitech/g710.hpp>
#include <vendor/microsoft/sidewinder.hpp>

constexpr auto VENDOR_MICROSOFT =	"045e";
constexpr auto VENDOR_LOGITECH =	"046d";

void DeviceManager::scan() {
	for (auto it : devices_) {
		struct Device device = it;
		struct sidewinderd::DevNode devNode;

		if (probe(&device, &devNode) > 0) {
			switch (device.driver) {
				case Device::Driver::LogitechG105: {
					LogitechG105 keyboard(&device, &devNode, config_, process_);
					keyboard.connect();
					break;
				}
				case Device::Driver::LogitechG710: {
					LogitechG710 keyboard(&device, &devNode, config_, process_);
					keyboard.connect();
					break;
				}
				case Device::Driver::SideWinder: {
					SideWinder keyboard(&device, &devNode, config_, process_);
					keyboard.connect();
					break;
				}
			}
		}
	}
}

int DeviceManager::probe(struct Device *device, struct sidewinderd::DevNode *devNode) {
	struct udev *udev;
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *devListEntry;
	bool isFound = false;
	udev = udev_new();

	if (!udev) {
		std::cerr << "Can't create udev." << std::endl;
		return -1;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(devListEntry, devices) {
		const char *sysPath, *devNodePath;
		sysPath = udev_list_entry_get_name(devListEntry);
		dev = udev_device_new_from_syspath(udev, sysPath);

		if (std::string(udev_device_get_subsystem(dev)) == std::string("hidraw")) {
			devNodePath = udev_device_get_devnode(dev);
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

			if (!dev) {
				std::cerr << "Unable to find parent device." << std::endl;
			}

			if (std::string(udev_device_get_sysattr_value(dev, "bInterfaceNumber")) == std::string("01")) {
				dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

				if (std::string(udev_device_get_sysattr_value(dev, "idVendor")) == device->vendor) {
					if (std::string(udev_device_get_sysattr_value(dev, "idProduct")) == device->product) {
						std::clog << "Found device: " << device->vendor << ":" << device->product << std::endl;
						isFound = true;
						devNode->hidraw = devNodePath;
					}
				}
			}
		}

		/* find correct /dev/input/event* file */
		if (std::string(udev_device_get_subsystem(dev)) == std::string("input")
			&& udev_device_get_property_value(dev, "ID_MODEL_ID") != NULL
			&& std::string(udev_device_get_property_value(dev, "ID_MODEL_ID")) == device->product
			&& udev_device_get_property_value(dev, "ID_VENDOR_ID") != NULL
			&& std::string(udev_device_get_property_value(dev, "ID_VENDOR_ID")) == device->vendor
			&& udev_device_get_property_value(dev, "ID_USB_INTERFACE_NUM") != NULL
			&& std::string(udev_device_get_property_value(dev, "ID_USB_INTERFACE_NUM")) == "00"
			&& udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD") != NULL
			&& strstr(sysPath, "event")
			&& udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
				devNode->inputEvent = udev_device_get_devnode(dev);
		}

		udev_device_unref(dev);
	}

	/* free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return isFound;
}

DeviceManager::DeviceManager(libconfig::Config *config, Process *process) {
	// list of supported devices
	devices_ = {
		{VENDOR_MICROSOFT, "074b", "Microsoft SideWinder X6",
			Device::Driver::SideWinder},
		{VENDOR_MICROSOFT, "0768", "Microsoft SideWinder X4",
			Device::Driver::SideWinder},
		{VENDOR_LOGITECH, "c248", "Logitech G105",
			Device::Driver::LogitechG105},
		{VENDOR_LOGITECH, "c24d", "Logitech G710+",
			Device::Driver::LogitechG710}
	};

	config_ = config;
	process_ = process;
}
