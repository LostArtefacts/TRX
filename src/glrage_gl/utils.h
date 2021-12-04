#pragma once

#include "glrage_gl/gl_core_3_3.h"
#include "log.h"

#define GLRage_GLCheckError()                                                  \
    {                                                                          \
        for (GLenum err; (err = glGetError()) != GL_NO_ERROR;) {               \
            LOG_ERROR("glGetError: (%s)", GLRage_GLGetErrorString(err));       \
        }                                                                      \
    }

#ifdef __cplusplus
extern "C" {
#endif

const char *GLRage_GLGetErrorString(GLenum err);

#ifdef __cplusplus
}
#endif
