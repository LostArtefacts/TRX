#pragma once

#include "./enum.h"

#include <stdint.h>

typedef struct {
    int16_t number;
    int16_t volume;
    int16_t randomness;
    int16_t flags;
    SAMPLE_MODE mode;
} SAMPLE_INFO;
