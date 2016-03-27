/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef LED_CLASS_H
#define LED_CLASS_H

class LED {
	public:
		void on();
		void off();
		void blink();
		LED(int bit);

	private:
		void privateStuff();
};

#endif
