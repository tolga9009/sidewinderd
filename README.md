sidewinderd
===========

Sidewinder daemon - Linux support for Microsoft Sidewinder X4 / X6 keyboards


Current status
==============

Update (12. Nov 2014): I've setup CMake for better supporting all the different
Linux distros. I have reached a point, where I think, that the software is ready
for beta testing. Note: this software has only been tested with Arch Linux. If
there are bugs with different distros, please report the issue via GitHub. I
will now create an AUR package for sidewinderd. I need your help for other
distros.

Update (1. Nov 2014): What I currently need is some type of IPC mechanism, so
the system can talk with sidewinderd. With kdbus just around the corner, I don't
want to start with dbus. For now, I've implemented a simple PID file mechanism,
so the daemon exits, if any other instance is already running. This will make
sure, that only a single instance is running at a time to avoid any problems. We
will mainly need the IPC mechanism for the GUI tool I'm planning. Since I've not
even started working on it, I think it's okay to wait for now. Major milestones
are reached; I will start working on cleanups and the TODOs all over the source
code. I'm also interested in making other, similar hid-devices work this daemon.
If you're willing to help me or have any other suggestions, please let me know!

You can create new macros by recording them or writing them manually by looking
up keycodes in linux/input.h. A conversion script for native Microsoft Macros
will be provided soon.

If you find any bugs, please report by opening up an issue here on GitHub.


What's this?
============

sidewinderd is a userland daemon, which takes care of your Microsoft Sidewinder
X4 / X6 keyboard's special keys. It's in an early development stage, but you can
already test basic functionality. This driver has been made, because there is no
complete and user-friendly Sidewinder X4 and X6 driver out there. Wattos' Linux
Sidewinder X6 driver (https://github.com/Wattos/LinuxSidewinderX6) doesn't
support our beloved Sidewinder X4s and EvilAndi's x4daemon
(http://geekparadise.de/x4daemon/) needs to be recompiled everytime you want to
change a button and also doesn't support setting any LEDs, profile switching and
macro playing / recording. Also, it seems to have minor issues with media keys,
due to libusb's nature of detaching the device from hid-generic kernel driver to
gain full access over it.

So, there is need for a complete and stable driver for Linux. We're supporting
both, the Sidewinder X4 and X6 (I have access to both keyboards for testing and
debugging). Our goal is to reach feature-parity with Microsoft's Windows drivers
and open up the doors for other, non-Microsoft gaming keyboards and mice.


Software dependencies
=====================

- libconfig 1.4.9 (might also work with earlier versions)
- systemd (successfully tested with 210 and greater)
- tinyxml2 2.2.0 (included, until widely available)
- cmake 3.0 (for building)


What's already done?
====================

- Macro recording
- Macro playback, using XML files - similar to Microsoft's solution
- Special keys (S1-S6 / S1-S30 keys) - listening to keys via hidraw
- Profile switch
- Setting LEDs (LED 1-3, Record)
- X6 only: toggle Macropad between Macro-mode and Numpad-mode
- Very basic configuration setup
- Clang++ as default compiler


What needs to be done?
======================

- Auto profile and Auto LED support
- Documentation
- Qt GUI tool for macro creation, conversion, editing and general configuration


Hey, I want to help!
====================

Great! I was looking for help ^_^! Please get in contact with me via my public
E-Mail adress, which can be found on my profile page. You can also E-Mail me
about anything else related to this project, like ideas / suggestions.


Known bugs
==========

- When a macro is running and you press Bank Switch key, Record key or Game
Center key, the driver will go crazy (not confirmed in C++ port)
