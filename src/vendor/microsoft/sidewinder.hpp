/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef MICROSOFT_SIDEWINDER_CLASS_H
#define MICROSOFT_SIDEWINDER_CLASS_H

#include <device/keyboard.hpp>

const unsigned char SW_FEATURE_REPORT = 0x07;
const unsigned char SW_LED_AUTO = 0x02;
const unsigned char SW_LED_P1 = 0x04;
const unsigned char SW_LED_P2 = 0x08;
const unsigned char SW_LED_P3 = 0x10;
const unsigned char SW_LED_RECORD = 0x60;
const unsigned char SW_LED_RECORD_BLINK = 0x40;
const int SW_KEY_GAMECENTER = 0x10;
const int SW_KEY_RECORD = 0x11;
const int SW_KEY_PROFILE = 0x14;

class SideWinder : public Keyboard {
	public:
		SideWinder(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void handleKey(struct KeyData *keyData);

	private:
		int macroPad_;
		LED ledProfile1_;
		LED ledProfile2_;
		LED ledProfile3_;
		LED ledRecord_;
		LED ledAuto_;
		void toggleMacroPad();
		void switchProfile();
};

#endif
