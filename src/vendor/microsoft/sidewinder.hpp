/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef MICROSOFT_SIDEWINDER_CLASS_H
#define MICROSOFT_SIDEWINDER_CLASS_H

#include <device/keyboard.hpp>

class SideWinder : public Keyboard {
	public:
		SideWinder(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void handleKey(struct KeyData *keyData);

	private:
		unsigned char macroPad_;
		LED ledProfile1_;
		LED ledProfile2_;
		LED ledProfile3_;
		LED ledRecord_;
		LED ledAuto_;
		void toggleMacroPad();
		void switchProfile();
};

#endif
