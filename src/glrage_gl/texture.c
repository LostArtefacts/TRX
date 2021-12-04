#include "glrage_gl/texture.h"

#include <assert.h>

void GLRage_GLTexture_Init(GLRage_GLTexture *texture, GLenum target)
{
    assert(texture);
    texture->target = target;
    glGenTextures(1, &texture->id);
}

void GLRage_GLTexture_Close(GLRage_GLTexture *texture)
{
    assert(texture);
    glDeleteTextures(1, &texture->id);
}

void GLRage_GLTexture_Bind(GLRage_GLTexture *texture)
{
    assert(texture);
    glBindTexture(texture->target, texture->id);
}

GLenum GLRage_GLTexture_Target(GLRage_GLTexture *texture)
{
    assert(texture);
    return texture->target;
}
