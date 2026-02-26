// WebGL compatibility - runtime helpers for Emscripten builds.
//
// This file provides implementations for functions that need more than a
// simple inline shim.

#ifdef EMSCRIPTEN_BUILD

    #include <GLES3/gl3.h>
    #include <stdio.h>

// glMapBuffer shim: use glMapBufferRange with appropriate flags.
void *glMapBuffer_compat(GLenum target, GLenum access)
{
    // We need the buffer size to call glMapBufferRange.
    GLint size = 0;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);
    if (size <= 0) {
        return NULL;
    }

    GLbitfield flags = 0;
    if (access == 0x88B8 /* GL_READ_ONLY */) {
        flags = GL_MAP_READ_BIT;
    } else if (access == 0x88B9 /* GL_WRITE_ONLY */) {
        flags = GL_MAP_WRITE_BIT;
    } else {
        // GL_READ_WRITE
        flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    }

    return glMapBufferRange(target, 0, size, flags);
}

#endif // EMSCRIPTEN_BUILD
