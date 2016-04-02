/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <component/led.hpp>

void LED::on() {
	/* TODO: dont override sticky LEDs */
	if (isExclusive_) {
		hidInterface_->setFeatureReport(report_, led_);
	} else {
		unsigned char buf = hidInterface_->getFeatureReport(report_);
		buf |= led_;
		hidInterface_->setFeatureReport(report_, buf);
	}
}

void LED::off() {
	unsigned char buf = hidInterface_->getFeatureReport(report_);
	buf &= ~led_;
	hidInterface_->setFeatureReport(report_, buf);
}

void LED::blink() {
	if (blink_) {
		unsigned char buf = hidInterface_->getFeatureReport(report_);
		buf &= ~led_;
		buf |= blink_;
		hidInterface_->setFeatureReport(report_, buf);
	} else {
		/* TODO: implement record LED using a timer */
	}
}

LED::LED(unsigned char report, unsigned char led, HIDInterface *hidInterface, bool isExclusive, bool isSticky, unsigned char blink) {
	report_ = report;
	led_ = led;
	hidInterface_ = hidInterface;
	isExclusive_ = isExclusive;
	isSticky_ = isSticky;
	blink_ = blink;
}
