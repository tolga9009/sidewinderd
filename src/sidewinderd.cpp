/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdlib>
#include <csignal>
#include <cstring>
#include <sstream>

#include <fcntl.h>
#include <getopt.h>
#include <libudev.h>
#include <pwd.h>
#include <unistd.h>

#include <libconfig.h++>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "keyboard.hpp"
#include "logitech_g105.hpp"
#include "logitech_g710p.hpp"
#include "microsoft_sidewinder.hpp"
#include "sidewinderd.hpp"
#include "virtual_input.hpp"

void sigHandler(int sig) {
	switch (sig) {
		case SIGINT:
			sidewinderd::isRunning = 0;
			break;
		case SIGTERM:
			sidewinderd::isRunning = 0;
			break;
		default:
			std::cout << "Unknown signal received." << std::endl;
	}
}

int createPid(std::string pidFilePath) {
	int pidFd = open(pidFilePath.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (pidFd < 0) {
		std::cout << "PID file could not be created." << std::endl;
		return -1;
	}

	if (flock(pidFd, LOCK_EX | LOCK_NB) < 0) {
		std::cout << "Could not lock PID file, another instance is already running. Terminating." << std::endl;
		close(pidFd);
		return -1;
	}

	return pidFd;
}

void closePid(int pidFd, std::string pidFilePath) {
	flock(pidFd, LOCK_UN);
	close(pidFd);
	unlink(pidFilePath.c_str());
}

/* TODO: throw exceptions instead of returning values */
int initializeDaemon() {
	pid_t pid, sid;
	pid = fork();

	if (pid < 0) {
		std::cout << "Error creating daemon" << std::endl;
		return -1;
	}

	if (pid > 0) {
		return 1;
	}

	sid = setsid();

	if (sid < 0) {
		std::cout << "Error setting sid" << std::endl;
		return -1;
	}

	pid = fork();

	if (pid < 0) {
		std::cout << "Error forking second time" << std::endl;
		return -1;
	}

	if (pid > 0) {
		return 1;
	}

	umask(0);
	chdir("/");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	return 0;
}

void setupConfig(libconfig::Config *config, std::string configFilePath = "/etc/sidewinderd.conf") {
	try {
		config->readFile(configFilePath.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
	} catch (const libconfig::ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
	}

	libconfig::Setting &root = config->getRoot();

	/* TODO: check values for validity and throw exceptions, if invalid */
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
		std::cout << "Can't create udev" << std::endl;
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
				std::cout << "Unable to find parent device" << std::endl;
			}

			if (std::string(udev_device_get_sysattr_value(dev, "bInterfaceNumber")) == std::string("01")) {
				dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

				if (std::string(udev_device_get_sysattr_value(dev, "idVendor")) == deviceData->vid) {
					if (std::string(udev_device_get_sysattr_value(dev, "idProduct")) == deviceData->pid) {
						std::cout << "Found device: " << deviceData->vid << ":" << deviceData->pid << std::endl;
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
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt, index, ret;
	std::string configFilePath;
	index = 0;

	while ((opt = getopt_long(argc, argv, ":c:dv", longOptions, &index)) != -1) {
		switch (opt) {
			case 'c':
				configFilePath = optarg;
				break;
			case 'd':
				ret = initializeDaemon();

				if (ret > 0) {
					return EXIT_SUCCESS;
				}

				if (ret < 0) {
					return EXIT_FAILURE;
				};

				break;
			case 'v':
				std::cout << "Option --verbose" << std::endl;
				break;
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

	/* creating pid file for single instance mechanism */
	std::string pidFilePath = config.lookup("pid-file");
	int pidFd = createPid(pidFilePath);

	if (pidFd < 0) {
		return EXIT_FAILURE;
	}

	/* get user's home directory */
	std::string user = config.lookup("user");
	struct passwd *pw = getpwnam(user.c_str());
	/* setting gid and uid to configured user */
	setegid(pw->pw_gid);
	seteuid(pw->pw_uid);
	/* creating sidewinderd directory in user's home directory */
	std::string workdir = pw->pw_dir;
	std::string xdgData;

	if (const char *env = std::getenv("XDG_DATA_HOME")) {
		xdgData = env;

		if (!xdgData.empty()) {
			workdir = xdgData;
		}
	} else {
		xdgData = "/.local/share";
		workdir.append(xdgData);
	}

	workdir.append("/sidewinderd");
	mkdir(workdir.c_str(), S_IRWXU);

	if (chdir(workdir.c_str())) {
		std::cout << "Error chdir" << std::endl;
	}

	std::cout << "Sidewinderd v" << sidewinderd::version << " has been started." << std::endl;

	for (std::vector<sidewinderd::DeviceData>::iterator it = sidewinderd::deviceList.begin(); it != sidewinderd::deviceList.end(); ++it) {
		struct sidewinderd::DeviceData deviceData;
		struct sidewinderd::DevNode devNode;
		deviceData.vid = it->vid;
		deviceData.pid = it->pid;

		if (findDevice(&deviceData, &devNode) > 0) {
			if (deviceData.vid == "045e") {
				SideWinder keyboard(&deviceData, &devNode, &config, pw);
				/* main loop */
				/* TODO: exit loop, if keyboards gets unplugged */
				sidewinderd::isRunning = 1;

				while (sidewinderd::isRunning) {
					keyboard.listen();
				}
			} else if (deviceData.vid == "046d") {
				if (deviceData.pid == "c24d") {
					LogitechG710 keyboard(&deviceData, &devNode, &config, pw);
					/* main loop */
					/* TODO: exit loop, if keyboards gets unplugged */
					sidewinderd::isRunning = 1;

					while (sidewinderd::isRunning) {
					keyboard.listen();
					}
				} else if (deviceData.pid == "c248") {
					LogitechG105 keyboard(&deviceData, &devNode, &config, pw);
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

	closePid(pidFd, pidFilePath);
	std::cout << "Sidewinderd has been terminated." << std::endl;

	return EXIT_SUCCESS;
}
