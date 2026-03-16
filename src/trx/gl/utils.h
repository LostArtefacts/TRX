#pragma once

#include <trx/core/log.h>
#include <trx/gl/track.h>

#include <trx/gl/gl.h>

void TRX_GL_CheckError(void);
const char *TRX_GL_GetErrorString(GLenum err);
