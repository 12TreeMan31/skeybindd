# skeybindd - Simple Keybind Daemon

skeybindd is a little program I made for my laptop so that I could easily manage music without needing to use some kind of desktop enviorment.

## Building

#### Dependencies (Most distros already come with these):

- libevdev
- uinput

``````
git clone https://github.com/12TreeMan31/skeybindd.git
make install
``````

#### Setup (Arch)

Makes sure to have `uinput` enabled and that you add yourself to the `input` group, otherwise the program might complain about not having the necessary permissions. If that still doesn't work see [here](https://bbs.archlinux.org/viewtopic.php?pid=821783#p821783) for a potental solution.

When starting the program you must specify what device to read inputs from. These devices can be found in `/dev/input/` or by using a tool like [evtest](https://cgit.freedesktop.org/evtest/). (Note: Once started skeybindd will grab the device and make it unreadable by other programs)

You can then start skeybindd like so:
``````
skeybindd -f /dev/input/<YOUR_DEVICE>
``````

## Configuration
Configuration is similar to suckless, so the default keybinding can be found in `config.def.h`. Users should not edit this file directly and should instead copy the file to `config.h` (Will be automatically generated if it doesn't exist when building).


#### systemd:
First make sure that you specify your keyboard in `skeybindd.service`.

Then run:

``````
make systemd
systemctl --user enable skeybindd.serivce
``````

## Todo

- Restart program on new compile (When updating configs)
- Sometimes program falls asleep making you need to "rub" the keyboard to wake up