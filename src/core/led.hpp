/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LED_CLASS_H
#define LED_CLASS_H

#include <core/hid_interface.hpp>

enum class LedType {
	Unknown,
	Profile,
	Record
};

class Led {
	public:
		void on();
		void off();
		void blink();
		// TODO wtf, this needs to be shorter!
		Led(LedType type, unsigned char report, unsigned char led, HidInterface *hidInterface, unsigned char blink = 0);

	protected:
		bool isExclusive_;
		bool isSticky_;
		unsigned char report_;
		unsigned char led_;
		unsigned char blink_;
		LedType type_;
		HidInterface *hidInterface_;
};

#endif
