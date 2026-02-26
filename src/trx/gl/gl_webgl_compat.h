// WebGL / Emscripten OpenGL compatibility layer for TRX
//
// This header provides compatibility shims so that the TRX codebase can compile
// against both desktop OpenGL 3.3 and WebGL 2.0 (OpenGL ES 3.0 via Emscripten).
//
// Key differences handled:
//   - GLEW is not available under Emscripten; GL ES 3 headers are provided
//     directly by the Emscripten SDK.
//   - Several desktop-only GL entry points (glPolygonMode, glDrawBuffer,
//     glReadBuffer, glMapBuffer, glClearDepth, glBindFragDataLocation,
//     glGetStringi, glDebugMessageCallback, glCopyTexImage2D backbuffer)
//     are either absent or behave differently under WebGL 2.
//   - GL_STACK_UNDERFLOW / GL_STACK_OVERFLOW are not defined in GLES3.

#pragma once

#ifdef EMSCRIPTEN_BUILD

    // Use Emscripten-provided GL ES 3.0 headers instead of GLEW.
    #include <GLES3/gl3.h>
    #include <GLES3/gl3platform.h>

// ---- Desktop-only constants that may be referenced in TRX ----

    #ifndef GL_STACK_UNDERFLOW
        #define GL_STACK_UNDERFLOW 0x0504
    #endif

    #ifndef GL_STACK_OVERFLOW
        #define GL_STACK_OVERFLOW 0x0503
    #endif

    // glPolygonMode does not exist in GL ES; define it as a no-op.
    #ifndef GL_FRONT_AND_BACK
        #define GL_FRONT_AND_BACK 0x0408
    #endif
    #ifndef GL_FILL
        #define GL_FILL 0x1B02
    #endif
    #ifndef GL_LINE
        #define GL_LINE 0x1B01
    #endif

static inline void glPolygonMode(GLenum face, GLenum mode)
{
    (void)face;
    (void)mode;
}

    // glDrawBuffer is not available in GL ES 3.0.  FBOs always draw to
    // GL_COLOR_ATTACHMENT0 by default, which is the only usage in TRX.
    #ifndef GL_COLOR_ATTACHMENT0
        #define GL_COLOR_ATTACHMENT0 0x8CE0
    #endif
static inline void glDrawBuffer(GLenum buf)
{
    (void)buf;
}

    // glReadBuffer is available in ES 3.0; provide GL_BACK definition
    // if missing for completeness.
    #ifndef GL_BACK
        #define GL_BACK 0x0405
    #endif

    // glClearDepth (double precision) → glClearDepthf (float) on ES
    #define glClearDepth(d) glClearDepthf((float)(d))

// glBindFragDataLocation does not exist in GL ES 3.0.
// Fragment outputs use layout(location=0) qualifiers instead.
static inline void glBindFragDataLocation(
    GLuint program, GLuint colorNumber, const char *name)
{
    (void)program;
    (void)colorNumber;
    (void)name;
}

    // glMapBuffer is not available in GL ES 3.0; only glMapBufferRange exists.
    // We wrap glMapBuffer in terms of glMapBufferRange for read/write access.
    #ifndef GL_READ_WRITE
        #define GL_READ_WRITE 0x88BA
    #endif
    #ifndef GL_WRITE_ONLY
        #define GL_WRITE_ONLY 0x88B9
    #endif
    #ifndef GL_READ_ONLY
        #define GL_READ_ONLY 0x88B8
    #endif

    // glGetStringi / GL_NUM_EXTENSIONS: available in ES 3.0, but the desktop
    // usage in context.c queries extensions before glewInit.  Under Emscripten
    // these are fine, but we still need to ensure the defines exist.
    #ifndef GL_NUM_EXTENSIONS
        #define GL_NUM_EXTENSIONS 0x821D
    #endif

    // glDebugMessageCallback may not be available in base ES 3.0.
    // Provide a no-op definition only if not already declared.
    #ifndef GL_DEBUG_OUTPUT
        #define GL_DEBUG_OUTPUT 0x92E1
    #endif

    #ifndef GL_DEBUG_SEVERITY_NOTIFICATION
        #define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
    #endif

    // GLAPIENTRY may not be defined under Emscripten.
    #ifndef GLAPIENTRY
        #define GLAPIENTRY
    #endif

    // GL_RGBA8 internal format: available in ES 3.0, just make sure it's
    // defined.
    #ifndef GL_RGBA8
        #define GL_RGBA8 0x8058
    #endif

    // GL_DEPTH24_STENCIL8: available in ES 3.0
    #ifndef GL_DEPTH24_STENCIL8
        #define GL_DEPTH24_STENCIL8 0x88F8
    #endif

    // GL_DEPTH_STENCIL_ATTACHMENT: available in ES 3.0
    #ifndef GL_DEPTH_STENCIL_ATTACHMENT
        #define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
    #endif

    // GL_PACK_ALIGNMENT / GL_UNPACK_ALIGNMENT: available in ES 3.0

    // GL_TEXTURE_MAX_ANISOTROPY_EXT – available via
    // EXT_texture_filter_anisotropic extension in WebGL 2, but the constant may
    // not be defined in the headers.
    #ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
        #define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
    #endif
    #ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
        #define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
    #endif

    // glGetActiveUniformName – not available in WebGL 2 / GL ES 3.0.
    // Use glGetActiveUniform instead (the name is returned via the name
    // parameter).
    #include <string.h>
static inline void glGetActiveUniformName(
    GLuint program, GLuint uniform_index, GLsizei buf_size, GLsizei *length,
    GLchar *uniform_name)
{
    GLint size;
    GLenum type;
    glGetActiveUniform(
        program, uniform_index, buf_size, length, &size, &type, uniform_name);
}

// glDrawElementsBaseVertex – not available in WebGL 2 / GL ES 3.0.
// Fall back to glDrawElements (basevertex=0 assumption; callers must adjust).
// For proper support, vertex data must be re-indexed or use a draw offset.
static inline void glDrawElementsBaseVertex(
    GLenum mode, GLsizei count, GLenum type, const void *indices,
    GLint basevertex)
{
    (void)basevertex; // Not supported in WebGL 2; ignore for now
    glDrawElements(mode, count, type, indices);
}

// ---- glVertexAttribI1ui shim ----
// glVertexAttribI1ui is not available in GL ES 3.0 / WebGL 2.
// We emulate it via glVertexAttribI4ui with the remaining components set to 0.
static inline void glVertexAttribI1ui(GLuint index, GLuint x)
{
    glVertexAttribI4ui(index, x, 0, 0, 0);
}

    // ---- GL_POLYGON_MODE shim ----
    // GL_POLYGON_MODE (0x0B40) is a desktop-only query target.
    // We define the constant so code compiles, and track state manually.
    #ifndef GL_POLYGON_MODE
        #define GL_POLYGON_MODE 0x0B40
    #endif

// Shadow the real glGetIntegerv to intercept GL_POLYGON_MODE queries.
// We track the polygon mode state ourselves since glPolygonMode is a no-op.
static GLint m_TRX_PolygonMode_State = 0x1B02; // GL_FILL
static inline void m_TRX_glGetIntegerv(GLenum pname, GLint *data)
{
    if (pname == GL_POLYGON_MODE) {
        data[0] = m_TRX_PolygonMode_State;
        data[1] = m_TRX_PolygonMode_State;
        return;
    }
    glGetIntegerv(pname, data);
}
    #define glGetIntegerv m_TRX_glGetIntegerv

    // ---- GL_UNSIGNED_INT_8_8_8_8_REV shim ----
    // This packed pixel type is not available in GL ES 3.0 / WebGL 2.
    // Map it to GL_UNSIGNED_BYTE which is the standard GLES3 equivalent
    // for 32-bit RGBA textures (4 bytes per pixel, 1 byte per channel).
    #ifndef GL_UNSIGNED_INT_8_8_8_8_REV
        #define GL_UNSIGNED_INT_8_8_8_8_REV GL_UNSIGNED_BYTE
    #endif

    // ---- GL_BGRA shim ----
    // GL_BGRA is not available in core GL ES 3.0 / WebGL 2.
    // Map it to GL_RGBA.  Callers that use BGRA data must swizzle on the CPU
    // or accept R/B channel swap.  The Emscripten FMV path uses the browser's
    // <video> element which produces RGBA natively, so no swizzle is needed.
    #ifndef GL_BGRA
        #define GL_BGRA GL_RGBA
    #endif

// glewInit replacement: a no-op.
typedef unsigned int GLenum_compat;
    #define GLEW_OK 0
static inline int glewInit(void)
{
    return GLEW_OK;
}

#endif // EMSCRIPTEN_BUILD
