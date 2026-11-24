#ifndef CONFIG_H
#define CONFIG_H

/* See for more info on possible keys or use a program like evtest */
#include "includes/skeybindd.h"
#include <linux/input-event-codes.h>

#define MOD_KEY KEY_LEFTMETA

/* Macos for using the shell */
#define SH(x) {"sh", "-c", x, NULL}
#define BASH(x) {"bash", "-c", x, NULL}
#define ZSH(x) {"zsh", "-c", x, NULL}

/* Put commands here */
/* pulseaudio */
static const char *volume_up[] = SH("pactl set-sink-volume @DEFAULT_SINK@ +5%");
static const char *volume_down[] = SH("pactl set-sink-volume @DEFAULT_SINK@ -5%");
static const char *volume_mute[] = SH("pactl set-sink-mute @DEFAULT_SINK@ toggle");
static const char *mic_mute[] = SH("pactl set-source-mute @DEFAULT_SOURCE@ toggle");

/* mpc */
static const char *music_toggle[] = {"mpc", "toggle", NULL};
static const char *music_stop[] = {"mpc", "stop", NULL};
static const char *music_next[] = {"mpc", "next", NULL};
static const char *music_previous[] = {"mpc", "prev", NULL};

/* Put keybindings here */
static struct keybinding keybindings[] = {
    {.keycodes = {KEY_VOLUMEUP}, volume_up},     {.keycodes = {KEY_VOLUMEDOWN}, volume_down},
    {.keycodes = {KEY_MUTE}, volume_mute},       {.keycodes = {MOD_KEY, KEY_F4}, mic_mute},

    {.keycodes = {KEY_NEXTSONG}, music_next},    {.keycodes = {KEY_PREVIOUSSONG}, music_previous},
    {.keycodes = {KEY_PLAYPAUSE}, music_toggle}, {.keycodes = {KEY_STOPCD}, music_stop},

};

/* Don't touch this */
#define KEYBINDING_LEN sizeof(keybindings) / sizeof(*keybindings)

#endif