/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>
#include <string>

#include <getopt.h>

#include <libconfig.h++>

#include <process.hpp>
#include <core/device_manager.hpp>

void help(std::string name) {
	std::cerr << "Usage: " << name << " [options]" << std::endl
		  << std::endl
		  << "Options:" << std::endl
		  << "  -c, --config=<file>   Override default configuration file path" << std::endl
		  << "  -d, --daemon          Run process as daemon" << std::endl
		  << "  -h, --help            Print this screen" << std::endl
		  << "  -v, --version         Print program version" << std::endl;
}

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

	if (!root.exists("capture_delays")) {
		root.add("capture_delays", libconfig::Setting::TypeBoolean) = true;
	}

	if (!root.exists("pid-file")) {
		root.add("pid-file", libconfig::Setting::TypeString) = "/var/run/sidewinderd.pid";
	}

	if (!root.exists("encrypted_workdir")) {
		root.add("encrypted_workdir", libconfig::Setting::TypeBoolean) = false;
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
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt, index = 0;
	std::string configFilePath;

	/* flags */
	bool shouldDaemonize = false;

	while ((opt = getopt_long(argc, argv, ":c:dhv", longOptions, &index)) != -1) {
		switch (opt) {
			case 'c':
				configFilePath = optarg;
				break;
			case 'd':
				shouldDaemonize = true;
				break;
			case 'h':
				help(process.getName());
				return EXIT_SUCCESS;
			case 'v':
				std::cerr << process.getName() << " " << process.getVersion() << std::endl;
				return EXIT_SUCCESS;
			case ':':
				std::cerr << "Missing argument." << std::endl;
				return EXIT_FAILURE;
			case '?':
				std::cerr << "Unknown option." << std::endl;
				return EXIT_FAILURE;
			default:
				std::cerr << "Unexpected error." << std::endl;
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

	// setting up working directory
	std::string workdir;

	if (config.exists("workdir")) {
		workdir = config.lookup("workdir").c_str();
	}

	if (process.createWorkdir(workdir, config.lookup("encrypted_workdir"))) {
		return EXIT_FAILURE;
	}

	std::clog << "Started sidewinderd." << std::endl;
	process.setActive(true);

	DeviceManager deviceManager(&config, &process);

	deviceManager.monitor();
	process.destroyPid();
	std::clog << "Stopped sidewinderd." << std::endl;

	return EXIT_SUCCESS;
}
