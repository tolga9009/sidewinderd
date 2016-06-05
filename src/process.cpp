/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <csignal>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "process.hpp"

/* constants */
constexpr auto version = "0.3.1";

std::atomic<bool> Process::isActive_;

bool Process::isActive() {
	return isActive_;
}

void Process::setActive(bool isActive) {
	isActive_ = isActive;
}

std::string Process::getName() {
	if (name_.empty()) {
		name_ = "sidewinderd";
	}

	return name_;
}

void Process::setName(std::string name) {
	name_ = name;
}

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

int Process::createPid(std::string pidPath) {
	pidPath_ = pidPath;
	pidFd_ = open(pidPath_.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (pidFd_ < 0) {
		std::cerr << "PID file could not be created." << std::endl;

		return -1;
	}

	if (flock(pidFd_, LOCK_EX | LOCK_NB) < 0) {
		std::cerr << "Could not lock PID file, another instance is already running." << std::endl;
		close(pidFd_);

		return -1;
	}

	hasPid_ = true;

	return 0;
}

void Process::destroyPid() {
	if (pidFd_) {
		flock(pidFd_, LOCK_UN);
		close(pidFd_);
	}

	if (hasPid_) {
		unlink(pidPath_.c_str());
		hasPid_ = false;
	}
}

void Process::applyUser(std::string user) {
	user_ = user;
	pw_ = getpwnam(user_.c_str());
	setegid(pw_->pw_gid);
	seteuid(pw_->pw_uid);
}

void Process::createWorkdir() {
	if (user_.empty()) {
		return;
	}

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
	if (user_.empty()) {
		return;
	}

	seteuid(pw_->pw_uid);
}

std::string Process::getVersion() {
	return version;
}

void Process::sigHandler(int sig) {
	std::cerr << std::endl << "Stop signal received." << std::endl;

	switch(sig) {
		case SIGINT:
			setActive(false);
			break;
		case SIGTERM:
			setActive(false);
			break;
	}
}

Process::Process() {
	isActive_ = false;
	hasPid_ = false;
	pidFd_ = 0;

	/* signal handling */
	struct sigaction action;
	action.sa_handler = sigHandler;
	sigaction(SIGINT, &action, nullptr);
	sigaction(SIGTERM, &action, nullptr);
}

Process::~Process() {
	destroyPid();
}
