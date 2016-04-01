/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LOGITECH_G105_CLASS_H
#define LOGITECH_G105_CLASS_H

#include <device/keyboard.hpp>

const unsigned char G105_FEATURE_REPORT_LED = 0x06;
const unsigned char G105_FEATURE_REPORT_MACRO = 0x08;
const int G105_FEATURE_REPORT_MACRO_SIZE = 7;
const unsigned char G105_LED_M1 = 0x01;
const unsigned char G105_LED_M2 = 0x02;
const unsigned char G105_LED_M3 = 0x04;
const unsigned char G105_LED_MR = 0x08;
const int G105_KEY_M1 = 0x01;
const int G105_KEY_M2 = 0x02;
const int G105_KEY_M3 = 0x03;
const int G105_KEY_MR = 0x04;

class LogitechG105 : public Keyboard {
	public:
		LogitechG105(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, Process *process);

	protected:
		struct KeyData getInput();
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
