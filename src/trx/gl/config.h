#pragma once

#include <trx/config/enum.h>
#include <trx/gl/enum.h>

#include <stdint.h>

typedef struct {
    TEXTURE_FILTER display_filter;
    int32_t multisampling_factor;
    DITHER_MODE dither_mode;
    bool enable_wireframe;
    int32_t line_width;
} TRX_GL_CONFIG;
