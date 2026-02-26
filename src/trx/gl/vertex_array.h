#pragma once

#ifdef EMSCRIPTEN_BUILD
    #include <trx/gl/gl_webgl_compat.h>
#else
    #include <GL/glew.h>
#endif

typedef struct {
    bool initialized;
    GLuint id;
} TRX_GL_VERTEX_ARRAY;

void TRX_GL_VertexArray_Init(TRX_GL_VERTEX_ARRAY *array);
void TRX_GL_VertexArray_Close(TRX_GL_VERTEX_ARRAY *array);
void TRX_GL_VertexArray_Bind(TRX_GL_VERTEX_ARRAY *array);
void TRX_GL_VertexArray_Attribute(
    TRX_GL_VERTEX_ARRAY *array, GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset);
void TRX_GL_VertexArray_IAttribute(
    TRX_GL_VERTEX_ARRAY *array, GLuint index, GLint size, GLenum type,
    GLsizei stride, GLsizei offset);
