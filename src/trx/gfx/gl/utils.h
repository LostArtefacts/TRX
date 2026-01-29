#pragma once

#include <trx/gfx/gl/track.h>
#include <trx/log.h>

#include <GL/glew.h>

void GFX_GL_CheckError(void);
const char *GFX_GL_GetErrorString(GLenum err);
