#ifndef SKEYBINDD_H
/* Definitions for internal program structures */
#define SKEYBINDD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define VERSION "1.2"

#define ERR_LOG(...) fprintf(stderr, __VA_ARGS__)

/* TODO
    - Stop program from crashing when device is unplugged
    - Getting context when a DE is started so that graphical programs can be launched
*/
#define KEY_BUFFER 4

struct keybinding {
    uint16_t keycodes[KEY_BUFFER];
    const char **command;
};

#endif