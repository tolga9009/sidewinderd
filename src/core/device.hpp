/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DEVICE_CLASS_H
#define DEVICE_CLASS_H

#include <string>

#include <core/hid_interface.hpp>

enum class DeviceType {
	LogitechG105,
	LogitechG710,
	SideWinder
};

struct DeviceId {
		std::string name;
		std::string product;
		std::string vendor;
		DeviceType type;
};

class Device : public DeviceId {
	public:
		void connect();
		void disconnect();
		HidInterface hid;

	protected:
		bool isConnected_;
};

#endif
