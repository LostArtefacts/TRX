#pragma once

#include "glrage_gl/gl_core_3_3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GLRage_GLSampler {
    GLuint id;
} GLRage_GLSampler;

void GLRage_GLSampler_Init(GLRage_GLSampler *sampler);
void GLRage_GLSampler_Close(GLRage_GLSampler *sampler);

void GLRage_GLSampler_Bind(GLRage_GLSampler *sampler, GLuint unit);
void GLRage_GLSampler_Parameteri(
    GLRage_GLSampler *sampler, GLenum pname, GLint param);
void GLRage_GLSampler_Parameterf(
    GLRage_GLSampler *sampler, GLenum pname, GLfloat param);

#ifdef __cplusplus
}
#endif
