/*
 * Sidewinder daemon - used for supporting the special keys of Microsoft
 * Sidewinder X4 / X6 gaming keyboards.
 *
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 * 
 * Special Thanks to Filip Wieladek, Andreas Bader and Alan Ott. Without
 * these guys, I wouldn't be able to do anything.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <locale.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

/* VIDs & PIDs*/
#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* device list */
#define DEVICE_SIDEWINDER_X6		0x00
#define DEVICE_SIDEWINDER_X4		0x01

/* constants */
#define MIN_PROFILE 0
#define MAX_PROFILE 2

/* global variables */
volatile uint8_t active = 1;

/* sidewinder device status */
struct sidewinder_data {
	uint8_t device_id;
	uint8_t profile;
	uint8_t status;
	int32_t file_desc;
	const char *device_node;
};

void switch_profile(struct sidewinder_data *sw) {
	sw->profile++;

	if (sw->profile > MAX_PROFILE) {
		sw->profile = MIN_PROFILE;
	}
}

void cleanup(struct sidewinder_data *sw) {
	free(sw);
}

void handler() {
	active = 0;
}

void setup_udev(struct sidewinder_data *sw) {
	/* udev */
	struct udev *udev;
	struct udev_device *dev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	char vendor_id[4];
	char x4_id[4];
	char x6_id[4];
	snprintf(vendor_id, sizeof(vendor_id) + 1, "%04x", VENDOR_ID_MICROSOFT);
	snprintf(x4_id, sizeof(x4_id) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X4);
	snprintf(x6_id, sizeof(x6_id) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X6);

	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		const char *temp_path;

		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		temp_path = udev_device_get_devnode(dev);
		dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

		if (strcmp(udev_device_get_sysattr_value(dev, "bInterfaceNumber"), "01") == 0) {
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

			if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vendor_id) == 0) {
				if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), x6_id) == 0) {
					sw->device_node = temp_path;
					sw->device_id = DEVICE_SIDEWINDER_X6;
				} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), x4_id) == 0) {
					sw->device_node = temp_path;
					sw->device_id = DEVICE_SIDEWINDER_X4;
				}
			}
		}

		udev_device_unref(dev);
	}

	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

int main(int argc, char **argv) {
	struct sidewinder_data *sw;
	sw = calloc(4, sizeof(struct sidewinder_data));

	signal(SIGINT, handler);
	signal(SIGHUP, handler);
	signal(SIGTERM, handler);
	signal(SIGKILL, handler);

	setup_udev(sw);

	/* TODO: main loop - watching over the device with hidraw */
	while (active) {
		
	}

	cleanup(sw);

	//printf("%s\n", sw->device_path);

	exit(0);
}
