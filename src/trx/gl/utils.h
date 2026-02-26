#pragma once

#include <trx/core/log.h>
#include <trx/gl/track.h>

#ifdef EMSCRIPTEN_BUILD
    #include <trx/gl/gl_webgl_compat.h>
#else
    #include <GL/glew.h>
#endif

#ifdef EMSCRIPTEN_BUILD
    #define TRX_GL_CheckError() ((void)0)
#else
void TRX_GL_CheckError(void);
#endif
const char *TRX_GL_GetErrorString(GLenum err);
