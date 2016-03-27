/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <process.hpp>

int Process::daemonize() {
	pid_t pid, sid;
	pid = fork();

	if (pid < 0) {
		std::cerr << "Error creating daemon." << std::endl;
		return -1;
	}

	if (pid > 0) {
		return 1;
	}

	sid = setsid();

	if (sid < 0) {
		std::cerr << "Error setting sid." << std::endl;
		return -1;
	}

	pid = fork();

	if (pid < 0) {
		std::cerr << "Error forking second time." << std::endl;
		return -1;
	}

	if (pid > 0) {
		return 1;
	}

	umask(0);
	chdir("/");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	return 0;
}

int Process::createPid() {
	int pidFd = open(pidFilePath_.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (pidFd < 0) {
		std::cerr << "PID file could not be created." << std::endl;
		return -1;
	}

	if (flock(pidFd, LOCK_EX | LOCK_NB) < 0) {
		std::cerr << "Could not lock PID file, another instance is already running. Terminating." << std::endl;
		close(pidFd);
		return -1;
	}

	return pidFd;
}

void Process::closePid(int pidFd) {
	flock(pidFd, LOCK_UN);
	close(pidFd);
	unlink(pidFilePath_.c_str());
}

void Process::applyUser() {
	setegid(pw_->pw_gid);
	seteuid(pw_->pw_uid);
}

void Process::createWorkdir () {
	/* creating sidewinderd directory in user's home directory */
	std::string workdir = pw_->pw_dir;
	std::string xdgData;

	if (const char *env = std::getenv("XDG_DATA_HOME")) {
		xdgData = env;

		if (!xdgData.empty()) {
			workdir = xdgData;
		}
	} else {
		xdgData = "/.local/share";
		workdir.append(xdgData);
	}

	workdir.append("/sidewinderd");
	mkdir(workdir.c_str(), S_IRWXU);

	if (chdir(workdir.c_str())) {
		std::cerr << "Error accessing working directory." << std::endl;
	}
}

void Process::privilege() {
	seteuid(0);
}

void Process::unprivilege() {
	seteuid(pw_->pw_uid);
}

Process::Process(libconfig::Config *config) {
	config_ = config;
	/* assign configuration values */
	std::string pidFilePath = config_->lookup("pid-file");
	pidFilePath_ = pidFilePath;
	std::string user = config_->lookup("user");
	user_ = user;
	pw_ = getpwnam(user_.c_str());
}
