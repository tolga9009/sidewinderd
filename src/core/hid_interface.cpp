/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>

#include <linux/hidraw.h>

#include <sys/ioctl.h>

#include <core/hid_interface.hpp>

unsigned char HIDInterface::getFeatureReport(unsigned char report) {
	unsigned char buf[2] {};
	buf[0] = report;
	int ret = ioctl(*fd_, HIDIOCGFEATURE(sizeof(buf)), buf);

	if (ret < 0) {
		std::cerr << "Error getting HID feature report." << std::endl;
	} else {
		std::cout << "getFeatureReport(" << report << ") returned: " << std::hex
			<< buf[0] << " " << buf[1] <<  std::endl;
	}

	return buf[1];
}

void HIDInterface::setFeatureReport(unsigned char report, unsigned char value) {
	unsigned char buf[2];
	/* buf[0] is Report ID, buf[1] is value */
	buf[0] = report;
	buf[1] = value;
	/* TODO: check return value */
	int ret = ioctl(*fd_, HIDIOCSFEATURE(sizeof(buf)), buf);

	if (ret < 0) {
		std::cerr << "Error setting HID feature report." << std::endl;
	} else {
		std::cout << "ioctl returned: " << std::hex << buf[0] << " " << buf[1]
			<<  std::endl;
	}
}

HIDInterface::HIDInterface(int *fd) {
	fd_ = fd;
}
