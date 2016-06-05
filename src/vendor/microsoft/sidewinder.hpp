/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef MICROSOFT_SIDEWINDER_CLASS_H
#define MICROSOFT_SIDEWINDER_CLASS_H

#include <core/keyboard.hpp>

class SideWinder : public Keyboard {
	public:
		SideWinder(Process *process);

	protected:
		struct KeyData getInput();
		void handleKey(struct KeyData *keyData);

	private:
		unsigned char macroPad_;
		Led ledProfile1_;
		Led ledProfile2_;
		Led ledProfile3_;
		Led ledRecord_;
		Led ledAuto_;
		void toggleMacroPad();
		void switchProfile();
};

#endif
