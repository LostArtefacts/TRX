#pragma once

#include "../../log.h"

#include <GL/glew.h>

#define GFX_GL_CheckError()                                                    \
    {                                                                          \
        for (GLenum err; (err = glGetError()) != GL_NO_ERROR;) {               \
            LOG_ERROR("glGetError: (%s)", GFX_GL_GetErrorString(err));         \
        }                                                                      \
    }

const char *GFX_GL_GetErrorString(GLenum err);
