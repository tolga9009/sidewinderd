/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
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

/* global variables */
namespace sidewinderd {
	std::string version("0.2.0");
	std::atomic<bool> state;

	std::vector<std::pair<std::string, std::string>> devices = {
		{"045e", "074b"},	/* Microsoft Sidewinder X6 */
		{"045e", "0768"}	/* Microsoft Sidewinder X4 */
	};
};

#endif
