/*
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>

#include <fcntl.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "keyboard.hpp"

/* constants */
const int MAX_BUF = 8;
const int MIN_PROFILE = 0;
const int MAX_PROFILE = 3;
/* media keys */
const int EXTRA_KEY_GAMECENTER = 0x10;
const int EXTRA_KEY_RECORD = 0x11;
const int EXTRA_KEY_PROFILE = 0x14;

void Keyboard::featureRequest(unsigned char data) {
	unsigned char buf[2];
	/* buf[0] is Report Number, buf[1] is the control byte */
	buf[0] = 0x7;
	buf[1] = data << profile_;
	buf[1] |= macroPad_;
	buf[1] |= recordLed_ << 5;
	ioctl(fd_, HIDIOCSFEATURE(sizeof(buf)), buf);
}

void Keyboard::setupPoll() {
	fds[0].fd = fd_;
	fds[0].events = POLLIN;
	/* ignore second fd for now */
	fds[1].fd = -1;
	fds[1].events = POLLIN;
}

void Keyboard::toggleMacroPad() {
	macroPad_ ^= 1;
	featureRequest();
}

void Keyboard::switchProfile() {
	profile_ = (profile_ + 1) % MAX_PROFILE;
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
struct KeyData Keyboard::getInput() {
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

/* TODO: interrupt and exit play_macro when any macro_key has been pressed */
void Keyboard::playMacro(std::string macroPath, VirtualInput *virtInput) {
	tinyxml2::XMLDocument xmlDoc;
	xmlDoc.LoadFile(macroPath.c_str());

	if(!xmlDoc.ErrorID()) {
		tinyxml2::XMLElement* root = xmlDoc.FirstChildElement("Macro");

		for (tinyxml2::XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (child->Name() == std::string("KeyBoardEvent")) {
				bool isPressed;
				int key = std::atoi(child->GetText());
				child->QueryBoolAttribute("Down", &isPressed);
				virtInput->sendEvent(EV_KEY, key, isPressed);
			} else if (child->Name() == std::string("DelayEvent")) {
				int delay = std::atoi(child->GetText());
				struct timespec request, remain;
				/*
				 * value is given in milliseconds, so we need to split it into
				 * seconds and nanoseconds. nanosleep() is interruptable and saves
				 * the remaining sleep time.
				 */
				request.tv_sec = delay / 1000;
				delay = delay - (request.tv_sec * 1000);
				request.tv_nsec = 1000000L * delay;
				nanosleep(&request, &remain);
			}
		}
	}
}

/*
 * Macro recording captures delays by default. Use the configuration to disable
 * capturing delays.
 */
void Keyboard::recordMacro(std::string path) {
	struct timeval prev;
	struct KeyData keyData;
	prev.tv_usec = 0;
	prev.tv_sec = 0;
	std::cout << "Start Macro Recording on " << deviceData_->devNode.inputEvent << std::endl;
	seteuid(0);
	evfd_ = open(deviceData_->devNode.inputEvent.c_str(), O_RDONLY | O_NONBLOCK);
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

		if (keyData.index == EXTRA_KEY_RECORD && keyData.type == KeyData::KeyType::Extra) {
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

void Keyboard::handleKey(struct KeyData *keyData) {
	if (keyData->type == KeyData::KeyType::Macro) {
		Key key(keyData);
		std::string macroPath = key.getMacroPath(profile_);
		std::thread thread(playMacro, macroPath, virtInput_);
		thread.detach();
	} else if (keyData->type == KeyData::KeyType::Extra) {
		if (keyData->index == EXTRA_KEY_GAMECENTER) {
			toggleMacroPad();
		} else if (keyData->index == EXTRA_KEY_RECORD) {
			handleRecordMode();
		} else if (keyData->index == EXTRA_KEY_PROFILE) {
			switchProfile();
		}
	}
}

void Keyboard::handleRecordMode() {
	bool isRecordMode = true;
	recordLed_ = 3;
	featureRequest();

	while (isRecordMode) {
		struct KeyData keyData = pollDevice(1);

		if (keyData.type == KeyData::KeyType::Macro) {
			recordLed_ = 2;
			featureRequest();
			isRecordMode = false;
			Key key(&keyData);
			recordMacro(key.getMacroPath(profile_));
		} else if (keyData.type == KeyData::KeyType::Extra) {
			/* deactivate Record LED */
			recordLed_ = 0;
			featureRequest();
			isRecordMode = false;

			if (keyData.index != EXTRA_KEY_RECORD) {
				handleKey(&keyData);
			}
		}
	}
}

struct KeyData Keyboard::pollDevice(nfds_t nfds) {
	/*
	 * poll() checks the device for any activities and blocks the loop. This
	 * leads to a very efficient polling mechanism.
	 */
	poll(fds, nfds, -1);
	struct KeyData keyData = getInput();

	return keyData;
}

void Keyboard::listen() {
	struct KeyData keyData = pollDevice(1);
	handleKey(&keyData);
}

Keyboard::Keyboard(struct sidewinderd::DeviceData *deviceData, libconfig::Config *config, struct passwd *pw) {
	config_ = config;
	pw_ = pw;
	deviceData_ = deviceData;
	virtInput_ = new VirtualInput(deviceData_, pw);
	profile_ = 0;
	autoLed_ = 0;
	recordLed_ = 0;
	macroPad_ = 0;

	for (int i = MIN_PROFILE; i < MAX_PROFILE; i++) {
		std::stringstream profileFolderPath;
		profileFolderPath << "profile_" << i + 1;
		mkdir(profileFolderPath.str().c_str(), S_IRWXU);
	}

	/* open file descriptor with root privileges */
	seteuid(0);
	fd_ = open(deviceData_->devNode.hidraw.c_str(), O_RDWR | O_NONBLOCK);
	seteuid(pw_->pw_uid);

	/* TODO: throw exception if interface can't be accessed, call destructor */
	if (fd_ < 0) {
		std::cout << "Can't open hidraw interface" << std::endl;
	}

	featureRequest();
	setupPoll();
}

Keyboard::~Keyboard() {
	delete virtInput_;
	recordLed_ = 0;
	featureRequest(0);
	close(fd_);
}
