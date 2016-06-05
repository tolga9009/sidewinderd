/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LOGITECH_G710_PLUS_CLASS_H
#define LOGITECH_G710_PLUS_CLASS_H

#include <process.hpp>
#include <core/keyboard.hpp>

class LogitechG710 : public Keyboard {
	public:
		LogitechG710(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void featureRequest();
		void recordMacro(std::string path);
		void handleKey(struct KeyData *keyData);
		void handleRecordMode();

	private:
		void setProfile(int profile);
		void disableGhostInput();
};

#endif
