/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <core/led_group.hpp>

unsigned char LedGroup::getIndicatorMask() {
	return indicator_;
}

void LedGroup::setIndicatorMask(unsigned char indicator) {
	indicator_ = indicator;
}

HidInterface *LedGroup::getHidInterface() {
	return hid_;
}

LedGroup::LedGroup(HidInterface *hid) {
	hid_ = hid;
	indicator_ = 0;
}
