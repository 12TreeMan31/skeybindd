# skeybindd - Simple Keybind Daemon

## Building

#### Dependencies (Most distros already come with these):

- libevdev
- uinput

#### Setup (Assuming you are on Arch)

Makes sure to have `uinput` enabled and that you add yourself to the `input` group, otherwise the program might conplain about not having the necessary permissions.

## Todo

- Make work under Wayland as it conflicts with libinput (I think)
- Restart program on new compile (When adding new key configs)
- Learn how to properly use a build system for compile options 