#pragma once

#include <trx/gl/track.h>
#include <trx/log.h>

#include <GL/glew.h>

void TRX_GL_CheckError(void);
const char *TRX_GL_GetErrorString(GLenum err);
