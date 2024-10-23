#include "gfx/gl/sampler.h"

#include "gfx/gl/utils.h"

#include <assert.h>

void GFX_GL_Sampler_Init(GFX_GL_SAMPLER *sampler)
{
    assert(sampler != NULL);
    glGenSamplers(1, &sampler->id);
    GFX_GL_CheckError();
    sampler->initialized = true;
}

void GFX_GL_Sampler_Close(GFX_GL_SAMPLER *sampler)
{
    assert(sampler != NULL);
    if (sampler->initialized) {
        glDeleteSamplers(1, &sampler->id);
        GFX_GL_CheckError();
    }
    sampler->initialized = false;
}

void GFX_GL_Sampler_Bind(GFX_GL_SAMPLER *sampler, GLuint unit)
{
    assert(sampler != NULL);
    assert(sampler->initialized);
    glBindSampler(unit, sampler->id);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Parameteri(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLint param)
{
    assert(sampler != NULL);
    assert(sampler->initialized);
    glSamplerParameteri(sampler->id, pname, param);
    GFX_GL_CheckError();
}

void GFX_GL_Sampler_Parameterf(
    GFX_GL_SAMPLER *sampler, GLenum pname, GLfloat param)
{
    assert(sampler != NULL);
    assert(sampler->initialized);
    glSamplerParameterf(sampler->id, pname, param);
    GFX_GL_CheckError();
}
