#pragma once

#include "gfx/gl/gl_core_3_3.h"

#include <libtrx/engine/image.h>

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
} GFX_2D_SurfaceDesc;

typedef struct GFX_2D_Surface {
    uint8_t *buffer;
    GFX_2D_SurfaceDesc desc;
    bool is_locked;
    bool is_dirty;
} GFX_2D_Surface;

GFX_2D_Surface *GFX_2D_Surface_Create(const GFX_2D_SurfaceDesc *desc);
GFX_2D_Surface *GFX_2D_Surface_CreateFromImage(const IMAGE *image);
void GFX_2D_Surface_Free(GFX_2D_Surface *surface);

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, const GFX_2D_SurfaceDesc *desc);
void GFX_2D_Surface_Close(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Clear(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Lock(GFX_2D_Surface *surface, GFX_2D_SurfaceDesc *out_desc);
bool GFX_2D_Surface_Unlock(GFX_2D_Surface *surface);
