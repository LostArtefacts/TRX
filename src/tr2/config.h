#pragma once

#include <libtrx/config.h>

#include <stdbool.h>

typedef struct {
    bool loaded;

    struct {
        bool fix_m16_accuracy;
    } gameplay;
} CONFIG;

extern CONFIG g_Config;
