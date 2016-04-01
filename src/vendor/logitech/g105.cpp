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

#include <vendor/logitech/g105.hpp>

void LogitechG105::setProfile(int profile) {
	profile_ = profile;

	switch (profile_) {
		case 0: ledProfile1_.exclusiveOn();
		case 1: ledProfile2_.exclusiveOn();
		case 2: ledProfile3_.exclusiveOn();
	}
}

/*
 * get_input() checks, which keys were pressed. The macro keys are packed in a
 * 5-byte buffer, media keys (including Bank Switch and Record) use 8-bytes.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been pressed at
 * the same time.
 */
struct KeyData LogitechG105::getInput() {
	struct KeyData keyData = KeyData();
	int key, nBytes;
	unsigned char buf[MAX_BUF];
	nBytes = read(fd_, buf, MAX_BUF);

	if (nBytes == 3 && buf[0] == 0x03) {
		/*
		 * cutting off buf[0], which is used to differentiate between macro and
		 * media keys. Our task is now to translate the buffer codes to
		 * something we can work with. Here is a table, where you can look up
		 * the keys and buffer, if you want to improve the current method:
		 *
		 * G1	0x03 0x01 0x00 - buf[1]
		 * G2	0x03 0x02 0x00 - buf[1]
		 * G3	0x03 0x04 0x00 - buf[1]
		 * G4	0x03 0x08 0x00 - buf[1]
		 * G5	0x03 0x10 0x00 - buf[1]
		 * G6	0x03 0x20 0x00 - buf[1]
		 * M1	0x03 0x00 0x01 - buf[2]
		 * M2	0x03 0x00 0x02 - buf[2]
		 * M3	0x03 0x00 0x04 - buf[2]
		 * MR	0x03 0x00 0x08 - buf[2]
		 */
		if (buf[2] == 0) {
			key = (static_cast<int>(buf[1]));
			key = ffs(key);
			keyData.index = key;
			keyData.type = KeyData::KeyType::Macro;
		} else if (buf[1] == 0) {
			key = (static_cast<int>(buf[2]));
			key = ffs(key);
			keyData.index = key;
			keyData.type = KeyData::KeyType::Extra;
		}
	}

	return keyData;
}

void LogitechG105::handleKey(struct KeyData *keyData) {
	if (keyData->index != 0) {
		if (keyData->type == KeyData::KeyType::Macro) {
			Key key(keyData);
			std::string macroPath = key.getMacroPath(profile_);
			std::thread thread(playMacro, macroPath, virtInput_);
			thread.detach();
		} else if (keyData->type == KeyData::KeyType::Extra) {
			if (keyData->index == G105_KEY_M1) {
				/* M1 key */
				setProfile(0);
			} else if (keyData->index == G105_KEY_M2) {
				/* M2 key */
				setProfile(1);
			} else if (keyData->index == G105_KEY_M3) {
				/* M3 key */
				setProfile(2);
			} else if (keyData->index == G105_KEY_MR) {
				/* MR key */
				handleRecordMode();
			}
		}
	}
}

void LogitechG105::handleRecordMode() {
	bool isRecordMode = true;
	ledRecord_.on();

	while (isRecordMode) {
		struct KeyData keyData = pollDevice(1);

		if (keyData.index != 0) {
			if (keyData.type == KeyData::KeyType::Macro) {
				ledRecord_.on();
				isRecordMode = false;
				Key key(&keyData);
				recordMacro(key.getMacroPath(profile_), &ledRecord_, &G105_KEY_MR);
			} else if (keyData.type == KeyData::KeyType::Extra) {
				/* deactivate Record LED */
				ledRecord_.off();
				isRecordMode = false;

				if (keyData.index != G105_KEY_MR) {
					handleKey(&keyData);
				}
			}
		}
	}
}

void LogitechG105::resetMacroKeys() {
	/* we need to zero out the report, so macro keys don't emit numbers */
	unsigned char buf[G105_FEATURE_REPORT_MACRO_SIZE] = {};
	/* buf[0] is Report ID */
	buf[0] = G105_FEATURE_REPORT_MACRO;
	ioctl(fd_, HIDIOCSFEATURE(sizeof(buf)), buf);
}

LogitechG105::LogitechG105(sidewinderd::DeviceData *deviceData,
		sidewinderd::DevNode *devNode, libconfig::Config *config,
		Process *process) :
		Keyboard::Keyboard(deviceData, devNode, config, process),
		ledProfile1_{G105_FEATURE_REPORT_LED, G105_LED_M1, &hidInterface_},
		ledProfile2_{G105_FEATURE_REPORT_LED, G105_LED_M2, &hidInterface_},
		ledProfile3_{G105_FEATURE_REPORT_LED, G105_LED_M3, &hidInterface_},
		ledRecord_{G105_FEATURE_REPORT_LED, G105_LED_MR, &hidInterface_} {
	resetMacroKeys();
	/* TODO: read from config */
	/* set initial LED */
	ledProfile1_.exclusiveOn();
}