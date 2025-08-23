#pragma once

#include <stdint.h>

typedef struct {
    float r, g, b;
} RGB_F;

typedef struct {
    float r, g, b, a;
} RGBA_F;

typedef struct {
    uint8_t r, g, b;
} RGB_888;

typedef struct {
    uint8_t r, g, b, a;
} RGBA_8888;
