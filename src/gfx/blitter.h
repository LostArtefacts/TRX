#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct GFX_BlitterRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} GFX_BlitterRect;

typedef struct GFX_BlitterImage {
    int32_t width;
    int32_t height;
    int32_t depth;
    uint8_t *buffer;
} GFX_BlitterImage;

void GFX_Blit(
    const GFX_BlitterImage *src_img, const GFX_BlitterRect *src_rect,
    GFX_BlitterImage *dst_img, const GFX_BlitterRect *dst_rect);
