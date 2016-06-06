/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>
#include <core/led.hpp>

void Led::on() {
	auto buf = hid_->getReport(report_);

	if (type_ == LedType::Profile) {
		// clear out all LEDs, but Indicator LEDs
		buf &= group_->getIndicatorMask();
	}

	buf |= led_;
	hid_->setReport(report_, buf);
}

void Led::off() {
	auto buf = hid_->getReport(report_);
	buf &= ~led_;
	hid_->setReport(report_, buf);
}

void Led::blink() {
	if (blink_) {
		auto buf = hid_->getReport(report_);
		buf &= ~led_;
		buf |= blink_;
		hid_->setReport(report_, buf);
	} else {
		/*
		 * TODO Implement non-blocking software emulated blink, using
		 * std::thread and std::chrono. It should start blinking, when
		 * Led::blink() has been called and stop, when Led::off() has
		 * been called. 1 second on, 1 second off sounds like a good
		 * rhythm. It would be perfect, if one could interrupt the blink
		 * anytime, even in a this_thread::wait_for().
		 */
	}
}

void Led::registerBlink(unsigned char led) {
	blink_ = led;
}

void Led::setLedType(LedType type) {
	type_ = type;

	if (type_ == LedType::Indicator) {
		auto indicator= group_->getIndicatorMask();
		indicator |= led_;
		group_->setIndicatorMask(indicator);
	}
}

Led::Led(unsigned char report, unsigned char led, LedGroup *group) {
	report_ = report;
	led_ = led;
	group_ = group;
	blink_ = 0;
	type_ = LedType::Common;
	hid_ = group_->getHidInterface();
}
