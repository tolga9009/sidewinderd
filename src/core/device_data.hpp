/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICEDATA_CLASS_H
#define DEVICEDATA_CLASS_H

#include <string>
#include <vector>

namespace sidewinderd {
	/**
	 * Struct for storing and passing paths to relevant /dev/<node> files.
	 */
	struct DevNode {
		std::string hidraw, inputEvent; /**< path to hidraw and input event */
	};

	/**
	 * Struct for storing and passing device data.
	 */
	struct DeviceData {
		std::string vid, pid, name; /**< device's VID and PID */
	};

	/**
	 * List of supported devices.
	 */
	extern std::vector<DeviceData> deviceList;
};

#endif
