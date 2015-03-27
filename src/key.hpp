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
	int Index;
	enum class KeyType {
		Unknown,
		Macro,
		Extra
	} Type;
};

class Key {
	public:
		struct KeyData *kd;
		std::string GetMacroPath(int profile);
		Key(struct KeyData *kd);
	private:
		bool active, macro_active;;
};

#endif
