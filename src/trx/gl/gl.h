#pragma once

// Centralized OpenGL include header.
//
// By default we build against desktop OpenGL (using GLEW for function loading).
// For GLES builds (e.g. iOS or ANGLE), define TRX_USE_GLES at compile time.

#if defined(TRX_USE_GLES)
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif
