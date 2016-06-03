/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef HID_INTERFACE_CLASS_H
#define HID_INTERFACE_CLASS_H

class HIDInterface {
	public:
		unsigned char getFeatureReport(unsigned char report);
		void setFeatureReport(unsigned char report, unsigned char value);
		HIDInterface(int *fd);

	private:
		int *fd_;
};

#endif
