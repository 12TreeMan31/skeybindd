#ifndef CONFIG_H
#define CONFIG_H

#include <linux/input-event-codes.h>

struct keybinding
{
    int keycodes[8];
    const char *command;
};

/* put commands here */
static const char *volume_Up = "pactl set-sink-volume @DEFAULT_SINK@ +5%";
static const char *volume_Down = "pactl set-sink-volume @DEFAULT_SINK@ -5%";
static const char *volume_Mute = "pactl set-sink-mute @DEFAULT_SINK@ toggle";

/* put keybindings here */
static const struct keybinding keybindings[] =
    {
        {{KEY_VOLUMEUP}, volume_Up},
        {{KEY_VOLUMEDOWN}, volume_Down},
        {{KEY_MUTE}, volume_Mute},
};

#endif