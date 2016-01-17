/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef SIDEWINDERD_H
#define SIDEWINDERD_H

#include <atomic>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "device_data.hpp"

namespace sidewinderd {
	std::string version("0.2.1");
	std::atomic<bool> isRunning; /**< Sidewinderd main thread */

	/**
	 * List of supported devices.
	 */
	std::vector<DeviceData> deviceList = {
		{"045e", "074b", "Microsoft SideWinder X6"},
		{"045e", "0768", "Microsoft SideWinder X4"}
	};
};

#endif
