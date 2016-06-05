/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_MANAGER_CLASS_H
#define DEVICE_MANAGER_CLASS_H

#include <array>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <process.hpp>
#include <core/keyboard.hpp>

class DeviceManager {
	public:
		void monitor();
		void discover();
		void check();
		int probe(Device *device);
		void bind(Device *device);
		void unbind(Device *device);
		DeviceManager(Process *process);
		~DeviceManager();

	private:
		std::vector<DeviceId> supportedDevices_;
		std::map<std::string, Device> connectedDevices_;
		Process *process_;
};

#endif
