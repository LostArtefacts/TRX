#pragma once

#include "gfx/gl/gl_core_3_3.h"
#include "log.h"

#define GFX_GL_CheckError()                                                    \
    {                                                                          \
        for (GLenum err; (err = glGetError()) != GL_NO_ERROR;) {               \
            LOG_ERROR("glGetError: (%s)", GFX_GL_GetErrorString(err));         \
        }                                                                      \
    }

#ifdef __cplusplus
extern "C" {
#endif

const char *GFX_GL_GetErrorString(GLenum err);

#ifdef __cplusplus
}
#endif
