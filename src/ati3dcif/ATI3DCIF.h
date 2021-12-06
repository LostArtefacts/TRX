#pragma once

#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} C3D_COLOR;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} C3D_PALETTENTRY;

typedef struct {
    float x, y, z;
    float s, t, w;
    float r, g, b, a;
} C3D_VTCF;

typedef enum {
    C3D_EPRIM_LINE = 0,
    C3D_EPRIM_TRI = 1,
} C3D_EPRIM;

typedef void *C3D_PRSDATA;
