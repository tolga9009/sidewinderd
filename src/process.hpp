/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef PROCESS_CLASS_H
#define PROCESS_CLASS_H

#include <string>

#include <pwd.h>

#include <libconfig.h++>

class Process {
	public:
		int daemonize();
		int createPid();
		void closePid(int pidFd);
		void applyUser();
		void createWorkdir();
		void privilege();
		void unprivilege();
		Process(libconfig::Config *config);

	private:
		std::string pidFilePath_;
		std::string user_;
		struct passwd *pw_;
		libconfig::Config *config_;
};

#endif
