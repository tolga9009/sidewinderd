/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef VIRTUALINPUT_CLASS_H
#define VIRTUALINPUT_CLASS_H

#include <process.hpp>
#include <core/device_data.hpp>

/**
 * Class representing a virtual input device.
 *
 * Needed to send key events to the operating system. For Linux, uinput is used
 * as the back-end.
 */
class VirtualInput {
	public:
		void sendEvent(unsigned short type, unsigned short code, int value);
		VirtualInput(sidewinderd::DeviceData *deviceData, sidewinderd::DevNode *devNode, Process *process);
		~VirtualInput();

	private:
		int uifd_; /**< uinput device file descriptor */
		Process *process_; /**< process object for setting privileges */
		sidewinderd::DeviceData *deviceData_; /**< device information */
		sidewinderd::DevNode *devNode_; /**< device information */
		void createUidev();
};

#endif
