#pragma once

#include "../../engine/image.h"
#include "../gl/gl_core_3_3.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t width;
    int32_t height;
    int32_t pitch;
    void *pixels;
    int32_t bit_count;
    GLenum tex_format;
    GLenum tex_type;
} GFX_2D_SURFACE_DESC;

typedef struct {
    uint8_t *buffer;
    GFX_2D_SURFACE_DESC desc;
    bool is_locked;
    bool is_dirty;
} GFX_2D_SURFACE;

GFX_2D_SURFACE *GFX_2D_Surface_Create(const GFX_2D_SURFACE_DESC *desc);
GFX_2D_SURFACE *GFX_2D_Surface_CreateFromImage(const IMAGE *image);
void GFX_2D_Surface_Free(GFX_2D_SURFACE *surface);

void GFX_2D_Surface_Init(
    GFX_2D_SURFACE *surface, const GFX_2D_SURFACE_DESC *desc);
void GFX_2D_Surface_Close(GFX_2D_SURFACE *surface);

bool GFX_2D_Surface_Clear(GFX_2D_SURFACE *surface);

bool GFX_2D_Surface_Lock(
    GFX_2D_SURFACE *surface, GFX_2D_SURFACE_DESC *out_desc);
bool GFX_2D_Surface_Unlock(GFX_2D_SURFACE *surface);
