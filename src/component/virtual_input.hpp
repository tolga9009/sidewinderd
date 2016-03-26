/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef VIRTUALINPUT_CLASS_H
#define VIRTUALINPUT_CLASS_H

#include <pwd.h>

#include <device_data.hpp>

/**
 * Class representing a virtual input device.
 *
 * Needed to send key events to the operating system. For Linux, uinput is used
 * as the back-end.
 */
class VirtualInput {
	public:
		void sendEvent(short type, short code, int value);
		VirtualInput(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, struct passwd *pw);
		~VirtualInput();

	private:
		int uifd_; /**< uinput device file descriptor */
		struct passwd *pw_; /**< configured user for permissions */
		sidewinderd::DeviceData *deviceData_; /**< device information */
		sidewinderd::DevNode *devNode_; /**< device information */
		void createUidev();
};

#endif
