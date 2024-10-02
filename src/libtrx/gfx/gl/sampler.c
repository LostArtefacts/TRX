#include "gfx/gl/sampler.h"

#include "gfx/gl/utils.h"

void GFX_GL_Sampler_Init(GFX_GL_SAMPLER *sampler)
{
    glGenSamplers(1, &sampler->id);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Close(GFX_GL_SAMPLER *sampler)
{
    glDeleteSamplers(1, &sampler->id);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Bind(GFX_GL_SAMPLER *sampler, GLuint unit)
{
    glBindSampler(unit, sampler->id);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Parameteri(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLint param)
{
    glSamplerParameteri(sampler->id, pname, param);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Parameterf(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLfloat param)
{
    glSamplerParameterf(sampler->id, pname, param);
    GFX_GL_CheckError();
}
