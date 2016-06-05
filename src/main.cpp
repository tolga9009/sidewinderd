/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <getopt.h>
#include <libudev.h>

#include <libconfig.h++>

#include <process.hpp>
#include <core/device_manager.hpp>

/* TODO: remove exceptions for better portability */
void setupConfig(libconfig::Config *config, std::string configFilePath = "/etc/sidewinderd.conf") {
	try {
		config->readFile(configFilePath.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
	} catch (const libconfig::ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
	}

	libconfig::Setting &root = config->getRoot();

	if (!root.exists("user")) {
		root.add("user", libconfig::Setting::TypeString) = "root";
	}

	if (!root.exists("profile")) {
		root.add("profile", libconfig::Setting::TypeInt) = 1;
	}

	if (!root.exists("capture_delays")) {
		root.add("capture_delays", libconfig::Setting::TypeBoolean) = true;
	}

	if (!root.exists("pid-file")) {
		root.add("pid-file", libconfig::Setting::TypeString) = "/var/run/sidewinderd.pid";
	}

	try {
		config->writeFile(configFilePath.c_str());
	} catch (const libconfig::FileIOException &fioex) {
		std::cerr << "I/O error while writing file." << std::endl;
	}
}

int main(int argc, char *argv[]) {
	/* object for managing runtime information */
	Process process;

	/* set program name */
	process.setName(argv[0]);

	/* handling command-line options */
	static struct option longOptions[] = {
		{"config", required_argument, 0, 'c'},
		{"daemon", no_argument, 0, 'd'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt, index = 0;
	std::string configFilePath;

	/* flags */
	bool shouldDaemonize = false;

	while ((opt = getopt_long(argc, argv, ":c:dv", longOptions, &index)) != -1) {
		switch (opt) {
			case 'c':
				configFilePath = optarg;
				break;
			case 'd':
				shouldDaemonize = true;
				break;
			case 'v':
				std::cout << "sidewinderd version " << process.getVersion() << std::endl;
				return EXIT_SUCCESS;
			case ':':
				std::cout << "Missing argument." << std::endl;
				break;
			case '?':
				std::cout << "Unrecognized option." << std::endl;
				break;
			default:
				std::cout << "Unexpected error." << std::endl;
				return EXIT_FAILURE;
		}
	}

	/* reading config file */
	libconfig::Config config;

	if (configFilePath.empty()) {
		setupConfig(&config);
	} else {
		setupConfig(&config, configFilePath);
	}

	/* daemonize, if flag has been set */
	if (shouldDaemonize) {
		int ret = process.daemonize();

		if (ret > 0) {
			return EXIT_SUCCESS;
		} else if (ret < 0) {
			return EXIT_FAILURE;
		}
	}

	/* creating pid file for single instance mechanism */
	if (process.createPid(config.lookup("pid-file"))) {
		return EXIT_FAILURE;
	}

	/* setting gid and uid to configured user */
	process.applyUser(config.lookup("user"));

	/* creating sidewinderd directory in user's home directory */
	process.createWorkdir();

	std::clog << "Started sidewinderd." << std::endl;

	DeviceManager deviceManager(&process);

	// TODO implement delays: config_->lookup("capture_delays")

	while(process.isActive()) {
		deviceManager.monitor();
	}

	process.destroyPid();
	std::clog << "Stopped sidewinderd." << std::endl;

	return EXIT_SUCCESS;
}
