/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef KEYBOARD_CLASS_H
#define KEYBOARD_CLASS_H

#include <string>

#include <poll.h>
#include <pwd.h>

#include <libconfig.h++>

#include "device_data.hpp"
#include "key.hpp"
#include "virtual_input.hpp"

class Keyboard {
	public:
		void listen();
		Keyboard(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, libconfig::Config *config, struct passwd *pw);
		~Keyboard();

	private:
		int profile_, autoLed_, recordLed_, macroPad_;
		int fd_, evfd_;
		struct passwd *pw_;
		struct pollfd fds[2];
		libconfig::Config *config_;
		sidewinderd::DeviceData *deviceData_;
		sidewinderd::DevNode *devNode_;
		VirtualInput *virtInput_;
		struct KeyData getInput();
		void featureRequest(unsigned char data = 0x04);
		void setupPoll();
		static void playMacro(std::string macroPath, VirtualInput *virtInput);
		void recordMacro(std::string path);
		void toggleMacroPad();
		void switchProfile();
		struct KeyData pollDevice(nfds_t nfds);
		void handleKey(struct KeyData *keyData);
		void handleRecordMode();
};

#endif
