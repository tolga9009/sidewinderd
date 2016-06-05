/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LED_GROUP_CLASS_H
#define LED_GROUP_CLASS_H

#include <core/hid_interface.hpp>

class LedGroup {
	public:
		unsigned char getIndicatorMask();
		void setIndicatorMask(unsigned char indicator);
		HidInterface *getHidInterface();
		LedGroup(HidInterface *hid);

	private:
		unsigned char indicator_;
		HidInterface *hid_;
};

#endif
