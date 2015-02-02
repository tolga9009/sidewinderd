/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICEHANDLER_CLASS_H
#define DEVICEHANDLER_CLASS_H

#include <string>

#include <poll.h>
#include <pwd.h>

#include <libconfig.h++>

#include "virtual_input.hpp"

namespace sidewinderd {
}

class DeviceHandler {
	public:
		bool found;
		std::string devnode_hidraw, devnode_input_event;
		void search_device(std::string vid, std::string pid);
		DeviceHandler();
	private:
		void udev(std::string vid, std::string pid);
};

#endif
