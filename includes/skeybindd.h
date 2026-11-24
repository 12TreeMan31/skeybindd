#ifndef SKEYBINDD_H
/* Definitions for internal program structures */
#define SKEYBINDD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define VERSION "1.3"

#define ERR_LOG(...) fprintf(stderr, __VA_ARGS__)

/* TODO
    - Stop program from crashing when device is unplugged
*/
#define KEY_BUFFER 4

struct keybinding {
    uint16_t keycodes[KEY_BUFFER];
    const char **command;
};

#endif