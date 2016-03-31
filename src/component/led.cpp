/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <component/led.hpp>

void LED::on() {
}

void LED::exclusiveOn() {
}

void LED::off() {
}

void LED::blink() {
}

LED::LED(unsigned char report, unsigned char led, HIDInterface *hidInterface) {
	report_ = report;
	led_ = led;
	hidInterface_ = hidInterface;
}
