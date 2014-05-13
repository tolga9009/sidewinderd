/*
 * Sidewinder daemon - used for supporting the special keys of Microsoft
 * Sidewinder X4 / X6 gaming keyboards.
 *
 * Copyright (c) 2014 Tolga Cakir <tolga@cevel.net>
 * 
 * Special Thanks to Filip Wieladek and Andreas Bader. Without these two
 * guys, I wouldn't be able to do anything.
 */

/*
 * LICENSE!
 */

#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID_MICROSOFT			0x045e
#define PRODUCT_ID_SIDEWINDER_X6	0x074b
#define PRODUCT_ID_SIDEWINDER_X4	0x0768

/* libusb variables */
static libusb_device_handle *handle;
static libusb_context *context = NULL;

int is_sidewinder(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);

	if (desc.idVendor == VENDOR_ID_MICROSOFT && desc.idProduct == PRODUCT_ID_SIDEWINDER_X6) {
		return 1;
	}

	if (desc.idVendor == VENDOR_ID_MICROSOFT && desc.idProduct == PRODUCT_ID_SIDEWINDER_X4) {
		return 1;
	}

	return 0;
}

void sidewinder_init() {
	libusb_device **list;
	libusb_device *found = NULL;

	libusb_init(&context);
	libusb_set_debug(context, 3);

	ssize_t cnt = libusb_get_device_list(context, &list);
	ssize_t i = 0;
	int err = 0;
	if (cnt < 0) {
		printf("No USB devices found\n");
		error();
	}

	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		if (is_sidewinder(device)) {
			found = device;
			break;
		}
	}

	libusb_free_device_list(list, 1);

	if (found) {
		printf("found usb-dev-block!\n");
		err = libusb_open(found, &handle);
		if (err) {
			printf("Unable to open usb device\n");
			error();
		}

		if (libusb_kernel_driver_active(handle, 1)) {
			libusb_detach_kernel_driver(handle, 1);
			printf("Device busy...detaching...\n");
		} else printf("Device free from kernel\n");

		err = libusb_claim_interface(handle, 1);
		if (err) {
			printf("Failed to claim interface.");
			switch (err) {
			case LIBUSB_ERROR_NOT_FOUND:	printf("not found\n");	break;
			case LIBUSB_ERROR_BUSY:		printf("busy\n");		break;
			case LIBUSB_ERROR_NO_DEVICE:	printf("no device\n");	break;
			default:			printf("other\n");		break;
			}
			error();
		}
	} else {
		printf("Could not find compatible USB device.\n");
	}
}

void sidewinder_exit() {
	if (handle) {
		libusb_release_interface(handle, 1);
		libusb_attach_kernel_driver(handle, 1);
		libusb_close(handle);
	}

	if (context) {
		libusb_exit(context);
	}

	handle = NULL;
	context = NULL;
}

int main(int argc, char **argv) {
	uint8_t data[2];
	data[0] = 0x7;
	data[1] = 0x4;
	sidewinder_init();

	libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
							LIBUSB_REQUEST_SET_CONFIGURATION, 0x307, 0x1, data, sizeof(data), 0);

	sidewinder_exit();
	exit(0);
}
