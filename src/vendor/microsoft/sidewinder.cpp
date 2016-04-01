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

#include <vendor/microsoft/sidewinder.hpp>

void SideWinder::toggleMacroPad() {
	/* TODO: set bit, without overriding LEDs */
	macroPad_ ^= 1;
	unsigned char buf = macroPad_;
	hidInterface_.setFeatureReport(SW_FEATURE_REPORT, buf);
}

void SideWinder::switchProfile() {
	profile_ = (profile_ + 1) % MAX_PROFILE;

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
struct KeyData SideWinder::getInput() {
	struct KeyData keyData = KeyData();
	int key, nBytes;
	unsigned char buf[MAX_BUF];
	nBytes = read(fd_, buf, MAX_BUF);

	if (nBytes == 5 && buf[0] == 8) {
		/*
		 * cutting off buf[0], which is used to differentiate between macro and
		 * media keys. Our task is now to translate the buffer codes to
		 * something we can work with. Here is a table, where you can look up
		 * the keys and buffer, if you want to improve the current method:
		 *
		 * S1	0x08 0x01 0x00 0x00 0x00 - buf[1]
		 * S2	0x08 0x02 0x00 0x00 0x00 - buf[1]
		 * S3	0x08 0x04 0x00 0x00 0x00 - buf[1]
		 * S4	0x08 0x08 0x00 0x00 0x00 - buf[1]
		 * S5	0x08 0x10 0x00 0x00 0x00 - buf[1]
		 * S6	0x08 0x20 0x00 0x00 0x00 - buf[1]
		 * S7	0x08 0x40 0x00 0x00 0x00 - buf[1]
		 * S8	0x08 0x80 0x00 0x00 0x00 - buf[1]
		 * S9	0x08 0x00 0x01 0x00 0x00 - buf[2]
		 * S10	0x08 0x00 0x02 0x00 0x00 - buf[2]
		 * S11	0x08 0x00 0x04 0x00 0x00 - buf[2]
		 * S12	0x08 0x00 0x08 0x00 0x00 - buf[2]
		 * S13	0x08 0x00 0x10 0x00 0x00 - buf[2]
		 * S14	0x08 0x00 0x20 0x00 0x00 - buf[2]
		 * S15	0x08 0x00 0x40 0x00 0x00 - buf[2]
		 * S16	0x08 0x00 0x80 0x00 0x00 - buf[2]
		 * S17	0x08 0x00 0x00 0x01 0x00 - buf[3]
		 * S18	0x08 0x00 0x00 0x02 0x00 - buf[3]
		 * S19	0x08 0x00 0x00 0x04 0x00 - buf[3]
		 * S20	0x08 0x00 0x00 0x08 0x00 - buf[3]
		 * S21	0x08 0x00 0x00 0x10 0x00 - buf[3]
		 * S22	0x08 0x00 0x00 0x20 0x00 - buf[3]
		 * S23	0x08 0x00 0x00 0x40 0x00 - buf[3]
		 * S24	0x08 0x00 0x00 0x80 0x00 - buf[3]
		 * S25	0x08 0x00 0x00 0x00 0x01 - buf[4]
		 * S26	0x08 0x00 0x00 0x00 0x02 - buf[4]
		 * S27	0x08 0x00 0x00 0x00 0x04 - buf[4]
		 * S28	0x08 0x00 0x00 0x00 0x08 - buf[4]
		 * S29	0x08 0x00 0x00 0x00 0x10 - buf[4]
		 * S30	0x08 0x00 0x00 0x00 0x20 - buf[4]
		 */
		key = (static_cast<int>(buf[1]))
			| (static_cast<int>(buf[2]) << 8)
			| (static_cast<int>(buf[3]) << 16)
			| (static_cast<int>(buf[4]) << 24);
		key = ffs(key);
		keyData.index = key;
		keyData.type = KeyData::KeyType::Macro;
	} else if (nBytes == 8 && buf[0] == 1 && buf[6]) {
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		keyData.index = buf[6];
		keyData.type = KeyData::KeyType::Extra;
	}

	return keyData;
}

void SideWinder::handleKey(struct KeyData *keyData) {
	if (keyData->type == KeyData::KeyType::Macro) {
		Key key(keyData);
		std::string macroPath = key.getMacroPath(profile_);
		std::thread thread(playMacro, macroPath, virtInput_);
		thread.detach();
	} else if (keyData->type == KeyData::KeyType::Extra) {
		if (keyData->index == SW_KEY_GAMECENTER) {
			toggleMacroPad();
		} else if (keyData->index == SW_KEY_RECORD) {
			handleRecordMode();
		} else if (keyData->index == SW_KEY_PROFILE) {
			switchProfile();
		}
	}
}

void SideWinder::handleRecordMode() {
	bool isRecordMode = true;
	/* record LED solid light */
	ledRecord_.on();

	while (isRecordMode) {
		struct KeyData keyData = pollDevice(1);

		if (keyData.type == KeyData::KeyType::Macro) {
			/* record LED should blink */
			ledRecord_.blink();
			isRecordMode = false;
			Key key(&keyData);
			recordMacro(key.getMacroPath(profile_), &ledRecord_, &SW_KEY_RECORD);
		} else if (keyData.type == KeyData::KeyType::Extra) {
			/* deactivate Record LED */
			ledRecord_.off();
			isRecordMode = false;

			if (keyData.index != SW_KEY_RECORD) {
				handleKey(&keyData);
			}
		}
	}
}

SideWinder::SideWinder(sidewinderd::DeviceData *deviceData,
		sidewinderd::DevNode *devNode, libconfig::Config *config,
		Process *process) :
		Keyboard::Keyboard(deviceData, devNode, config, process),
		ledProfile1_{SW_FEATURE_REPORT, SW_LED_P1, &hidInterface_},
		ledProfile2_{SW_FEATURE_REPORT, SW_LED_P2, &hidInterface_},
		ledProfile3_{SW_FEATURE_REPORT, SW_LED_P3, &hidInterface_},
		ledRecord_{SW_FEATURE_REPORT, SW_LED_RECORD, &hidInterface_},
		ledAuto_{SW_FEATURE_REPORT, SW_LED_AUTO, &hidInterface_} {
	ledProfile1_.exclusiveOn();
}
