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
		unsigned char getReport(unsigned char report);
		void setReport(unsigned char report, unsigned char value);
		void writeData(unsigned char *buf, unsigned int size);
		HidInterface(int *fd);

	private:
		int *fd_;
};

#endif
