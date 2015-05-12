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
#define MAX_BUF		8
#define MIN_PROFILE	0
#define MAX_PROFILE	3

/* media keys */
#define EXTRA_KEY_GAMECENTER	0x10
#define EXTRA_KEY_RECORD		0x11
#define EXTRA_KEY_PROFILE		0x14

void Keyboard::feature_request(unsigned char data) {
	unsigned char buf[2];
	/* buf[0] is Report Number, buf[1] is the control byte */
	buf[0] = 0x7;
	buf[1] = data << profile;
	buf[1] |= macropad;
	buf[1] |= record_led << 5;

	ioctl(fd, HIDIOCSFEATURE(sizeof(buf)), buf);
}

void Keyboard::setup_poll() {
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* ignore second fd for now */
	fds[1].fd = -1;
	fds[1].events = POLLIN;
}

void Keyboard::toggle_macropad() {
	macropad ^= 1;
	feature_request();
}

void Keyboard::switch_profile() {
	profile = (profile + 1) % MAX_PROFILE;
	feature_request();
}

/*
 * get_input() checks, which keys were pressed. The macro keys are packed in a
 * 5-byte buffer, media keys (including Bank Switch and Record) use 8-bytes.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been pressed at
 * the same time.
 */
struct KeyData Keyboard::get_input() {
	struct KeyData kd = {0, KeyData::KeyType::Unknown};
	int key, nbytes;
	unsigned char buf[MAX_BUF];

	nbytes = read(fd, buf, MAX_BUF);

	if (nbytes == 5 && buf[0] == 8) {
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
		kd.Index = key;
		kd.Type = KeyData::KeyType::Macro;
	} else if (nbytes == 8 && buf[0] == 1 && buf[6]) {
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		kd.Index = buf[6];
		kd.Type = KeyData::KeyType::Extra;
	}

	return kd;
}

/* TODO: interrupt and exit play_macro when any macro_key has been pressed */
void Keyboard::play_macro(std::string path, VirtualInput *virtinput) {
	tinyxml2::XMLDocument doc;

	doc.LoadFile(path.c_str());

	if(!doc.ErrorID()) {
		tinyxml2::XMLElement* root = doc.FirstChildElement("Macro");

		for (tinyxml2::XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (child->Name() == std::string("KeyBoardEvent")) {
				bool boolDown;
				int key = std::atoi(child->GetText());

				child->QueryBoolAttribute("Down", &boolDown);
				virtinput->send_event(EV_KEY, key, boolDown);
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
void Keyboard::record_macro(std::string path) {
	bool run = true;
	struct timeval prev;
	struct KeyData kd;

	prev.tv_usec = 0;
	prev.tv_sec = 0;

	std::cout << "Start Macro Recording on " << data->devnode.input_event << std::endl;

	seteuid(0);
	evfd = open(data->devnode.input_event.c_str(), O_RDONLY | O_NONBLOCK);
	seteuid(pw->pw_uid);

	if (evfd < 0) {
		std::cout << "Can't open input event file" << std::endl;
	}

	/* additionally monitor /dev/input/event* with poll */
	fds[1].fd = evfd;

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLNode* root = doc.NewElement("Macro");

	/* start root element "Macro" */
	doc.InsertFirstChild(root);

	while (run) {
		kd = check(2);

		if (kd.Index == EXTRA_KEY_RECORD && kd.Type == KeyData::KeyType::Extra) {
			record_led = 0;
			feature_request();
			run = false;
		}

		struct input_event inev;

		read(evfd, &inev, sizeof(struct input_event));

		if (inev.type == 1 && inev.value != 2) {

			/* only capturing delays, if capture_delays is set to true */
			if (prev.tv_usec && config->lookup("capture_delays")) {
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
	close(evfd);
}

void Keyboard::process_input(struct KeyData *kd) {
	if (kd->Type == KeyData::KeyType::Macro) {
		Key mkey(kd);
		std::string path = mkey.GetMacroPath(profile);
		std::thread t(play_macro, path, virtinput);
		t.detach();
	} else if (kd->Type == KeyData::KeyType::Extra) {
		if (kd->Index == EXTRA_KEY_GAMECENTER) {
			toggle_macropad();
		} else if (kd->Index == EXTRA_KEY_RECORD) {
			record_mode_handler();
		} else if (kd->Index == EXTRA_KEY_PROFILE) {
			switch_profile();
		}
	}
}

void Keyboard::record_mode_handler() {
	bool run = true;
	record_led = 3;
	feature_request();

	while (run) {
		struct KeyData kd = check(1);

		if (kd.Type == KeyData::KeyType::Macro) {
			record_led = 2;
			feature_request();
			run = false;
			Key mkey(&kd);
			record_macro(mkey.GetMacroPath(profile));
		} else if (kd.Type == KeyData::KeyType::Extra) {
			/* deactivate Record LED */
			record_led = 0;
			feature_request();
			run = false;

			if (kd.Index != EXTRA_KEY_RECORD) {
				process_input(&kd);
			}
		}
	}
}

struct KeyData Keyboard::check(nfds_t nfds) {
	/*
	 * poll() checks the device for any activities and blocks the loop. This
	 * leads to a very efficient polling mechanism.
	 */
	poll(fds, nfds, -1);
	struct KeyData kd = get_input();

	return kd;
}

void Keyboard::listen() {
	struct KeyData kd = check(1);
	process_input(&kd);
}

Keyboard::Keyboard(struct sidewinderd::DeviceData *data, libconfig::Config *config, struct passwd *pw) {
	Keyboard::config = config;
	Keyboard::pw = pw;
	Keyboard::data = data;
	Keyboard::virtinput = new VirtualInput(data, pw);
	profile = 0;
	auto_led = 0;
	record_led = 0;
	macropad = 0;

	for (int i = MIN_PROFILE; i < MAX_PROFILE; i++) {
		std::stringstream path;

		path << "profile_" << i + 1;
		mkdir(path.str().c_str(), S_IRWXU);
	}

	/* open file descriptor with root privileges */
	seteuid(0);
	fd = open(data->devnode.hidraw.c_str(), O_RDWR | O_NONBLOCK);
	seteuid(pw->pw_uid);

	/* TODO: throw exception if interface can't be accessed, call destructor */
	if (fd < 0) {
		std::cout << "Can't open hidraw interface" << std::endl;
	}

	feature_request();

	setup_poll();
}

Keyboard::~Keyboard() {
	delete virtinput;
	record_led = 0;
	feature_request(0);
	close(fd);
}
