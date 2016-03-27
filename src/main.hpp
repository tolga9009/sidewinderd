/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef MAIN_H
#define MAIN_H

#include <atomic>
#include <iostream>
#include <string>

namespace sidewinderd {
	std::string version("0.3.1");
	std::atomic<bool> isRunning; /**< Sidewinderd main thread */
};

#endif
