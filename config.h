#ifndef CONFIG_H
#define CONFIG_H

/* Complie time options */
#define KEY_BUFFER 3
// #define SYSTEMD

#define MOD_KEY KEY_LEFTMETA

/* See for more info on possible keys or use a program like evtest */
#include <linux/input-event-codes.h>
#include <stdio.h> // NULL

struct keybinding
{
	int keycodes[KEY_BUFFER];
	const char **command;
};

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
static struct keybinding keybindings[] =
	{
		{{KEY_VOLUMEUP}, volume_up},
		{{KEY_VOLUMEDOWN}, volume_down},
		{{KEY_MUTE}, volume_mute},
		{{KEY_LEFTSHIFT, MOD_KEY, KEY_F4}, mic_mute},

		{{KEY_NEXTSONG}, music_next},
		{{KEY_PREVIOUSSONG}, music_previous},
		{{KEY_PLAYPAUSE}, music_toggle},
		{{KEY_STOPCD}, music_stop},
};
#endif