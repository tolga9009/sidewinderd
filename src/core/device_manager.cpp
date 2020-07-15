/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstring>
#include <iostream>

#include <core/device_manager.hpp>
#include <vendor/logitech/g103.hpp>
#include <vendor/logitech/g105.hpp>
#include <vendor/logitech/g710.hpp>
#include <vendor/microsoft/sidewinder.hpp>

constexpr auto VENDOR_MICROSOFT =	"045e";
constexpr auto VENDOR_LOGITECH =	"046d";
constexpr auto TIMEOUT =		5000;
constexpr auto NUM_POLL =		1;

void DeviceManager::discover() {
	for (auto it : devices_) {
		struct Device device = it;
		struct sidewinderd::DevNode devNode;

		if (probe(&device, &devNode) > 0) {
			// TODO use a unique identifier
			auto it = connected_.find(device.product);

			// skip this loop, if device is already connected
			if (it != connected_.end()) {
				continue;
			}

			switch (device.driver) {
				case Device::Driver::LogitechG103: {
					auto keyboard = new LogitechG103(&device, &devNode, config_, process_);
					keyboard->connect();
					connected_[device.product] = std::unique_ptr<Keyboard>(keyboard);
					break;
				}
				case Device::Driver::LogitechG105: {
					auto keyboard = new LogitechG105(&device, &devNode, config_, process_);
					keyboard->connect();
					connected_[device.product] = std::unique_ptr<Keyboard>(keyboard);
					break;
				}
				case Device::Driver::LogitechG710: {
					auto keyboard = new LogitechG710(&device, &devNode, config_, process_);
					keyboard->connect();
					connected_[device.product] = std::unique_ptr<Keyboard>(keyboard);
					break;
				}
				case Device::Driver::SideWinder: {
					auto keyboard = new SideWinder(&device, &devNode, config_, process_);
					keyboard->connect();
					connected_[device.product] = std::unique_ptr<Keyboard>(keyboard);
					break;
				}
			}
		}
	}
}

int DeviceManager::monitor() {
	// create udev object
	udev_ = udev_new();

	if (!udev_) {
		std::cerr << "Can't create udev." << std::endl;

		return -1;
	}

	// set up monitor for /dev/hidraw and /dev/input/event*
	monitor_ = udev_monitor_new_from_netlink(udev_, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(monitor_, "hidraw", NULL);
	udev_monitor_filter_add_match_subsystem_devtype(monitor_, "input", NULL);
	udev_monitor_enable_receiving(monitor_);

	// get file descriptor for the monitor, so we can use poll() on it
	fd_ = udev_monitor_get_fd(monitor_);

	// setup poll
	pfd_.fd = fd_;
	pfd_.events = POLLIN;

	// initial discovery of new devices
	discover();

	struct udev_device *dev;

	// run monitoring loop, until we receive a signal
	while (process_->isActive()) {
		poll(&pfd_, NUM_POLL, TIMEOUT);
		dev = udev_monitor_receive_device(monitor_);

		if (dev) {
			// filter out nullptr returns, else std::string() fails
			auto ret = udev_device_get_action(dev);

			if (ret) {
				std::string action(ret);

				if (action == "add") {
					discover();
				} else if (action == "remove") {
					// check for disconnected devices
					unbind();
				}
			}

			udev_device_unref(dev);
		}
	}

	udev_monitor_unref(monitor_);
	udev_unref(udev_);

	return 0;
}

int DeviceManager::probe(struct Device *device, struct sidewinderd::DevNode *devNode) {
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *entry;
	bool isFound = false;

	// create a list of devices in hidraw and input subsystems
	enumerate = udev_enumerate_new(udev_);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(entry, devices) {
		const char *sysPath, *devNodePath;
		sysPath = udev_list_entry_get_name(entry);
		dev = udev_device_new_from_syspath(udev_, sysPath);
		auto hidraw = udev_device_get_subsystem(dev);

		// evaluation from left to right; used to filter out nullptr
		if (hidraw && std::string(hidraw) == std::string("hidraw")) {
			devNodePath = udev_device_get_devnode(dev);
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

			if (!dev) {
				std::cerr << "Unable to find parent device." << std::endl;
			}

			auto bInterfaceNumber = udev_device_get_sysattr_value(dev, "bInterfaceNumber");

			if (bInterfaceNumber && std::string(bInterfaceNumber) == std::string("01")) {
				dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
				auto idVendor = udev_device_get_sysattr_value(dev, "idVendor");
				auto idProduct = udev_device_get_sysattr_value(dev, "idProduct");

				if (idVendor && std::string(idVendor) == device->vendor
					&& idProduct && std::string(idProduct) == device->product) {
						devNode->hidraw = devNodePath;
				}
			}
		}

		auto input = udev_device_get_subsystem(dev);
		auto product = udev_device_get_property_value(dev, "ID_MODEL_ID");
		auto vendor = udev_device_get_property_value(dev, "ID_VENDOR_ID");
		auto interface = udev_device_get_property_value(dev, "ID_USB_INTERFACE_NUM");

		/* find correct /dev/input/event* file */
		if (input && std::string(input) == std::string("input")
			&& product && std::string(product) == device->product
			&& vendor && std::string(vendor) == device->vendor
			&& interface && std::string(interface) == "00"
			&& udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD")
			&& strstr(sysPath, "event")
			&& udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
				devNode->inputEvent = udev_device_get_devnode(dev);
				std::clog << "Found device: " << device->vendor << ":" << device->product << std::endl;
				isFound = true;
		}

		udev_device_unref(dev);
	}

	/* free the enumerator object */
	udev_enumerate_unref(enumerate);

	return isFound;
}

void DeviceManager::unbind() {
	for (auto it = connected_.cbegin(); it != connected_.cend();) {
		if (it->second->isConnected()) {
			++it;
		} else {
			it = connected_.erase(it);
		}
	}
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
		{VENDOR_LOGITECH, "c24b", "Logitech G103",
			Device::Driver::LogitechG103},
		{VENDOR_LOGITECH, "c24d", "Logitech G710+",
			Device::Driver::LogitechG710}
	};

	config_ = config;
	process_ = process;
	udev_ = nullptr;
	monitor_ = nullptr;
}

DeviceManager::~DeviceManager() {
	unbind();

	if (udev_) {
		udev_unref(udev_);
	}
}
