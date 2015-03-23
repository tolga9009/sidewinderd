/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICEDATA_CLASS_H
#define DEVICEDATA_CLASS_H

#include <string>

/* global variables */
namespace sidewinderd {
	struct DevNode {
		std::string hidraw, input_event;
	};

	struct DeviceData {
		std::string vid, pid;
		struct DevNode devnode;
	};
};

#endif
