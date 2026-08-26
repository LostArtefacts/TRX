#pragma once

#include <trx/core/log.h>
#include <trx/gl/track.h>

#include <GL/glew.h>

void TRX_GL_CheckErrorAt(const char *file, int line, const char *func);
#define TRX_GL_CheckError() TRX_GL_CheckErrorAt(__FILE__, __LINE__, __func__)
const char *TRX_GL_GetErrorString(GLenum err);
