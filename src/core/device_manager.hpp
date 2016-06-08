/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_MANAGER_CLASS_H
#define DEVICE_MANAGER_CLASS_H

#include <string>
#include <vector>

#include <libconfig.h++>

#include <device_data.hpp>
#include <process.hpp>
#include <core/device.hpp>

class DeviceManager {
	public:
		void scan();
		int probe(struct Device *device, struct sidewinderd::DevNode *devNode);
		DeviceManager(libconfig::Config *config, Process *process);

	private:
		std::vector<Device> devices_;
		libconfig::Config *config_;
		Process *process_;
};

#endif
