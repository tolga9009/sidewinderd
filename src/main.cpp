/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdlib>
#include <csignal>
#include <cstring>

#include <getopt.h>
#include <libudev.h>

#include <libconfig.h++>

#include <device_data.hpp>
#include <main.hpp>
#include <process.hpp>
#include <vendor/logitech/g105.hpp>
#include <vendor/logitech/g710.hpp>
#include <vendor/microsoft/sidewinder.hpp>

void sigHandler(int sig) {
	switch (sig) {
		case SIGINT:
			sidewinderd::isRunning = 0;
			break;
		case SIGTERM:
			sidewinderd::isRunning = 0;
			break;
		default:
			std::cerr << "Unknown signal received." << std::endl;
	}
}

/* TODO: remove exceptions for better portability */
void setupConfig(libconfig::Config *config, std::string configFilePath = "/etc/sidewinderd.conf") {
	try {
		config->readFile(configFilePath.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
	} catch (const libconfig::ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
	}

	libconfig::Setting &root = config->getRoot();

	if (!root.exists("user")) {
		root.add("user", libconfig::Setting::TypeString) = "root";
	}

	if (!root.exists("profile")) {
		root.add("profile", libconfig::Setting::TypeInt) = 1;
	}

	if (!root.exists("capture_delays")) {
		root.add("capture_delays", libconfig::Setting::TypeBoolean) = true;
	}

	if (!root.exists("pid-file")) {
		root.add("pid-file", libconfig::Setting::TypeString) = "/var/run/sidewinderd.pid";
	}

	try {
		config->writeFile(configFilePath.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while writing file." << std::endl;
	}
}

int findDevice(struct sidewinderd::DeviceData *deviceData, struct sidewinderd::DevNode *devNode) {
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

				if (std::string(udev_device_get_sysattr_value(dev, "idVendor")) == deviceData->vid) {
					if (std::string(udev_device_get_sysattr_value(dev, "idProduct")) == deviceData->pid) {
						std::clog << "Found device: " << deviceData->vid << ":" << deviceData->pid << std::endl;
						isFound = true;
						devNode->hidraw = devNodePath;
					}
				}
			}
		}

		/* find correct /dev/input/event* file */
		if (std::string(udev_device_get_subsystem(dev)) == std::string("input")
			&& udev_device_get_property_value(dev, "ID_MODEL_ID") != NULL
			&& std::string(udev_device_get_property_value(dev, "ID_MODEL_ID")) == deviceData->pid
			&& udev_device_get_property_value(dev, "ID_VENDOR_ID") != NULL
			&& std::string(udev_device_get_property_value(dev, "ID_VENDOR_ID")) == deviceData->vid
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

int main(int argc, char *argv[]) {
	/* signal handling */
	struct sigaction action;
	action.sa_handler = sigHandler;
	sigaction(SIGINT, &action, nullptr);
	sigaction(SIGTERM, &action, nullptr);

	/* handling command-line options */
	static struct option longOptions[] = {
		{"config", required_argument, 0, 'c'},
		{"daemon", no_argument, 0, 'd'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt, index = 0;
	std::string configFilePath;
	/* flags */
	bool shouldDaemonize;

	while ((opt = getopt_long(argc, argv, ":c:dv", longOptions, &index)) != -1) {
		switch (opt) {
			case 'c':
				configFilePath = optarg;
				break;
			case 'd':
				shouldDaemonize = true;
				break;
			case 'v':
				std::cout << "sidewinderd version " << sidewinderd::version << std::endl;
				return EXIT_SUCCESS;
			case ':':
				std::cout << "Missing argument." << std::endl;
				break;
			case '?':
				std::cout << "Unrecognized option." << std::endl;
				break;
			default:
				std::cout << "Unexpected error." << std::endl;
				return EXIT_FAILURE;
		}
	}

	/* reading config file */
	libconfig::Config config;

	if (configFilePath.empty()) {
		setupConfig(&config);
	} else {
		setupConfig(&config, configFilePath);
	}

	/* create object for process information */
	Process process(&config);

	/* daemonize, if flag has been set */
	if (shouldDaemonize) {
		int ret = process.daemonize();

		if (ret > 0) {
			return EXIT_SUCCESS;
		} else if (ret < 0) {
			return EXIT_FAILURE;
		}
	}

	/* creating pid file for single instance mechanism */
	int pidFd = process.createPid();

	if (pidFd < 0) {
		return EXIT_FAILURE;
	}

	/* setting gid and uid to configured user */
	process.applyUser();
	/* creating sidewinderd directory in user's home directory */
	process.createWorkdir();

	std::clog << "Started sidewinderd." << std::endl;

	for (auto it: sidewinderd::deviceList) {
		struct sidewinderd::DeviceData deviceData;
		struct sidewinderd::DevNode devNode;
		deviceData.vid = it.vid;
		deviceData.pid = it.pid;

		if (findDevice(&deviceData, &devNode) > 0) {
			if (deviceData.vid == "045e") {
				SideWinder keyboard(&deviceData, &devNode, &config, &process);
				/* main loop */
				/* TODO: exit loop, if keyboards gets unplugged */
				sidewinderd::isRunning = 1;

				while (sidewinderd::isRunning) {
					keyboard.listen();
				}
			} else if (deviceData.vid == "046d") {
				if (deviceData.pid == "c24d") {
					LogitechG710 keyboard(&deviceData, &devNode, &config, &process);
					/* main loop */
					/* TODO: exit loop, if keyboards gets unplugged */
					sidewinderd::isRunning = 1;

					while (sidewinderd::isRunning) {
					keyboard.listen();
					}
				} else if (deviceData.pid == "c248") {
					LogitechG105 keyboard(&deviceData, &devNode, &config, &process);
					/* main loop */
					/* TODO: exit loop, if keyboards gets unplugged */
					sidewinderd::isRunning = 1;

					while (sidewinderd::isRunning) {
						keyboard.listen();
					}
				}
			}
		}
	}

	process.closePid(pidFd);
	std::clog << "Stopped sidewinderd." << std::endl;

	return EXIT_SUCCESS;
}
