# skeybindd - Simple Keybind Daemon

skeybindd is a little program I made for my laptop so that I could easily manage music without needing to use some kind of desktop enviorment.

Configuration is similar to the suckless way of doing things and is managed in `config.h`

## Building

#### Dependencies (Most distros already come with these):

- libevdev
- uinput

#### Setup (Assuming you are on Arch)

Makes sure to have `uinput` enabled and that you add yourself to the `input` group, otherwise the program might complain about not having the necessary permissions. Then if that still doesn't work see [here](https://bbs.archlinux.org/viewtopic.php?pid=821783#p821783).

When using the systemd service you also must specify what device you want to use.

## Todo

- Restart program on new compile (When updating configs)
- Learn how to properly use a build system for compile options 