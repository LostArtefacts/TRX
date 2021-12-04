#include "glrage_gl/sampler.h"

#include <assert.h>

void GLRage_GLSampler_Init(GLRage_GLSampler *sampler)
{
    glGenSamplers(1, &sampler->id);
}

void GLRage_GLSampler_Close(GLRage_GLSampler *sampler)
{
    glDeleteSamplers(1, &sampler->id);
}

void GLRage_GLSampler_Bind(GLRage_GLSampler *sampler, GLuint unit)
{
    glBindSampler(unit, sampler->id);
}

void GLRage_GLSampler_Parameteri(
    GLRage_GLSampler *sampler, GLenum pname, GLint param)
{
    glSamplerParameteri(sampler->id, pname, param);
}

void GLRage_GLSampler_Parameterf(
    GLRage_GLSampler *sampler, GLenum pname, GLfloat param)
{
    glSamplerParameterf(sampler->id, pname, param);
}
