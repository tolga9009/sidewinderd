/*
 * Sidewinder daemon - support for Microsoft Sidewinder X4 / X6 keyboards
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <libconfig.h>
#include <libudev.h>
#include <locale.h>
#include <time.h>

#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libxml/encoding.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

/* VIDs & PIDs*/
#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* constants */
#define MAX_BUF		8
#define MAX_EVENTS	2
#define MIN_PROFILE	0
#define MAX_PROFILE	2
#define SIDEWINDER_X6_MAX_SKEYS	30
#define SIDEWINDER_X4_MAX_SKEYS	6

/* media keys */
#define MKEY_GAMECENTER		0x10
#define MKEY_RECORD			0x11
#define MKEY_PROFILE		0x14

/* global variables */
volatile uint8_t active = 1;
int32_t fd, uifd, epfd, evfd;

/* global structs */
struct uinput_user_dev *uidev;
struct input_event *inev;
struct epoll_event *epev;
struct config_t *cfg;
struct sidewinder_data {
	uint16_t device_id;
	uint8_t profile;
	uint8_t auto_led;
	uint8_t record_led;	/* 0: off, 1: breath, 2: blink, 3: solid */
	uint8_t max_skeys;
	uint8_t macropad;
	char *devnode_hidraw;
	char *devnode_input;
} *sw;

void handler() {
	active = 0;
}

void setup_udev() {
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
	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *syspath, *devnode_path;
		syspath = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, syspath);

		if (strcmp(udev_device_get_subsystem(dev), "hidraw") == 0) {
			devnode_path = udev_device_get_devnode(dev);
			dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");

			if (!dev) {
				printf("Unable to find parent device\n");
				exit(1);
			}

			if (strcmp(udev_device_get_sysattr_value(dev, "bInterfaceNumber"), "01") == 0) {
				dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

				if (strcmp(udev_device_get_sysattr_value(dev, "idVendor"), vid_microsoft) == 0) {
					if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x6) == 0) {
						sw->devnode_hidraw = strdup(devnode_path);
						sw->device_id = PRODUCT_ID_SIDEWINDER_X6;
					} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x4) == 0) {
						sw->devnode_hidraw = strdup(devnode_path);
						sw->device_id = PRODUCT_ID_SIDEWINDER_X4;
					}
				}
			}
		}

		/* find correct /dev/input/event* file */
		if (strcmp(udev_device_get_subsystem(dev), "input") == 0
			&& udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD") != NULL
			&& strstr(syspath, "event")
			&& udev_device_get_parent_with_subsystem_devtype(dev, "usb", NULL)) {
				sw->devnode_input = strdup(udev_device_get_devnode(dev));
		}

		udev_device_unref(dev);
	}

	/* free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

void setup_hidraw() {
	fd = open(sw->devnode_hidraw, O_RDWR | O_NONBLOCK);

	if (fd < 0) {
		printf("Can't open hidraw interface");
		exit(1);
	}
}

void setup_uidev() {
	int i;
	uidev = calloc(7, sizeof(struct uinput_user_dev));
	inev = calloc(3, sizeof(struct input_event));
	uifd = open("/dev/uinput", O_WRONLY);

	if (uifd < 0) {
		uifd = open("/dev/input/uinput", O_WRONLY);

		if (uifd < 0) {
			printf("Can't open uinput");
			exit(1);
		}
	}

	/*
	 * TODO: dynamically get and setbit needed keys by play_macro()
	 *
	 * Currently, we set all keybits, to make things easier.
	 */
	ioctl(uifd, UI_SET_EVBIT, EV_KEY);
	ioctl(uifd, UI_SET_EVBIT, EV_SYN);

	for (i = KEY_ESC; i <= KEY_KPDOT; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (i = KEY_ZENKAKUHANKAKU; i <= KEY_F24; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	for (i = KEY_PLAYCD; i <= KEY_MICMUTE; i++) {
		ioctl(uifd, UI_SET_KEYBIT, i);
	}

	/* our uinput device's details */
	/* TODO: read keyboard information via udev and set config here */
	snprintf(uidev->name, UINPUT_MAX_NAME_SIZE, "sidewinderd");
	uidev->id.bustype = BUS_USB;
	uidev->id.vendor = 0x1;
	uidev->id.product = 0x1;
	uidev->id.version = 1;
	write(uifd, uidev, sizeof(struct uinput_user_dev));
	ioctl(uifd, UI_DEV_CREATE);
}

void setup_epoll() {
	epev = calloc(2, sizeof(struct epoll_event));
	epfd = epoll_create1(0);
	epev->data.fd = fd;
	epev->events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, epev);
}

void setup_config() {
	int ret, i, j;
	struct config_setting_t *root, *group, *setting;
	/* TODO: use string operators for building strings */
	const char *cfg_file = "sidewinderd.conf";
	cfg = calloc(12, sizeof(struct config_t));
	config_init(cfg);
	root = config_root_setting(cfg);

	/* default global settings */
	setting = config_setting_add(root, "user", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "nobody");
	setting = config_setting_add(root, "group", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "nobody");
	setting = config_setting_add(root, "folder_path", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "~/.sidewinderd/");
	setting = config_setting_add(root, "profile", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 1);
	setting = config_setting_add(root, "capture_delays", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, 1);
	setting = config_setting_add(root, "ms_compat_mode", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, 0);
	setting = config_setting_add(root, "save_profile", CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, 0);

	/* read user config */
	if (config_read_file(cfg, cfg_file) == CONFIG_FALSE) {
		/* if no user config is available, write new config */
		if (config_write_file(cfg, cfg_file) == CONFIG_FALSE) {
			/* TODO: error handling */
		}
	}
}

void feature_request() {
	unsigned char buf[2];
	/* buf[0] is Report Number, buf[1] is the control byte */
	buf[0] = 0x7;
	buf[1] = 0x04 << sw->profile;
	buf[1] |= sw->macropad;
	buf[1] |= sw->record_led << 5;

	/* reset LEDs on program exit */
	if (!active) {
		buf[1] = 0;
	}

	ioctl(fd, HIDIOCSFEATURE(sizeof(buf)), buf);
}

void toggle_macropad() {
	sw->macropad ^= 1;
	feature_request();
}

void switch_profile() {
	sw->profile++;

	if (sw->profile > MAX_PROFILE) {
		sw->profile = MIN_PROFILE;
	}

	feature_request();
}

void setup_device() {
	sw->profile = 0;
	sw->auto_led = 0;
	sw->record_led = 0;

	if (sw->device_id == PRODUCT_ID_SIDEWINDER_X4) {
		sw->max_skeys = SIDEWINDER_X4_MAX_SKEYS;
	}

	if (sw->device_id == PRODUCT_ID_SIDEWINDER_X6) {
		sw->max_skeys = SIDEWINDER_X6_MAX_SKEYS;
	}

	sw->macropad = 0;
}

void send_key(uint16_t type, uint16_t code, int32_t value) {
	inev->type = type;
	inev->code = code;
	inev->value = value;
	write(uifd, inev, sizeof(struct input_event));

	inev->type = EV_SYN;
	inev->code = 0;
	inev->value = 0;
	write(uifd, inev, sizeof(struct input_event));
}

/*
 * get_input() checks, which keys were pressed. The macro keys are
 * packed in a 5-byte buffer, media keys (including Bank Switch and
 * Record) use 8-bytes.
 */
/*
 * TODO: only return latest pressed key, if multiple keys have been
 * pressed at the same time.
 */
unsigned char get_input(void) {
	int i;
	unsigned char key, nbytes, buf[MAX_BUF];

	nbytes = read(fd, buf, MAX_BUF);

	if (nbytes == 5 && buf[0] == 8) {
		/*
		 * cutting off buf[0], which is used to differentiate between
		 * macro and media keys. Our task is now to translate the buffer
		 * codes to something we can work with. Here is a table, where
		 * you can look up the keys and buffer, if you want to improve
		 * the current method:
		 *
		 * S1	0x08 0x01 0x00 0x00 0x00 - buf[1]
		 * S2	0x08 0x02 0x00 0x00 0x00 - buf[1]
		 * S3	0x08 0x04 0x00 0x00 0x00 - buf[1]
		 * S4	0x08 0x08 0x00 0x00 0x00 - buf[1]
		 * S5	0x08 0x10 0x00 0x00 0x00 - buf[1]
		 * S6	0x08 0x20 0x00 0x00 0x00 - buf[1]
		 * S7	0x08 0x40 0x00 0x00 0x00 - buf[1]
		 * S8	0x08 0x80 0x00 0x00 0x00 - buf[1]
		 * S9	0x08 0x00 0x01 0x00 0x00 - buf[2]
		 * S10	0x08 0x00 0x02 0x00 0x00 - buf[2]
		 * S11	0x08 0x00 0x04 0x00 0x00 - buf[2]
		 * S12	0x08 0x00 0x08 0x00 0x00 - buf[2]
		 * S13	0x08 0x00 0x10 0x00 0x00 - buf[2]
		 * S14	0x08 0x00 0x20 0x00 0x00 - buf[2]
		 * S15	0x08 0x00 0x40 0x00 0x00 - buf[2]
		 * S16	0x08 0x00 0x80 0x00 0x00 - buf[2]
		 * S17	0x08 0x00 0x00 0x01 0x00 - buf[3]
		 * S18	0x08 0x00 0x00 0x02 0x00 - buf[3]
		 * S19	0x08 0x00 0x00 0x04 0x00 - buf[3]
		 * S20	0x08 0x00 0x00 0x08 0x00 - buf[3]
		 * S21	0x08 0x00 0x00 0x10 0x00 - buf[3]
		 * S22	0x08 0x00 0x00 0x20 0x00 - buf[3]
		 * S23	0x08 0x00 0x00 0x40 0x00 - buf[3]
		 * S24	0x08 0x00 0x00 0x80 0x00 - buf[3]
		 * S25	0x08 0x00 0x00 0x00 0x01 - buf[4]
		 * S26	0x08 0x00 0x00 0x00 0x02 - buf[4]
		 * S27	0x08 0x00 0x00 0x00 0x04 - buf[4]
		 * S28	0x08 0x00 0x00 0x00 0x08 - buf[4]
		 * S29	0x08 0x00 0x00 0x00 0x10 - buf[4]
		 * S30	0x08 0x00 0x00 0x00 0x20 - buf[4]
		 */
		for (i = 1; i < nbytes; i++) {
			int j;

			for (j = 0; buf[i]; j++) {
				int k;
				key = ((i - 1) * 8) + ffs(buf[i]);

				return key;

				buf[i] &= buf[i] - 1;
			}
		}
	} else if (nbytes == 8 && buf[0] == 1) {
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		key = buf[6];

		/* TODO: recognize media keys and calc key */
		switch (key) {
			case MKEY_GAMECENTER: return sw->max_skeys + 1;	break;
			case MKEY_RECORD: return sw->max_skeys + 2;		break;
			case MKEY_PROFILE: return sw->max_skeys + 3;		break;
		}
	}

	return 0;
}

/*
 * Returns path to XML, containing macro instructions. asprintf()
 * allocates memory, so the caller needs to free memory.
 */
char *get_xmlpath(unsigned char key) {
	char *path;

	asprintf(&path, "p%d/s%02d.xml", sw->profile + 1, key);

	return path;
}

/* TODO: interrupt and exit play_macro when a key is pressed */
/* BUG: if Bank Switch is pressed during play_macro(), crazy things happen */
/* TODO: block play_macro, if any key has been pressed during execution */
void play_macro(unsigned char key) {
	xmlTextReaderPtr reader;
	int ret;
	char *path = get_xmlpath(key);
	reader = xmlReaderForFile(path, NULL, 0);
	free(path);

	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);

		while (ret == 1 && active) {
			const xmlChar *name, *attr = NULL;
			int value, down = 0;

			if (xmlTextReaderNodeType(reader) == XML_ELEMENT_NODE) {
				name = xmlTextReaderConstName(reader);

				if (xmlTextReaderHasAttributes(reader)) {
					attr = xmlTextReaderGetAttribute(reader, "Down");

					/* TODO: more elegant way; also check for false */
					if (!strcmp(attr, "true")) {
						down = 1;
					} else {
						down = 0;
					}
				}

				/*
				 * TODO: error handling - wrong XML format leads to
				 * infinite loop
				 */
				while (!xmlTextReaderHasValue(reader) && active) {
					ret = xmlTextReaderRead(reader);
				}

				/*
				 * xmlTextReaderConstValue() returns a string, which we
				 * need to convert to an integer.
				 */
				value = strtol(xmlTextReaderConstValue(reader), NULL, 10);

				if (!strcmp(name, "KeyBoardEvent")) {
					send_key(EV_KEY, value, down);
				} else if (!strcmp(name, "DelayEvent")) {
					struct timespec request, remain;
					/*
					 * value is given in milliseconds, so we need to
					 * split it into seconds and nanoseconds.
					 * nanosleep() is interruptable and saves the
					 * remaining sleep time.
					 */
					request.tv_sec = value / 1000;
					value = value - (request.tv_sec * 1000);
					request.tv_nsec = 1000000L * value;
					nanosleep(&request, &remain);
				}
			}

			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			printf("parse failed");
		}
	}
}

/*
 * Macro recording captures delays by default. Use the configuration
 * to disable capturing delays.
 */
/*
 * TODO: abort Macro recording, if Record key is pressed again.
 */
void record_macro() {
	uint8_t nbytes;
	char tmp[10];
	char *path;
	unsigned char buf[MAX_BUF];
	int rc, run, i;
	xmlTextWriterPtr writer;
	struct timeval prev;

	prev.tv_usec = 0;
	prev.tv_sec = 0;
	run = 1;
	sw->record_led = 3;
	feature_request();

	while (active && run) {
		unsigned char key;
		epoll_wait(epfd, epev, MAX_EVENTS, -1);
		key = get_input();

		if (key && key < sw->max_skeys) {
			path = get_xmlpath(key);
			sw->record_led = 2;
			run = 0;
			feature_request();
		}
	}

	printf("Start Macro Recording\n");

	evfd = open(sw->devnode_input, O_RDONLY | O_NONBLOCK);

	if (evfd < 0) {
		printf("Can't open input event file");
		exit(1);
	}

	/* add /dev/input/event* to epoll watchlist */
	epoll_ctl(epfd, EPOLL_CTL_ADD, evfd, epev);

	/* create a new xmlWriter with no compression */
	writer = xmlNewTextWriterFilename(path, 0);
	free(path);

	/* activate indentation */
	rc = xmlTextWriterSetIndent(writer, 1);

	/* set indentation character */
	rc = xmlTextWriterSetIndentString(writer, "\t");

	/* start document with defaults */
	rc = xmlTextWriterStartDocument(writer, NULL, NULL, NULL);

	/* start root element "Macro" */
	rc = xmlTextWriterStartElement(writer, BAD_CAST "Macro");

	run = 1;

	while (active && run) {
		unsigned char key;
		epoll_wait(epfd, epev, MAX_EVENTS, -1);

		key = get_input();

		if (key == sw->max_skeys + 2) {
			sw->record_led = 0;
			run = 0;
			feature_request();
		}

		nbytes = read(evfd, inev, sizeof(struct input_event));

		if (inev->type == 1 && inev->value != 2) {

			/* TODO: read config, if capture_delays is true */
			if (prev.tv_usec) {
				long int diff = (inev->time.tv_usec + 1000000 * inev->time.tv_sec) - (prev.tv_usec + 1000000 * prev.tv_sec);
				int delay = diff / 1000;

				/* convert delay to string */
				snprintf(tmp, sizeof(tmp) + 1, "%d", delay);

				/* start element "DelayEvent" */
				rc = xmlTextWriterStartElement(writer, BAD_CAST "DelayEvent");

				/* write value */
				rc = xmlTextWriterWriteRaw(writer, BAD_CAST tmp);

				/* close element "DelayEvent" */
				rc = xmlTextWriterEndElement(writer);
			}

			/* start element "KeyBoardEvent" */
			rc = xmlTextWriterStartElement(writer, BAD_CAST "KeyBoardEvent");

			if (inev->value) {
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "Down", BAD_CAST "true");
			} else {
				rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "Down", BAD_CAST "false");
			}

			/* convert keycode to string */
			snprintf(tmp, sizeof(tmp) + 1, "%d", inev->code);

			/* write value */
			rc = xmlTextWriterWriteRaw(writer, BAD_CAST tmp);

			/* close element "KeyBoardEvent" */
			rc = xmlTextWriterEndElement(writer);

			prev = inev->time;
		}
	}

    /* xmlTextWriterEndDocument() closes all remaining open elements */
    rc = xmlTextWriterEndDocument(writer);
    xmlFreeTextWriter(writer);

	printf("Exit Macro Recording\n");

	/* cleanup function for the XML library */
	xmlCleanupParser();

	/* remove event file from epoll watchlist */
	epoll_ctl(epfd, EPOLL_CTL_DEL, evfd, epev);
	close(evfd);
}

void process_input(unsigned char key) {
	if (key && key <= sw->max_skeys) {
		play_macro(key);
	} else if (key == sw->max_skeys + 1) {
		toggle_macropad();
	} else if (key == sw->max_skeys + 2) {
		record_macro();
	} else if (key == sw->max_skeys + 3) {
		switch_profile();
	}
}

void cleanup() {
	/* TODO: check, if save_profile is set and save profile to config */

	/* reset device LEDs */
	feature_request();

	/* destroy configuration */
	config_destroy(cfg);

	/* destroying uinput device */
	ioctl(uifd, UI_DEV_DESTROY);

	/* closing open file descriptors */
	close(uifd);
	close(epfd);
	close(fd);

	/* free up allocated memory */
	free(cfg);
	free(epev);
	free(inev);
	free(uidev);
	free(sw->devnode_hidraw);
	free(sw->devnode_input);
	free(sw);

	/* output some epic status message */
	printf("\nThe almighty Sidewinder daemon has been eliminated\n");
}

int main(int argc, char **argv) {
	int i;
	sw = calloc(8, sizeof(struct sidewinder_data));

	const char * test = NULL;

	signal(SIGINT, handler);
	signal(SIGHUP, handler);
	signal(SIGTERM, handler);
	signal(SIGKILL, handler);

	setup_udev();		/* get device node */
	setup_hidraw();		/* setup hidraw interface */
	setup_uidev();		/* sending input events */
	setup_epoll();		/* watching device events */
	setup_device();		/* initialize device */
	setup_config();		/* parsing config files */

	/* sync the device */
	feature_request();

	/* main loop */
	while (active) {
		/*
		 * epoll_wait() checks our device for any changes and blocks the
		 * loop. This leads to a very efficient polling mechanism.
		 */
		epoll_wait(epfd, epev, MAX_EVENTS, -1);
		/*
		 * epoll_wait() unblocks the loop, because a signal has been
		 * registered. We use read() to check the signal.
		 */
		process_input(get_input());
	}

	cleanup();
	exit(0);
}
