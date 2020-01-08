/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <ctime>
#include <iostream>
#include <thread>

#include <fcntl.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <iomanip>
#include <sstream>

#include "g815.hpp"

/* constants */
constexpr auto G815_FEATURE_REPORT_LED = 0;
constexpr auto G815_LED_POS = 4;
constexpr auto G815_BUF_SIZE = 20;
constexpr auto G815_LED_M1 = 0x01;
constexpr auto G815_LED_M2 = 0x02;
constexpr auto G815_LED_M3 = 0x04;
constexpr auto G815_LED_MR = 0x01;
constexpr auto G815_KEY_M1 = 0x01;
constexpr auto G815_KEY_M2 = 0x02;
constexpr auto G815_KEY_M3 = 0x03;
constexpr auto G815_KEY_MR = 0x01;

void LogitechG815::setProfile(int profile) {
	profile_ = profile;
	switch (profile_) {
		case 0:
			ledProfile1_.on();
			break;
		case 1:
			ledProfile2_.on();
			break;
		case 2:
			ledProfile3_.on();
			break;
	}
}

/*
 * get_input() checks, which keys were pressed.
 * TODO: only return latest pressed key, if multiple keys have been pressed at the same time.
 */
struct KeyData LogitechG815::getInput() {
	struct KeyData keyData = KeyData();
	int key, nBytes;
	unsigned char buf[MAX_BUF];
	nBytes = read(fd_, buf, MAX_BUF);

	//std::cout << "LogitechG815::stream: " << "len:" << nBytes << ' ';
	//for (int i = 0; i < MAX_BUF; ++i) {
	//	std::cout << std::setw(2) << std::setfill('0') << std::hex << (int) buf[i] << std::dec << ' ';
	//}
	//std::cout << std::endl;

	if (nBytes == 8 && buf[0] == 0x11 && buf[1] == 0xff && buf[3] == 0x00) {
		/*
		 * Our task is to translate the buffer codes to something we can work with.
		 * Here is a table, where you can look up the keys and buffer, if you want
		 * to improve the current method:
		 *
		 * G1	0x11 0xff 0x0a 0x00 0x01
		 * G2	0x11 0xff 0x0a 0x00 0x02
		 * G3	0x11 0xff 0x0a 0x00 0x04
		 * G4	0x11 0xff 0x0a 0x00 0x08
		 * G5	0x11 0xff 0x0a 0x00 0x10
		 * M1	0x11 0xff 0x0b 0x00 0x01
		 * M2	0x11 0xff 0x0b 0x00 0x02
		 * M3	0x11 0xff 0x0b 0x00 0x04
		 * MR	0x11 0xff 0x0c 0x00 0x01
		 */
		if (buf[2] == 0x0a) {
			key = (static_cast<int>(buf[4]));
			key = ffs(key);
			if (key) {
				// std::cout << "LogitechG815 - GX key pressed" << std::endl;
				keyData.index = key;
				keyData.type = KeyData::KeyType::Macro;
			}
		} else if (buf[2] == 0x0b) {
			key = (static_cast<int>(buf[4]));
			key = ffs(key);
			if (key) {
				// std::cout << "LogitechG815 - MX key pressed" << std::endl;
				keyData.index = key;
				keyData.type = KeyData::KeyType::Extra;
			}
		} else if (buf[2] == 0x0c) {
			key = (static_cast<int>(buf[4]));
			key = ffs(key);
			if (key) {
				// std::cout << "LogitechG815 - MR key pressed" << std::endl;
				keyData.index = key;
				keyData.type = KeyData::KeyType::Record;
			}
		}
	}

	return keyData;
}

void LogitechG815::handleKey(struct KeyData *keyData) {
	if (keyData->index != 0) {
		// std::cout << "LogitechG815::handleKey()" << std::endl;
		if (keyData->type == KeyData::KeyType::Macro) {
			Key key(keyData);
			std::string macroPath = key.getMacroPath(profile_);
			std::thread thread(playMacro, macroPath, virtInput_);
			thread.detach();
		} else if (keyData->type == KeyData::KeyType::Extra) {
			if (keyData->index == G815_KEY_M1) {
				/* M1 key */
				setProfile(0);
			} else if (keyData->index == G815_KEY_M2) {
				/* M2 key */
				setProfile(1);
			} else if (keyData->index == G815_KEY_M3) {
				/* M3 key */
				setProfile(2);
				//} else {
				//	std::cout << "LogitechG815:: UNKNOWN KEY ???" << std::endl;
			}
		} else if (keyData->type == KeyData::KeyType::Record) {
			if (keyData->index == G815_KEY_MR) {
				/* MR key */
				handleRecordMode(&ledRecord_, G815_KEY_MR);
			//} else {
			//	 std::cout << "LogitechG815:: UNKNOWN KEY ???" << std::endl;
			}
		}
	}
}

void LogitechG815::resetMacroKeys() {
	static unsigned char buf1[G815_BUF_SIZE] = {0x11, 0xff, 0x11, 0x1b, 0x02};
	write(fd_, buf1, sizeof(buf1));
	static unsigned char buf2[G815_BUF_SIZE] = {0x11, 0xff, 0x0a, 0x2b, 0x01};
	write(fd_, buf2, sizeof(buf2));
}

LogitechG815::LogitechG815(struct Device *device,
						   sidewinderd::DevNode *devNode, libconfig::Config *config,
						   Process *process) :
		Keyboard::Keyboard(device, devNode, config, process),
		group_{&hid_},
		ledProfile1_{G815_FEATURE_REPORT_LED, G815_LED_M1, &group_},
		ledProfile2_{G815_FEATURE_REPORT_LED, G815_LED_M2, &group_},
		ledProfile3_{G815_FEATURE_REPORT_LED, G815_LED_M3, &group_},
		ledRecord_{G815_FEATURE_REPORT_LED, G815_LED_MR, &group_} {
	ledProfile1_.setLedType(LedType::Profile);
	ledProfile2_.setLedType(LedType::Profile);
	ledProfile3_.setLedType(LedType::Profile);
	ledRecord_.setLedType(LedType::Indicator);
	resetMacroKeys();

	static unsigned char reportWrite[G815_BUF_SIZE] = {0x11, 0xff, 0x0b, 0x1c, 0x00};
	ledProfile1_.registerReportWrite(reportWrite, G815_LED_POS, G815_BUF_SIZE);
	ledProfile2_.registerReportWrite(reportWrite, G815_LED_POS, G815_BUF_SIZE);
	ledProfile3_.registerReportWrite(reportWrite, G815_LED_POS, G815_BUF_SIZE);

	static unsigned char reportWriteMR[G815_BUF_SIZE] = {0x11, 0xff, 0x0c, 0x0c, 0x00};
	ledRecord_.registerReportWrite(reportWriteMR, G815_LED_POS, G815_BUF_SIZE);

	// set initial LED
	setProfile(0);

}

