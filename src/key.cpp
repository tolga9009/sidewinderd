/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>
#include <string>
#include <sstream>

#include "key.hpp"

/* Returns path to XML, containing macro instructions */
std::string Key::GetMacroPath(int profile) {
	std::stringstream path;
	path << "profile_" << profile + 1 << "/" << "s" << kd->Index << ".xml";

	return path.str();
}

Key::Key(struct KeyData *kd) {
	Key::kd = kd;
}
