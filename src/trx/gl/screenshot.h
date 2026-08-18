#pragma once

#include <trx/core/result.h>

#include <GL/glew.h>
#include <stdint.h>

// Writes the contents of the viewport to an image file, reporting a path it
// cannot write.
RESULT TRX_GL_Screenshot_CaptureToFile(const char *path);

void TRX_GL_Screenshot_CaptureToBuffer(
    uint8_t *out_buffer, GLint *out_width, GLint *out_height, GLint depth,
    GLenum format, GLenum type, bool vflip);
