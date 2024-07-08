#ifndef CONFIG_H
#define CONFIG_H

/* See for more info on possible keys or use a program like evtest */
#include <linux/input-event-codes.h>
#include "includes/skeybindd.h"

#define MOD_KEY KEY_LEFTMETA

/* put commands here */
/* pulseaudio */
static const char *volume_up[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL};
static const char *volume_down[] = {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL};
static const char *volume_mute[] = {"pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL};
static const char *mic_mute[] = {"pactl", "set-source-mute", "@DEFAULT_SOURCE@", "toggle", NULL};

/* mpc */
static const char *music_toggle[] = {"mpc", "toggle", NULL};
static const char *music_stop[] = {"mpc", "stop", NULL};
static const char *music_next[] = {"mpc", "next", NULL};
static const char *music_previous[] = {"mpc", "prev", NULL};

/* put keybindings here */
static struct keybinding keybindings[] = {
	{.binding.keycodes = {KEY_VOLUMEUP}, volume_up},
	{.binding.keycodes = {KEY_VOLUMEDOWN}, volume_down},
	{.binding.keycodes = {KEY_MUTE}, volume_mute},
	{.binding.keycodes = {KEY_LEFTSHIFT, MOD_KEY, KEY_F4}, mic_mute},

	{.binding.keycodes = {KEY_NEXTSONG}, music_next},
	{.binding.keycodes = {KEY_PREVIOUSSONG}, music_previous},
	{.binding.keycodes = {KEY_PLAYPAUSE}, music_toggle},
	{.binding.keycodes = {KEY_STOPCD}, music_stop},

};

#define KEYBINDING_LEN sizeof(keybindings) / sizeof(*keybindings)

#endif