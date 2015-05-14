/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef KEY_CLASS_H
#define KEY_CLASS_H

#include <string>

struct KeyData {
	int index;

	enum class KeyType {
		Unknown,
		Macro,
		Extra
	} type;
};

class Key {
	public:
		std::string getMacroPath(int profile);
		Key(struct KeyData *keyData);

	private:
		struct KeyData *keyData_;
};

#endif
