/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LED_CLASS_H
#define LED_CLASS_H

#include <core/hid_interface.hpp>
#include <core/led_group.hpp>

enum class LedType {
	Common,		// common LEDs don't have special handling
	Profile,	// profile LEDs are exclusively lit
	Indicator	// indicator LEDs are not allowed to be overwritten
};

class Led {
	public:
		/**
		 * Turns LED on.
		 */
		void on();

		/**
		 * Turns LED off.
		 */
		void off();

		/**
		 * Sets LED to blinking mode. Falls back to software emulation,
		 * if there is no hardware support.
		 */
		void blink();

		/**
		 * If LED supports blinking via hardware, set report ID and
		 * value using this function.
		 * @param led blink specific HID report value
		 */
		void registerBlink(unsigned char led);

		/**
		 * Sets LedType.
		 * @param type LedType can be Common, Profile or Indicator.
		 */
		void setLedType(LedType type);
		Led(unsigned char report, unsigned char led, LedGroup *group);

	private:
		unsigned char report_;
		unsigned char led_;
		unsigned char blink_;
		LedGroup *group_;
		LedType type_;
		HidInterface *hid_;
};

#endif
