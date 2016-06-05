/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef HID_INTERFACE_CLASS_H
#define HID_INTERFACE_CLASS_H

#include <string>

class HidInterface {
	public:
		unsigned char getFeatureReport(unsigned char report);
		void setFeatureReport(unsigned char report, unsigned char value);
		std::string getEvent();
		void setEvent(std::string node);
		std::string getHidraw();
		void setHidraw(std::string node);
		HidInterface();

	private:
		int fd_;
		std::string event_;
		std::string hidraw_;
};

#endif
