/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LOGITECH_G710_CLASS_H
#define LOGITECH_G710_CLASS_H

#include <device/keyboard.hpp>

const unsigned char G710_FEATURE_REPORT_LED = 0x06;
const unsigned char G710_FEATURE_REPORT_MACRO = 0x09;
const int G710_FEATURE_REPORT_MACRO_SIZE = 13;
const unsigned char G710_LED_M1 = 0x10;
const unsigned char G710_LED_M2 = 0x20;
const unsigned char G710_LED_M3 = 0x40;
const unsigned char G710_LED_MR = 0x80;

class LogitechG710 : public Keyboard {
	public:
		LogitechG710(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
		void recordMacro(std::string path);
		void handleKey(struct KeyData *keyData);
		void handleRecordMode();

	private:
		LED ledProfile1_;
		LED ledProfile2_;
		LED ledProfile3_;
		LED ledRecord_;
		void setProfile(int profile);
		void resetMacroKeys();
};

#endif
