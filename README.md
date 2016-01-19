Sidewinder daemon
=================

This project provides support for gaming peripherals under Linux.


Devices
=======

* Microsoft SideWinder X4
* Microsoft SideWinder X6
* Logitech G710+


Install
=======

Please check, if this project has already been packaged for your specific Linux
distribution. Currently maintained packages:

  * Arch Linux: https://aur.archlinux.org/packages/sidewinderd/

If there are no packages available for your Linux distribution, please proceed
with the manual installation from source:

1. Install the following dependencies. Please refer to your specific Linux
distribution, as package names might differ.

  * cmake 2.8.8 (make)
  * libconfig 1.4.9
  * tinyxml2 2.2.0
  * libudev 210

2. Create a build directory in the toplevel directory:

    ```
    mkdir build
    ```

3. Run cmake from within the new build directory:

    ```
    cd build
    cmake ..
    ```

4. Compile and install:

    ```
    make
    make install
    ```


Usage
=====

Start and enable Sidewinderd daemon:

    systemctl start sidewinderd.service
    systemctl enable sidewinderd.service

Configure `/etc/sidewinderd.conf` according to your needs. Please change the
user, as the default user is root.

You can now use your gaming peripheral! Please note, that there is no graphical
user interface. Some LEDs might light up, letting you know, that Sidewinder
daemon has successfully recognized your keyboard.

The macro keys are fully programmable, but there is no default event. You can
add functions to them, by recording macros or key combinations:

1. Choose a profile. The profile LED on the device will show you, which profile
is active.

2. Press record key. The record LED will light up.

3. Now, choose and press a macro key. On some devices, the record LED will begin
to blink. You're now in macro mode. Please note, that existing macros may get
overwritten.

4. Everything you type on the keyboard will get recorded. You can either record
a single key combination or a long series of keypresses. Please note, that
keypresses still send events to your operating system and your programs.

5. When done, press the record key again. This will stop recording and save the
macro.

6. You've now created a macro. Use it by setting the chosen profile and pressing
the chosen macro key.


License
=======

This project is made available under the MIT License.
