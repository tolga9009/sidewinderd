sidewinderd
===========

Sidewinder daemon - Linux support for Microsoft Sidewinder X4 / X6 keyboards


Current status
==============

Update (24th Oct 2014): I'm currently reading and learning about socket communication, secure daemon programming and modern, new-style daemons. There are still some design-fails, I need to fix. Also, I'm in the early stages of creating a Qt GUI for macro management. Some directories in the code need to be hand-edited to fit your environment (eventhough they can be fixed upstream in 10 minutes), but more or less, it's working. I will release a beta very soon, once these small cosmetic errors are fixed. If you find any bugs, please open up a issue.

Update (5th Oct 2014): The project has reached major milestones. There is still some work to be done, until this can be used in a productive environment. We're almost there, I just need some more time. In the last weeks, I've worked on a C++ port of the driver, because I think, that an object-oriented approach is giving us a cleaner perspective of what is going on. We're working with real world objects, so it made perfect sense to me to make use of an object-oriented language. Also, I was growing sick of the poor string handling in C (which I need to do, due to libudev, libconfig and libxml); there was way too much room for errors.

Using C++ also gives the advantage to use C++ libraries, like TinyXML2. I've completely switched over from the overblown, standard XML parser to a much smaller, lightweight XML parser. The result: cleaner and easier to understand. Also, I've migrated from libconfig to libconfig++, which also made the code much cleaner.

To use the XML macro player, you need manually edit the XML files and lookup the keycodes in linux/input.h.

If you find any bugs, please report. I think it's about time to collect bugs, so once daemon-mode is ready, we can work on the bugs.


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

- New-style daemon mode (using systemd)
- Single instance limit using pid file (may change in future)
- Socket support for interaction between daemon and GUI application
- Auto profile and Auto LED support
- Documentation
- Qt GUI tool for macro creation, conversion, editing and general configuration


Hey, I want to help!
====================

Great! I was looking for help ^_^! Please get in contact with me via my public E-Mail adress, which can be found on
my profile page. You can also E-Mail me about anything else related to this project, like ideas / suggestions.


Known bugs
==========

- When a macro is running and you press Bank Switch key, Record key or Game Center key, the driver will go crazy (not confirmed in C++ port)
