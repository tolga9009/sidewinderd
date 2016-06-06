/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
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

bool Keyboard::isConnected() {
	return isConnected_;
}

void Keyboard::connect() {
	isConnected_ = true;
}

void Keyboard::disconnect() {
	isConnected_ = false;
}

void Keyboard::setupPoll() {
	fds[0].fd = fd_;
	fds[0].events = POLLIN;
	/* ignore second fd for now */
	fds[1].fd = -1;
	fds[1].events = POLLIN;
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
void Keyboard::recordMacro(std::string path, Led *ledRecord, const int keyRecord) {
	struct timeval prev;
	struct KeyData keyData;
	prev.tv_usec = 0;
	prev.tv_sec = 0;
	std::cout << "Start Macro Recording on " << devNode_->inputEvent << std::endl;
	process_->privilege();
	evfd_ = open(devNode_->inputEvent.c_str(), O_RDONLY | O_NONBLOCK);
	process_->unprivilege();

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

		if (keyData.index == keyRecord && keyData.type == KeyData::KeyType::Extra) {
			ledRecord->off();
			isRecordMode = false;
		}

		struct input_event inev;
		read(evfd_, &inev, sizeof(struct input_event));

		if (inev.type == EV_KEY && inev.value != 2) {
			/* only capturing delays, if capture_delays is set to true */
			if (prev.tv_usec && config_->lookup("capture_delays")) {
				auto diff = (inev.time.tv_usec + 1000000 * inev.time.tv_sec) - (prev.tv_usec + 1000000 * prev.tv_sec);
				auto delay = diff / 1000;
				/* start element "DelayEvent" */
				tinyxml2::XMLElement* DelayEvent = doc.NewElement("DelayEvent");
				DelayEvent->SetText(static_cast<int>(delay));
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

struct KeyData Keyboard::pollDevice(nfds_t nfds) {
	/*
	 * poll() checks the device for any activities and blocks the loop. This
	 * leads to a very efficient polling mechanism.
	 */
	poll(fds, nfds, -1);

	// check, if device has been disconnected
	if (fds->revents & POLLHUP || fds->revents & POLLERR) {
		disconnect();

		return KeyData();
	}

	struct KeyData keyData = getInput();

	return keyData;
}

void Keyboard::listen() {
	struct KeyData keyData = pollDevice(1);
	handleKey(&keyData);
}

void Keyboard::handleRecordMode(Led *ledRecord, const int keyRecord) {
	bool isRecordMode = true;
	/* record LED solid light */
	ledRecord->on();

	while (isRecordMode) {
		struct KeyData keyData = pollDevice(1);

		if (keyData.type == KeyData::KeyType::Macro) {
			/* record LED should blink */
			ledRecord->blink();
			isRecordMode = false;
			Key key(&keyData);
			recordMacro(key.getMacroPath(profile_), ledRecord, keyRecord);
		} else if (keyData.type == KeyData::KeyType::Extra) {
			/* deactivate Record LED */
			ledRecord->off();
			isRecordMode = false;

			if (keyData.index != keyRecord) {
				handleKey(&keyData);
			}
		}
	}
}

Keyboard::Keyboard(sidewinderd::DeviceData *deviceData,
		sidewinderd::DevNode *devNode, libconfig::Config *config,
		Process *process) : hid_{&fd_} {
	config_ = config;
	process_ = process;
	deviceData_ = deviceData;
	devNode_ = devNode;
	virtInput_ = new VirtualInput(deviceData_, devNode_, process_);
	profile_ = 0;
	isConnected_ = true;

	for (int i = MIN_PROFILE; i < MAX_PROFILE; i++) {
		std::stringstream profileFolderPath;
		profileFolderPath << "profile_" << i + 1;
		mkdir(profileFolderPath.str().c_str(), S_IRWXU);
	}

	/* open file descriptor with root privileges */
	process_->privilege();
	fd_ = open(devNode->hidraw.c_str(), O_RDWR | O_NONBLOCK);
	process_->unprivilege();

	/* TODO: destruct, if interface can't be accessed */
	if (fd_ < 0) {
		std::cout << "Can't open hidraw interface" << std::endl;
	}

	setupPoll();
}

Keyboard::~Keyboard() {
	delete virtInput_;
	close(fd_);
}
