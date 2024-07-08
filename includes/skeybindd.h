#ifndef SKEYBINDD_H
/* Definitions for internal program structures */
#define SKEYBINDD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VERSION "1.0"
#define KEY_BUFFER 4

union bindingPair
{
    uint16_t keycodes[KEY_BUFFER];
    uint64_t seed;
};

struct keybinding
{
    union bindingPair binding;
    // In the future I would like add support for function callbacks
    const char **command;
};

#endif