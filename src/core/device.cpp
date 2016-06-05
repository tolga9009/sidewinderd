/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <core/device.hpp>

void Device::connect() {
	isConnected_ = true;
}

void Device::disconnect() {
	isConnected_ = false;
}
