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

#include "device_data.hpp"

namespace sidewinderd {
	std::string version("0.3.0");
	std::atomic<bool> isRunning; /**< Sidewinderd main thread */
};

#endif
