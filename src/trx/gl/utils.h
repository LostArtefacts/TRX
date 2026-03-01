#pragma once

#include <trx/core/log.h>
#include <trx/gl/gl_webgl_compat.h>
#include <trx/gl/track.h>

#ifdef EMSCRIPTEN_BUILD
    #define TRX_GL_CheckError() ((void)0)
#else
void TRX_GL_CheckError(void);
#endif
const char *TRX_GL_GetErrorString(GLenum err);
