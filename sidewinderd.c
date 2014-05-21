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
#include <libudev.h>
#include <locale.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

/* VIDs & PIDs*/
#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* device list */
#define DEVICE_SIDEWINDER_X6	0x01
#define DEVICE_SIDEWINDER_X4	0x02

/* constants */
#define MAX_BUF		8
#define SIZE_SKEYS	5
#define SIZE_MKEYS	8
#define MIN_PROFILE	0
#define MAX_PROFILE	2
#define SIDEWINDER_X6_MAX_SKEYS	30
#define SIDEWINDER_X4_MAX_SKEYS	6

/* special keys */
#define SKEY_S01	0x1
#define SKEY_S02	0x1 << 1
#define SKEY_S03	0x1 << 2
#define SKEY_S04	0x1 << 3
#define SKEY_S05	0x1 << 4
#define SKEY_S06	0x1 << 5
#define SKEY_S07	0x1 << 6
#define SKEY_S08	0x1 << 7
#define SKEY_S09	0x1 << 8
#define SKEY_S10	0x1 << 9
#define SKEY_S11	0x1 << 10
#define SKEY_S12	0x1 << 11
#define SKEY_S13	0x1 << 12
#define SKEY_S14	0x1 << 13
#define SKEY_S15	0x1 << 14
#define SKEY_S16	0x1 << 15
#define SKEY_S17	0x1 << 16
#define SKEY_S18	0x1 << 17
#define SKEY_S19	0x1 << 18
#define SKEY_S20	0x1 << 19
#define SKEY_S21	0x1 << 20
#define SKEY_S22	0x1 << 21
#define SKEY_S23	0x1 << 22
#define SKEY_S24	0x1 << 23
#define SKEY_S25	0x1 << 24
#define SKEY_S26	0x1 << 25
#define SKEY_S27	0x1 << 26
#define SKEY_S28	0x1 << 27
#define SKEY_S29	0x1 << 28
#define SKEY_S30	0x1 << 29

/* media keys */
#define MKEY_GAMECENTER		0x10
#define MKEY_RECORD			0x11
#define MKEY_PROFILE		0x14

/* global variables */
volatile uint8_t active = 1;

/* sidewinder device status */
/* TODO: make this struct globally available */
struct sidewinder_data {
	uint8_t device_id;
	uint8_t profile;
	uint8_t status;
	int32_t fd;
	const char *device_node;
};

void cleanup(struct sidewinder_data *sw) {
	close(sw->fd);
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

	char vid_microsoft[4], pid_sidewinder_x6[4], pid_sidewinder_x4[4];

	/* converting integers to strings */
	snprintf(vid_microsoft, sizeof(vid_microsoft) + 1, "%04x", VENDOR_ID_MICROSOFT);
	snprintf(pid_sidewinder_x6, sizeof(pid_sidewinder_x6) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X6);
	snprintf(pid_sidewinder_x4, sizeof(pid_sidewinder_x4) + 1, "%04x", PRODUCT_ID_SIDEWINDER_X4);

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
		const char *path, *temp_path;

		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		temp_path = udev_device_get_devnode(dev);
		dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

		if (strcmp(udev_device_get_sysattr_value(dev, "bInterfaceNumber"), "01") == 0) {
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

			if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vid_microsoft) == 0) {
				if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x6) == 0) {
					sw->device_node = temp_path;
					sw->device_id = DEVICE_SIDEWINDER_X6;
				} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x4) == 0) {
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

void setup_hidraw(struct sidewinder_data *sw) {
	/* O_SYNC for write operation? */
	sw->fd = open(sw->device_node, O_RDWR | O_NONBLOCK);

	if (sw->fd < 0) {
		printf("Can't open hidraw interface");
		exit(1);
	}
}

void feature_request(struct sidewinder_data *sw, uint8_t request) {
	unsigned char buf[2];

	/* buf[0] is Report Number, buf[1] is the control byte */
	buf[0] = 0x7;
	buf[1] = request;
	ioctl(sw->fd, HIDIOCSFEATURE(2), buf);
}

void switch_profile(struct sidewinder_data *sw) {
	uint8_t status;
	sw->profile++;

	if (sw->profile > MAX_PROFILE) {
		sw->profile = MIN_PROFILE;
	}

	status = 0x04 << sw->profile;

	feature_request(sw, status);
}

void send_key(uint32_t skey) {
	printf("key pressed: %x\n", skey);
}

/* TODO: improve multiple inputs */
void process_input(struct sidewinder_data *sw, uint8_t nbytes, unsigned char *buf) {
	if (nbytes == SIZE_SKEYS && buf[0] == 8) {
		int i;

		/* cutting off buf[0] */
		for (i = 1; i < nbytes; i++) {
			uint32_t key;

			if (buf[i]) {
				int j;

				key = buf[i] << (8 * (i - 1));

				for (j = 0; j < SIDEWINDER_X4_MAX_SKEYS; j++) {
					int skey = 1 << j;

					if (key & skey) {
						send_key(skey);
					}
				}

				if (sw->device_id == DEVICE_SIDEWINDER_X6) {
					for (j = SIDEWINDER_X4_MAX_SKEYS; j < SIDEWINDER_X6_MAX_SKEYS; j++) {
						int skey = 1 << j;

						if (key & skey) {
							send_key(skey);
						}
					}
				}
			}
		}
	/* TODO: use send_key() on keypress */
	} else if (nbytes == SIZE_MKEYS && buf[0] == 1) {
		int i;
		uint32_t key;
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		key = buf[6];

		switch(key) {
			case MKEY_GAMECENTER: printf("Game Center pressed\n");	break;
			case MKEY_RECORD: printf("Record pressed\n");			break;
			case MKEY_PROFILE: switch_profile(sw);					break;
		}
	}
}

int main(int argc, char **argv) {
	uint8_t nbytes;
	uint32_t epfd;
	unsigned char buf[MAX_BUF];
	struct sidewinder_data *sw;
	struct epoll_event ev;

	sw = calloc(4, sizeof(struct sidewinder_data));
	epfd = epoll_create1(0);
	if (epfd == -1) {
		error("epoll_create");
	}

	signal(SIGINT, handler);
	signal(SIGHUP, handler);
	signal(SIGTERM, handler);
	signal(SIGKILL, handler);

	setup_udev(sw);
	setup_hidraw(sw);

	/* setting initial profile */
	sw->profile = 0;
	feature_request(sw, 0x4 << sw->profile);

	ev.data.fd = sw->fd;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sw->fd, &ev);

	while (active) {
		epoll_wait(epfd, &ev, 1, -1);

		nbytes = read(sw->fd, buf, MAX_BUF);

		process_input(sw, nbytes, buf);
	}

	cleanup(sw);

	exit(0);
}
