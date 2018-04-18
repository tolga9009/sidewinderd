# Sidewinder daemon

This project provides support for gaming peripherals under Linux. It was
originally designed for the Microsoft SideWinder X4, but we have extended
support for more keyboards. Our goal is to create a framework-like environment
for rapid driver development under Linux.


## Supported devices

  * Microsoft SideWinder X4
  * Microsoft SideWinder X6
  * Logitech G105
  * Logitech G710
  * Logitech G710+


## Install

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


## Usage

Enable and start Sidewinder daemon:

    systemctl enable sidewinderd.service
    systemctl start sidewinderd.service

Configure `/etc/sidewinderd.conf` according to your needs. Please change the
user, as the default user is root.

You can now use your gaming peripheral! Please note, that there is no graphical
user interface. Some LEDs might light up, letting you know, that Sidewinder
daemon has successfully recognized your keyboard.


## Record macros

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

## Advanced macros

You can manually edit a macro by editing its associated XML file, located in
Sidewinderd's working directory. If you have not set one in the config, it will be
located at `/home_dir_of_user/.local/share/sidewinderd`, where `home_dir_of_user` is the
home directory of the user you have set in Sidewinderd's config. By default, this user
is `root`, but it is recommended to change this. The macro files are then located at `profile_#/s#.xml` where `profile_#` is the profile/bank number (e.g. if your keyboard has M1-3 keys, then the M1 set of macros would be located under `profile_1`) and `s#.xml` is the macro number (e.g. if your keyboard has macro keys G1-6 then the macro for G1 would be located at `s1.xml`).

Macro XML files consist of a document root `<Macro>`, the contents of which are a list of 
elements, representing actions for Sidewinderd to execute when the key is pressed.
Sidewinderd currently supports the following actions:
1. `<KeyBoardEvent Down="1|0">key</KeyBoardEvent>` - emits a virtual keyboard event, where the value of attribute `Down` is either 1 for a key press event or 0 for a key release event and `key` is the [key code](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h) of the desired key
2 `<MouseEvent Direction="X|Y">pixels</MouseEvent>` - Moves the mouse in X or Y axis for the number of pixels required, which can be a negative value.
3 `<MouseEvent Click="Right|Left"></MouseEvent>` - Sends a mouse click on the Right or Left mouse button
4. `<DelayEvent>msec</DelayEvent>` - Delays for a time, where `msec` is the number of milliseconds to delay
5. `<RunCommand>command</RunCommand>` - Runs a command in the shell, where `command` is the command to run. Supports `sudo` if Sidewinderd was originally run with `sudo`.

### Examples
This macro will type the letters "asdf" with the same delay between keypresses as when they were typed during recording.
```
<Macro>
    <KeyBoardEvent Down="1">30</KeyBoardEvent>
    <DelayEvent>112</DelayEvent>
    <KeyBoardEvent Down="1">31</KeyBoardEvent>
    <DelayEvent>39</DelayEvent>
    <KeyBoardEvent Down="0">30</KeyBoardEvent>
    <DelayEvent>31</DelayEvent>
    <KeyBoardEvent Down="1">32</KeyBoardEvent>
    <DelayEvent>80</DelayEvent>
    <KeyBoardEvent Down="0">31</KeyBoardEvent>
    <DelayEvent>7</DelayEvent>
    <KeyBoardEvent Down="1">33</KeyBoardEvent>
    <DelayEvent>40</DelayEvent>
    <KeyBoardEvent Down="0">32</KeyBoardEvent>
    <DelayEvent>39</DelayEvent>
    <KeyBoardEvent Down="0">33</KeyBoardEvent>
</Macro>
```
This macro will type the letters "asdf" instantly, with no delay between keypresses.
```
<Macro>
    <KeyBoardEvent Down="1">30</KeyBoardEvent>
    <KeyBoardEvent Down="1">31</KeyBoardEvent>
    <KeyBoardEvent Down="0">30</KeyBoardEvent>
    <KeyBoardEvent Down="1">32</KeyBoardEvent>
    <KeyBoardEvent Down="0">31</KeyBoardEvent>
    <KeyBoardEvent Down="1">33</KeyBoardEvent>
    <KeyBoardEvent Down="0">32</KeyBoardEvent>
    <KeyBoardEvent Down="0">33</KeyBoardEvent>
</Macro>
```
This macro will run the command `xscreensaver-command --lock`, to lock the screen on systems running [xscreensaver](https://www.jwz.org/xscreensaver/).
```
<Macro>
    <RunCommand>xscreensaver-command --lock</RunCommand>
</Macro>
```
This macro will move the mouse 20 pixels to the left, 30 pixels up then do a right click.
```
<MouseEvent Direction="X">20</MouseEvent>
<MouseEvent Direction="Y">-30</MouseEvent>
<MouseEvent Click="Right"></MouseEvent>
```


## Contribution

In order to contribute to this project, you need to read and agree the Developer
Certificate of Origin, which can be found in the CONTRIBUTING file. Therefore,
commits need to be signed-off. You can do this by adding the `-s` flag when
commiting: `git commit -s`. Pseudonyms and anonymous contributions will not be
accepted.


## License

This project is made available under the MIT License. For more information,
please refer to the LICENSE file.
