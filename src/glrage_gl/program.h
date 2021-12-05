#pragma once

#include "glrage_gl/gl_core_3_3.h"

#ifdef __cplusplus
    #include <cstdbool>
extern "C" {
#else
    #include <stdbool.h>
#endif

typedef struct GLRage_GLProgram {
    GLuint id;
} GLRage_GLProgram;

bool GLRage_GLProgram_Init(GLRage_GLProgram *program);
void GLRage_GLProgram_Close(GLRage_GLProgram *program);

void GLRage_GLProgram_Bind(GLRage_GLProgram *program);
void GLRage_GLProgram_AttachShader(
    GLRage_GLProgram *program, GLenum type, const char *path);
void GLRage_GLProgram_Link(GLRage_GLProgram *program);
void GLRage_GLProgram_FragmentData(GLRage_GLProgram *program, const char *name);
GLint GLRage_GLProgram_UniformLocation(
    GLRage_GLProgram *program, const char *name);

void GLRage_GLProgram_Uniform3f(
    GLRage_GLProgram *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2);
void GLRage_GLProgram_Uniform4f(
    GLRage_GLProgram *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
    GLfloat v3);
void GLRage_GLProgram_Uniform1i(GLRage_GLProgram *program, GLint loc, GLint v0);
void GLRage_GLProgram_UniformMatrix4fv(
    GLRage_GLProgram *program, GLint loc, GLsizei count, GLboolean transpose,
    const GLfloat *value);

#ifdef __cplusplus
}
#endif
