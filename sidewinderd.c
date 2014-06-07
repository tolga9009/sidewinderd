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
#include <libconfig.h>
#include <libudev.h>
#include <locale.h>

#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

/* VIDs & PIDs*/
#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* constants */
#define MAX_BUF		8
#define MAX_EVENTS	1
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
int32_t fd, uifd, epfd;

/* TODO: struct for special keys, including is_pressed and macro_path */
/* TODO: macro player and xml parser */

/* global structs */
struct uinput_user_dev *uidev;
struct input_event *inev;
struct epoll_event *epev;
struct sidewinder_data {
	uint16_t device_id;
	uint8_t profile;
	uint8_t auto_led;
	uint8_t record_led;	/* 0: off, 1: breath, 2: blink, 3: solid */
	uint8_t macropad;
	const char *device_node;
} *sw;

struct macro_keys {
	uint8_t is_pressed;
	const char *path_to_xml;
} macro_skey[30];

void handler() {
	active = 0;
}

void setup_udev() {
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
					sw->device_id = PRODUCT_ID_SIDEWINDER_X6;
				} else if (strcmp(udev_device_get_sysattr_value(dev, "idProduct"), pid_sidewinder_x4) == 0) {
					sw->device_node = temp_path;
					sw->device_id = PRODUCT_ID_SIDEWINDER_X4;
				}
			}
		}

		udev_device_unref(dev);
	}

	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
}

void setup_hidraw() {
	fd = open(sw->device_node, O_RDWR | O_NONBLOCK);

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

	/* TODO: dynamically get and setbit needed keys by load_config() */
	/* Currently, we set all keybits, to make things easier. */
	ioctl(uifd, UI_SET_EVBIT, EV_KEY);

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
	struct config_t *cfg;
	struct config_setting_t *root, *group, *setting;
	static const char *cfg_file = "sidewinderd.conf";
	int ret;
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
	config_setting_set_string(setting, "1");
	setting = config_setting_add(root, "ms_compat_mode", CONFIG_TYPE_BOOL);
	config_setting_set_string(setting, "false");
	setting = config_setting_add(root, "save_profile_on_exit", CONFIG_TYPE_BOOL);
	config_setting_set_string(setting, "false");

	/* TODO: use for-loop */

	/* profile 1 special keys */
	group = config_setting_add(root, "profile_1", CONFIG_TYPE_GROUP);
	setting = config_setting_add(group, "S01", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s01.xml");
	setting = config_setting_add(group, "S02", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s02.xml");
	setting = config_setting_add(group, "S03", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s03.xml");
	setting = config_setting_add(group, "S04", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s04.xml");
	setting = config_setting_add(group, "S05", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s05.xml");
	setting = config_setting_add(group, "S06", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p1_s06.xml");

	/* profile 2 special keys */
	group = config_setting_add(root, "profile_2", CONFIG_TYPE_GROUP);
	setting = config_setting_add(group, "S01", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s01.xml");
	setting = config_setting_add(group, "S02", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s02.xml");
	setting = config_setting_add(group, "S03", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s03.xml");
	setting = config_setting_add(group, "S04", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s04.xml");
	setting = config_setting_add(group, "S05", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s05.xml");
	setting = config_setting_add(group, "S06", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p2_s06.xml");

	/* profile 3 special keys */
	group = config_setting_add(root, "profile_3", CONFIG_TYPE_GROUP);
	setting = config_setting_add(group, "S01", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s01.xml");
	setting = config_setting_add(group, "S02", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s02.xml");
	setting = config_setting_add(group, "S03", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s03.xml");
	setting = config_setting_add(group, "S04", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s04.xml");
	setting = config_setting_add(group, "S05", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s05.xml");
	setting = config_setting_add(group, "S06", CONFIG_TYPE_STRING);
	config_setting_set_string(setting, "p3_s06.xml");

	/* read user config */
	if (config_read_file(cfg, cfg_file) == CONFIG_FALSE) {
		/* if no user config is available, write new config */
		if (config_write_file(cfg, cfg_file) == CONFIG_FALSE) {
			/* writing new config is not possible due to some reason */
			/* TODO: error handling */
		}
	}

	config_destroy(cfg);
	free(cfg);
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

void init_device() {
	/* TODO: read profile from config */
	sw->profile = 0;
	sw->auto_led = 0;
	sw->record_led = 0;
	sw->macropad = 0;
	feature_request();
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

void play_macro(int j) {
	printf("Key S%d has been pressed\n", j);
}

/* Currently only used for setting LEDs */
void record_macro() {
	/*
	 * We will support recording macros with and without recording
	 * delays. Therefore, pressing the macro key twice will set the LED
	 * to "breath" instead of "solid" and enable delay-recording.
	 * Blinking LED will be set out of "solid" or "breath" mode, when
	 * macro recording is active. Pressing the Record key again, while
	 * the LED is set to either "breath" or "blinking" will exit
	 * recording.
	 */
	switch (sw->record_led) {
		case 0: sw->record_led = 3;	break;
		case 1: sw->record_led = 0;	break;
		case 2: sw->record_led = 0;	break;
		case 3: sw->record_led = 1;	break;
	}

	feature_request();
}

/*
 * process_input() now checks, which key was pressed. The macro keys are
 * packed in a 5-byte buffer, media keys (including Bank Switch and
 * Record) use 8-bytes.
 */
void process_input(uint8_t nbytes, unsigned char *buf) {
	if (nbytes == 5 && buf[0] == 8) {
		int i;
		/*
		 * cutting off buf[0], which is used to differentiate between
		 * macro and media keys. Our task is now to translate the buffer
		 * codes to something we can work with. Here is a table, where
		 * you can look up the keys and buffer, if you want to improve
		 * this method:
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
			uint32_t key;

			if (buf[i]) {
				int j;
				key = buf[i] << (8 * (i - 1));

				for (j = 0; j < SIDEWINDER_X4_MAX_SKEYS; j++) {
					int skey = 1 << j;

					if (key & skey) {
						if (!macro_skey[j].is_pressed) {
							macro_skey[j].is_pressed = 1;
							play_macro(j);
						}
					} else {
						macro_skey[j].is_pressed = 0;
					}
				}

				if (sw->device_id == PRODUCT_ID_SIDEWINDER_X6) {
					for (j = SIDEWINDER_X4_MAX_SKEYS; j < SIDEWINDER_X6_MAX_SKEYS; j++) {
						int skey = 1 << j;

						if (key & skey) {
							if (!macro_skey[j].is_pressed) {
								macro_skey[j].is_pressed = 1;
								play_macro(j);
							}
						} else {
							macro_skey[j].is_pressed = 0;
						}
					}
				}
			}

			/*
			 * quick and dirty hack to unregister all pressed keys, when
			 * packet 0x08 0x00 0x00 0x00 0x00 is received
			 */
			if (buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 0) {
				int k;

				for (k = 0; k < 30; k++) {
					macro_skey[k].is_pressed = 0;
				}
			}
		}
	} else if (nbytes == 8 && buf[0] == 1) {
		int i;
		uint32_t key;
		/* buf[0] == 1 means media keys, buf[6] shows pressed key */
		key = buf[6];

		switch (key) {
			case MKEY_GAMECENTER: toggle_macropad();	break;
			case MKEY_RECORD: record_macro();			break;
			case MKEY_PROFILE: switch_profile();		break;
		}
	}
}

void cleanup() {
	ioctl(uifd, UI_DEV_DESTROY);
	close(uifd);
	close(epfd);
	close(fd);
	free(epev);
	free(inev);
	free(uidev);
	free(sw);
	printf("\nThe almighty Sidewinder daemon has been eliminated\n");
}

int main(int argc, char **argv) {
	uint8_t nbytes;
	unsigned char buf[MAX_BUF];
	int i;
	sw = calloc(4, sizeof(struct sidewinder_data));

	for (i = 0; i < 30; i++) {
		macro_skey[i].is_pressed = 0;
		macro_skey[i].path_to_xml = 0;
	}

	signal(SIGINT, handler);
	signal(SIGHUP, handler);
	signal(SIGTERM, handler);
	signal(SIGKILL, handler);

	setup_udev();		/* get device node */
	setup_hidraw();		/* setup hidraw interface */
	setup_uidev();		/* sending input events */
	setup_epoll();		/* watching hidraw device events */
	setup_config();		/* parsing config files */

	/* setting initial profile */
	init_device();

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
		nbytes = read(fd, buf, MAX_BUF);
		process_input(nbytes, buf);
	}

	feature_request();
	cleanup();
	exit(0);
}
