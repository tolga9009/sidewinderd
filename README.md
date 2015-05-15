sidewinderd
===========

Sidewinder daemon - Linux support for Microsoft Sidewinder X4 / X6 keyboards


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
both, the Sidewinder X4 and X6. Our goal is to reach feature-parity with
Microsoft's Windows drivers and open up the doors for other, non-Microsoft
gaming keyboards and mice.


Dependencies
============

- cmake 2.8.8 (make)
- libconfig 1.4.9
- tinyxml2 2.2.0
- libudev 210


Features
========

- Macro recording
- Macro playback, using XML files - similar to Microsoft's solution
- Special keys (S1-S6 / S1-S30 keys) - listening to keys via hidraw
- Profile switch
- Setting LEDs (LED 1-3, Record)
- X6 only: toggle Macropad between Macro-mode and Numpad-mode
- Very basic configuration setup


Todo
====

- Auto profile and Auto LED support
- Documentation
- Mac OS X support
- Code refactoring


Hey, I want to help!
====================

Great! I was looking for help ^_^! Please get in contact with me via my public
E-Mail adress, which can be found on my profile page. You can also E-Mail me
about anything else related to this project, like ideas / suggestions.

This project is sticking to Geosoft's C++ style guidelines and is using
Doxygen-friendly comments.

If you find any bugs, please report by opening up an issue on GitHub.
