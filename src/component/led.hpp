/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LED_CLASS_H
#define LED_CLASS_H

#include <component/hid_interface.hpp>

class LED {
	public:
		void on();
		void off();
		void blink();
		LED(unsigned char report, unsigned char led, HIDInterface *hidInterface, bool isExclusive = false, bool isSticky = false, unsigned char blink = 0);

	private:
		bool isExclusive_;
		bool isSticky_;
		unsigned char report_;
		unsigned char led_;
		unsigned char blink_;
		HIDInterface *hidInterface_;
};

#endif
