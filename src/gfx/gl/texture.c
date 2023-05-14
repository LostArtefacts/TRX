#include "gfx/gl/texture.h"

#include "gfx/gl/utils.h"
#include "memory.h"

#include <assert.h>

GFX_GL_Texture *GFX_GL_Texture_Create(GLenum target)
{
    GFX_GL_Texture *texture = Memory_Alloc(sizeof(GFX_GL_Texture));
    GFX_GL_Texture_Init(texture, target);
    return texture;
}

void GFX_GL_Texture_Free(GFX_GL_Texture *texture)
{
    if (texture) {
        GFX_GL_Texture_Close(texture);
        Memory_FreePointer(&texture);
    }
}

void GFX_GL_Texture_Init(GFX_GL_Texture *texture, GLenum target)
{
    assert(texture);
    texture->target = target;
    glGenTextures(1, &texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Close(GFX_GL_Texture *texture)
{
    assert(texture);
    glDeleteTextures(1, &texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Bind(GFX_GL_Texture *texture)
{
    assert(texture);
    glBindTexture(texture->target, texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Load(
    GFX_GL_Texture *texture, const void *data, int width, int height,
    GLint internal_format, GLint format)
{
    assert(texture);

    GFX_GL_Texture_Bind(texture);

    glTexImage2D(
        GL_TEXTURE_2D, 0, internal_format, width, height, 0, format,
        GL_UNSIGNED_BYTE, data);
    GFX_GL_CheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    GFX_GL_CheckError();
}
