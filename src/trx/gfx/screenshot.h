#pragma once

#include <GL/glew.h>
#include <stdint.h>

bool GFX_Screenshot_CaptureToFile(const char *path);

void GFX_Screenshot_CaptureToBuffer(
    uint8_t *out_buffer, GLint *out_width, GLint *out_height, GLint depth,
    GLenum format, GLenum type, bool vflip);
