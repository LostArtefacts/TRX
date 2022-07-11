#pragma once

#include "gfx/gl/gl_core_3_3.h"

#include <stdbool.h>
#include <stdint.h>

bool GFX_Screenshot_CaptureToFile(const char *path);

void GFX_Screenshot_CaptureToBuffer(
    uint8_t *out_buffer, GLint *out_width, GLint *out_height, GLint depth,
    GLenum format, GLenum type, bool vflip);
