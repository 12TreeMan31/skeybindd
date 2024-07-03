#ifndef CONFIG_H
#define CONFIG_H

/* Complie time options */
#define KEY_BUFFER 3

#define MOD_KEY KEY_LEFTMETA

/* See for more info on possible keys or use a program like evtest */
#include <linux/input-event-codes.h>
#include <stdio.h> // NULL
#include <stdint.h>

// In the future I would like add support for function callbacks

struct keybinding
{
	uint16_t keycodes[KEY_BUFFER];
	uint64_t binding;
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
static struct keybinding keybindings[] = {
	{{KEY_VOLUMEUP}, 0, volume_up},
	{{KEY_VOLUMEDOWN}, 0, volume_down},
	{{KEY_MUTE}, 0, volume_mute},
	{{KEY_LEFTSHIFT, MOD_KEY, KEY_F4}, 0, mic_mute},

	{{KEY_NEXTSONG}, 0, music_next},
	{{KEY_PREVIOUSSONG}, 0, music_previous},
	{{KEY_PLAYPAUSE}, 0, music_toggle},
	{{KEY_STOPCD}, 0, music_stop},
};

#define KEYBINDING_LEN sizeof(keybindings) / sizeof(*keybindings)

#endif