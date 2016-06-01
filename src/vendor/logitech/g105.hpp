/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LOGITECH_G105_CLASS_H
#define LOGITECH_G105_CLASS_H

#include <device/keyboard.hpp>

class LogitechG105 : public Keyboard {
	public:
		LogitechG105(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void handleKey(struct KeyData *keyData);

	private:
		LED ledProfile1_;
		LED ledProfile2_;
		LED ledProfile3_;
		LED ledRecord_;
		void setProfile(int profile);
		void resetMacroKeys();
};

#endif
