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
	/* TODO: some keyboards have a built-in value for record LED, others need a
	 * timer, turning record LED on and off.
	 */
	if (hasBlink_) {
		/* TODO: implementation */
	}
}

LED::LED(unsigned char report, unsigned char led, HIDInterface *hidInterface, bool isExclusive, bool isSticky, bool hasBlink) {
	report_ = report;
	led_ = led;
	hidInterface_ = hidInterface;
	isExclusive_ = isExclusive;
	isSticky_ = isSticky;
	hasBlink_ = hasBlink;
}
