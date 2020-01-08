/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <unistd.h>

#include <iostream>

#include <linux/hidraw.h>

#include <sys/ioctl.h>

#include <core/hid_interface.hpp>

unsigned char HidInterface::getReport(unsigned char report) {
	unsigned char buf[2] {};
	buf[0] = report;
	int ret = ioctl(*fd_, HIDIOCGFEATURE(sizeof(buf)), buf);

	if (ret < 0) {
		std::cerr << "Error getting HID feature report." << std::endl;
	} else {
		std::cout << "getFeatureReport(" << static_cast<int>(report)
			  << ") returned: " << std::hex
			  << static_cast<int>(buf[0]) << " "
			  << static_cast<int>(buf[1]) <<  std::endl;
	}

	return buf[1];
}

void HidInterface::setReport(unsigned char report, unsigned char value) {
	unsigned char buf[2];
	/* buf[0] is Report ID, buf[1] is value */
	buf[0] = report;
	buf[1] = value;
	/* TODO: check return value */
	int ret = ioctl(*fd_, HIDIOCSFEATURE(sizeof(buf)), buf);

	if (ret < 0) {
		std::cerr << "Error setting HID feature report." << std::endl;
	}
}

void HidInterface::writeData(unsigned char *buf, unsigned int size) {
	int ret = write(*fd_, buf, size);
	if (ret < 0) {
		std::cerr << "Error writing to HID." << std::endl;
	}
}

HidInterface::HidInterface(int *fd) {
	fd_ = fd;
}
