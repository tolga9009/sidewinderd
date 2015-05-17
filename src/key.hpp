/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef KEY_CLASS_H
#define KEY_CLASS_H

#include <string>

/**
 * Struct for storing and passing key data.
 *
 * @var index key index
 */
struct KeyData {
	int index;

	/**
	 * Enum class to classify key type.
	 * 
	 * @var Unknown not classified type
	 * @var Macro key type for macro keys, e.g. S1 on Microsoft Sidewinder X4
	 * @var Extra key type for extra keys, e.g. Bank Switch key on Microsoft
	 * Sidewinder X4
	 */
	enum class KeyType {
		Unknown,
		Macro,
		Extra
	} type;
};

/**
 * Class representing a key.
 *
 * Currently, it only works for Macro keys.
 */
class Key {
	public:
		std::string getMacroPath(int profile);
		Key(struct KeyData *keyData);

	private:
		struct KeyData *keyData_;
};

#endif
