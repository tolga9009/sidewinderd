/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LOGITECH_G103_CLASS_H
#define LOGITECH_G103_CLASS_H

#include <core/keyboard.hpp>

class LogitechG103 : public Keyboard {
	public:
		LogitechG103(struct Device *device, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void handleKey(struct KeyData *keyData);

	private:
		void setProfile(int profile);
		void resetMacroKeys();
};

#endif
