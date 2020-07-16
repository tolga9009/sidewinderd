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

#include "g103.hpp"

/* constants */
constexpr auto G103_FEATURE_REPORT_MACRO =	0x08;
constexpr auto G103_FEATURE_REPORT_MACRO_SIZE =	7;

void LogitechG103::setProfile(int profile) {
	profile_ = profile;
}

/*
 * get_input() checks, which keys were pressed. The macro keys are packed in a
 * 3-byte buffer.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been pressed at
 * the same time.
 */
struct KeyData LogitechG103::getInput() {
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
		 */
		if (buf[2] == 0) {
			key = (static_cast<int>(buf[1]));
			key = ffs(key);

			if (key) {
				keyData.index = key;
				keyData.type = KeyData::KeyType::Macro;
			}
		}
	}

	return keyData;
}

void LogitechG103::handleKey(struct KeyData *keyData) {
	if (keyData->index != 0) {
		if (keyData->type == KeyData::KeyType::Macro) {
			Key key(keyData);
			std::string macroPath = key.getMacroPath(profile_);
			std::thread thread(playMacro, macroPath, virtInput_);
			thread.detach();
		}
	}
}

void LogitechG103::resetMacroKeys() {
	/* we need to zero out the report, so macro keys don't emit F-keys */
	unsigned char buf[G103_FEATURE_REPORT_MACRO_SIZE] = {};
	/* buf[0] is Report ID */
	buf[0] = G103_FEATURE_REPORT_MACRO;
	ioctl(fd_, HIDIOCSFEATURE(sizeof(buf)), buf);
}

LogitechG103::LogitechG103(struct Device *device,
		sidewinderd::DevNode *devNode, libconfig::Config *config,
		Process *process) :
		Keyboard::Keyboard(device, devNode, config, process) {
	resetMacroKeys();

	// set profile to default, as no profile switching is supported
	setProfile(0);
}

LogitechG103::~LogitechG103() {
	std::cerr << "LogitechG103 Destructor" << std::endl;

	// keyboard is not connected anymore, join the thread
	if (listenThread_.joinable()) {
		listenThread_.join();
	}
}
