/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>
#include <sstream>

#include <core/key.hpp>

/**
 * Assembles relative path to Macro file.
 */
std::string Key::getMacroPath(int profile) {
	std::stringstream macroPath;
	macroPath << "profile_" << profile + 1 << "/" << "s" << keyData_->index << ".xml";

	return macroPath.str();
}

/**
 * Constructor initializing structs.
 */
Key::Key(struct KeyData *keyData) {
	keyData_ = keyData;
}
