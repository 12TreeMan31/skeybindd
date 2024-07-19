#ifndef SKEYBINDD_H
/* Definitions for internal program structures */
#define SKEYBINDD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VERSION "1.1"
/* TODO
    - Starting up without keeping the last regestered event
    - Stop program from crashing when device is unplugged
    - Getting context when a DE is started so that graphical programs can be launched
*/
#define KEY_BUFFER 4

union bindingPair
{
    uint16_t keycodes[KEY_BUFFER];
    uint64_t seed;
};

struct keybinding
{
    union bindingPair binding;
    const char **command;
};

#endif