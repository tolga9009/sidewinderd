/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_CLASS_H
#define DEVICE_CLASS_H

#include <string>

struct Device {
		std::string vendor;
		std::string product;
		std::string name;

		enum class Driver {
			LogitechG105,
			LogitechG710,
			LogitechG815,
			SideWinder
		} driver;
};

#endif
