#pragma once

#include "glrage_gl/gl_core_3_3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GLRage_GLBuffer {
    GLuint id;
    GLenum target;
} GLRage_GLBuffer;

void GLRage_GLBuffer_Init(GLRage_GLBuffer *buf, GLenum target);
void GLRage_GLBuffer_Close(GLRage_GLBuffer *buf);

void GLRage_GLBuffer_Bind(GLRage_GLBuffer *buf);
void GLRage_GLBuffer_Data(
    GLRage_GLBuffer *buf, GLsizei size, const void *data, GLenum usage);
void GLRage_GLBuffer_SubData(
    GLRage_GLBuffer *buf, GLsizei offset, GLsizei size, const void *data);
void *GLRage_GLBuffer_Map(GLRage_GLBuffer *buf, GLenum access);
void GLRage_GLBuffer_Unmap(GLRage_GLBuffer *buf);
GLint GLRage_GLBuffer_Parameter(GLRage_GLBuffer *buf, GLenum pname);

#ifdef __cplusplus
}
#endif
