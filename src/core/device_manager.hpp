/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_MANAGER_CLASS_H
#define DEVICE_MANAGER_CLASS_H

#include <array>
#include <string>
#include <vector>

#include <process.hpp>
#include <core/keyboard.hpp>

class DeviceManager {
	public:
		void monitor();
		int probe(Device *device);
		void bind(Device *device);
		//void unbind(Device *device);
		DeviceManager(Process *process);

	private:
		std::vector<DeviceId> supportedDevices_;
		std::vector<Device> connectedDevices_;
		Process *process_;
};

#endif
