#pragma once

#include "../common.h"

#include <GL/glew.h>

typedef struct {
    bool initialized;
    GLuint id;
} GFX_GL_PROGRAM;

bool GFX_GL_Program_Init(GFX_GL_PROGRAM *program);
void GFX_GL_Program_Close(GFX_GL_PROGRAM *program);

void GFX_GL_Program_Bind(GFX_GL_PROGRAM *program);
char *GFX_GL_Program_PreprocessShader(
    const char *content, GLenum type, GFX_GL_BACKEND backend);
void GFX_GL_Program_AttachShader(
    GFX_GL_PROGRAM *program, GLenum type, const char *path,
    GFX_GL_BACKEND backend);
void GFX_GL_Program_Link(GFX_GL_PROGRAM *program);
void GFX_GL_Program_FragmentData(GFX_GL_PROGRAM *program, const char *name);
GLint GFX_GL_Program_UniformLocation(GFX_GL_PROGRAM *program, const char *name);

void GFX_GL_Program_Uniform3f(
    GFX_GL_PROGRAM *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2);
void GFX_GL_Program_Uniform4f(
    GFX_GL_PROGRAM *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
    GLfloat v3);
void GFX_GL_Program_Uniform1i(GFX_GL_PROGRAM *program, GLint loc, GLint v0);
void GFX_GL_Program_Uniform1f(GFX_GL_PROGRAM *program, GLint loc, GLfloat v0);
void GFX_GL_Program_UniformMatrix4fv(
    GFX_GL_PROGRAM *program, GLint loc, GLsizei count, GLboolean transpose,
    const GLfloat *value);
