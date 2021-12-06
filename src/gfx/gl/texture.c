#include "gfx/gl/texture.h"

#include <assert.h>

void GFX_GL_Texture_Init(GFX_GL_Texture *texture, GLenum target)
{
    assert(texture);
    texture->target = target;
    glGenTextures(1, &texture->id);
}

void GFX_GL_Texture_Close(GFX_GL_Texture *texture)
{
    assert(texture);
    glDeleteTextures(1, &texture->id);
}

void GFX_GL_Texture_Bind(GFX_GL_Texture *texture)
{
    assert(texture);
    glBindTexture(texture->target, texture->id);
}

GLenum GFX_GL_Texture_Target(GFX_GL_Texture *texture)
{
    assert(texture);
    return texture->target;
}
