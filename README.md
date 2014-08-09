sidewinderd
===========

Sidewinder daemon - Linux support for Microsoft Sidewinder X4 / X6 keyboards


Current status
==============

We're trying to get the majority of functions implemented, this project is under development and not usable for
everyday work. But you can use it to get a basic idea of the program and test functionality. Please don't file bug
reports in the current state. Fixing bugs isn't a priority as of right now. To use the XML macro player, you need
manually edit the XML files and lookup the keycodes in linux/input.h.


What's this?
============

sidewinderd is a userland daemon, which takes care of your Microsoft Sidewinder X4 / X6 keyboard's special keys.
It's in an early development stage, but you can already test basic functionality. This driver has been made, because
there is no complete and user-friendly Sidewinder X4 and X6 driver out there. Wattos' Linux Sidewinder X6 driver
(https://github.com/Wattos/LinuxSidewinderX6) doesn't support our beloved Sidewinder X4s and EvilAndi's x4daemon
(http://geekparadise.de/x4daemon/) needs to be recompiled everytime you want to change a button and also doesn't support
setting any LEDs, profile switching and macro playing / recording. Also, it seems to have minor issues with media keys,
due to libusb's nature of detaching the device from hid-generic kernel driver to gain full access over it.

So, there is need for a complete and stable driver for Linux. We're supporting both, the Sidewinder X4 and X6 (I have
access to both keyboards for testing and debugging). Our goal is to reach feature-parity with Microsoft's Windows drivers
and open up the doors for other, non-Microsoft gaming keyboards and mice.


What's already done?
====================

- S1-S6 macro key recognition via hidraw (S1-S30 for X6)
- Setting profile and profile LEDs via Bank Switch button
- Setting record LEDs via Record button (no Macro Recording support as of right now)
- X6 only: toggle Macropad between Macro-mode and Numpad-mode
- Playing Macros using XML files, just like Microsoft's driver
- Very basic configuration setup
- Countless bugs have been implemented


What needs to be done?
======================

- Daemon mode
- Optimizations and cleanups
- Macro Recording
- Syslogging
- Optimizations and cleanups
- Lots of error handling
- Support for Auto profile and Auto LED
- Did I mention optimizations and cleanups?
- Documentation
- Some other things, which simply pop up, while working on the drivers


Hey, I want to help!
====================

Great! I was looking for help ^_^! Please get in contact with me via my public E-Mail adress, which can be found on
my profile page. You can also E-Mail me about anything else related to this project, like ideas / suggestions.


Known bugs
==========

- When a macro is running and you press Bank Switch key, Record key or Game Center key, the driver will go crazy.
