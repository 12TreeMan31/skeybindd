# skeybindd - Simple Keybind Daemon

skeybindd is a little program I made for my laptop so that I could easily manage music without needing to use some kind of desktop enviorment.

Configuration is similar to the suckless way of doing things and is managed in `config.h`

## Building

#### Dependencies (Most distros already come with these):

- libevdev
- uinput

#### Setup (Assuming you are on Arch)

Makes sure to have `uinput` enabled and that you add yourself to the `input` group, otherwise the program might complain about not having the necessary permissions then if that still doesn't work see [here](https://bbs.archlinux.org/viewtopic.php?pid=821783#p821783).

When starting the program you must specify what device to read inputs from. These devices can be found in `/dev/input/` or by using a tool like [evtest](https://cgit.freedesktop.org/evtest/). (Note: Once started skeybindd will grab a device and make it unreadable by other programs)

You can then start skeybindd like so:
``````
skeybindd /dev/input/<YOUR_DEVICE>
``````

##### systemd:
When using systemd make sure that `SYSTEMD` is uncommened in the main config file then you can run
``````
make systemd
systemctl --user enable skeybindd.serivce
``````
Note though that you must specify the device that you will want to get keybindings for within the service file


## Todo

- Restart program on new compile (When updating configs)
- Learn how to properly use a build system for compile options 
- Sometimes program falls asleep making you need to "rub" the keyboard to wake up