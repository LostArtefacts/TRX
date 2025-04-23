#pragma once

#include <stdint.h>

typedef struct {
    float r;
    float g;
    float b;
} RGB_F;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_888;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA_8888;
