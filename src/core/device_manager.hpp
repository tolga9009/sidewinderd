/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_MANAGER_CLASS_H
#define DEVICE_MANAGER_CLASS_H

#include <map>
#include <string>
#include <vector>

#include <libudev.h>
#include <poll.h>

#include <libconfig.h++>

#include <device_data.hpp>
#include <process.hpp>
#include <core/device.hpp>
#include <core/keyboard.hpp>

class DeviceManager {
	public:
		int monitor();
		DeviceManager(libconfig::Config *config, Process *process);
		~DeviceManager();

	private:
		int fd_;
		std::map<std::string, std::unique_ptr<Keyboard>> connected_;
		std::vector<Device> devices_;
		struct pollfd pfd_;
		struct udev *udev_;
		struct udev_monitor *monitor_;
		libconfig::Config *config_;
		Process *process_;
		void discover();
		int probe(struct Device *device, struct sidewinderd::DevNode *devNode);
		void unbind();
};

#endif
