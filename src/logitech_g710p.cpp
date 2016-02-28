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

#include "logitech_g710p.hpp"

void LogitechG710::featureRequest() {
	unsigned char buf[2];
	/* buf[0] is Report ID, buf[1] is value */
	buf[0] = 0x6;
	buf[1] = 0x10 << profile_;
	buf[1] |= recordLed_ << 7;
	ioctl(fd_, HIDIOCSFEATURE(sizeof(buf)), buf);
}

void LogitechG710::setProfile(int profile) {
	profile_ = profile;
	featureRequest();
}

/*
 * get_input() checks, which keys were pressed. The macro keys are packed in a
 * 5-byte buffer, media keys (including Bank Switch and Record) use 8-bytes.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been pressed at
 * the same time.
 */
struct KeyData LogitechG710::getInput() {
	struct KeyData keyData = KeyData();
	int key, nBytes;
	unsigned char buf[MAX_BUF];
	nBytes = read(fd_, buf, MAX_BUF);

	if (nBytes == 4 && buf[0] == 0x03) {
		/*
		 * cutting off buf[0], which is used to differentiate between macro and
		 * media keys. Our task is now to translate the buffer codes to
		 * something we can work with. Here is a table, where you can look up
		 * the keys and buffer, if you want to improve the current method:
		 *
		 * G1	0x03 0x01 0x00 0x00 - buf[1]
		 * G2	0x03 0x02 0x00 0x00 - buf[1]
		 * G3	0x03 0x04 0x00 0x00 - buf[1]
		 * G4	0x03 0x08 0x00 0x00 - buf[1]
		 * G5	0x03 0x10 0x00 0x00 - buf[1]
		 * G6	0x03 0x20 0x00 0x00 - buf[1]
		 * M1	0x03 0x00 0x10 0x00 - buf[2]
		 * M2	0x03 0x00 0x20 0x00 - buf[2]
		 * M3	0x03 0x00 0x40 0x00 - buf[2]
		 * MR	0x03 0x00 0x80 0x00 - buf[2]
		 */
		if (buf[2] == 0) {
			key = (static_cast<int>(buf[1]));
			key = ffs(key);
			keyData.index = key;
			keyData.type = KeyData::KeyType::Macro;
		} else if (buf[1] == 0) {
			key = (static_cast<int>(buf[2])) >> 4;
			key = ffs(key);
			keyData.index = key;
			keyData.type = KeyData::KeyType::Extra;
		}
	}

	return keyData;
}

/*
 * Macro recording captures delays by default. Use the configuration to disable
 * capturing delays.
 */
void LogitechG710::recordMacro(std::string path) {
	struct timeval prev;
	struct KeyData keyData;
	prev.tv_usec = 0;
	prev.tv_sec = 0;
	std::cout << "Start Macro Recording on " << devNode_->inputEvent << std::endl;
	seteuid(0);
	evfd_ = open(devNode_->inputEvent.c_str(), O_RDONLY | O_NONBLOCK);
	seteuid(pw_->pw_uid);

	if (evfd_ < 0) {
		std::cout << "Can't open input event file" << std::endl;
	}

	/* additionally monitor /dev/input/event* with poll */
	fds[1].fd = evfd_;
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLNode* root = doc.NewElement("Macro");
	/* start root element "Macro" */
	doc.InsertFirstChild(root);

	bool isRecordMode = true;

	while (isRecordMode) {
		keyData = pollDevice(2);

		if (keyData.index == 4 && keyData.type == KeyData::KeyType::Extra) {
			recordLed_ = 0;
			featureRequest();
			isRecordMode = false;
		}

		struct input_event inev;
		read(evfd_, &inev, sizeof(struct input_event));

		if (inev.type == EV_KEY && inev.value != 2) {
			/* only capturing delays, if capture_delays is set to true */
			if (prev.tv_usec && config_->lookup("capture_delays")) {
				long int diff = (inev.time.tv_usec + 1000000 * inev.time.tv_sec) - (prev.tv_usec + 1000000 * prev.tv_sec);
				int delay = diff / 1000;
				/* start element "DelayEvent" */
				tinyxml2::XMLElement* DelayEvent = doc.NewElement("DelayEvent");
				DelayEvent->SetText(delay);
				root->InsertEndChild(DelayEvent);
			}

			/* start element "KeyBoardEvent" */
			tinyxml2::XMLElement* KeyBoardEvent = doc.NewElement("KeyBoardEvent");

			if (inev.value) {
				KeyBoardEvent->SetAttribute("Down", true);
			} else {
				KeyBoardEvent->SetAttribute("Down", false);
			}

			KeyBoardEvent->SetText(inev.code);
			root->InsertEndChild(KeyBoardEvent);
			prev = inev.time;
		}
	}

	/* write XML document */
	if (doc.SaveFile(path.c_str())) {
		std::cout << "Error XML SaveFile" << std::endl;
	}

	std::cout << "Exit Macro Recording" << std::endl;
	/* remove event file from poll fds */
	fds[1].fd = -1;
	close(evfd_);
}

void LogitechG710::handleKey(struct KeyData *keyData) {
	if (keyData->index != 0) {
		if (keyData->type == KeyData::KeyType::Macro) {
			Key key(keyData);
			std::string macroPath = key.getMacroPath(profile_);
			std::thread thread(playMacro, macroPath, virtInput_);
			thread.detach();
		} else if (keyData->type == KeyData::KeyType::Extra) {
			if (keyData->index == 1) {
				/* M1 key */
				setProfile(0);
			} else if (keyData->index == 2) {
				/* M2 key */
				setProfile(1);
			} else if (keyData->index == 3) {
				/* M3 key */
				setProfile(2);
			} else if (keyData->index == 4) {
				/* MR key */
				handleRecordMode();
			}
		}
	}
}

void LogitechG710::handleRecordMode() {
	bool isRecordMode = true;
	recordLed_ = 1;
	featureRequest();

	while (isRecordMode) {
		struct KeyData keyData = pollDevice(1);

		if (keyData.index != 0) {
			if (keyData.type == KeyData::KeyType::Macro) {
				recordLed_ = 1;
				featureRequest();
				isRecordMode = false;
				Key key(&keyData);
				recordMacro(key.getMacroPath(profile_));
			} else if (keyData.type == KeyData::KeyType::Extra) {
				/* deactivate Record LED */
				recordLed_ = 0;
				featureRequest();
				isRecordMode = false;

				if (keyData.index != 4) {
					handleKey(&keyData);
				}
			}
		}
	}
}

void LogitechG710::disableGhostInput() {
	/* we need to zero out the report, so macro keys don't emit numbers */
	unsigned char buf[13] = {};
	/* buf[0] is Report ID */
	buf[0] = 0x9;
	ioctl(fd_, HIDIOCSFEATURE(sizeof(buf)), buf);
}

LogitechG710::LogitechG710(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, struct passwd *pw) : Keyboard::Keyboard(deviceData, devNode, config, pw) {
	disableGhostInput();
	featureRequest();
}
