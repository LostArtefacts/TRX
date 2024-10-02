#include "gfx/gl/texture.h"

#include "gfx/gl/utils.h"
#include "memory.h"
#include "utils.h"

#include <assert.h>

GFX_GL_TEXTURE *GFX_GL_Texture_Create(GLenum target)
{
    GFX_GL_TEXTURE *texture = Memory_Alloc(sizeof(GFX_GL_TEXTURE));
    GFX_GL_Texture_Init(texture, target);
    return texture;
}

void GFX_GL_Texture_Free(GFX_GL_TEXTURE *texture)
{
    if (texture != NULL) {
        GFX_GL_Texture_Close(texture);
        Memory_FreePointer(&texture);
    }
}

void GFX_GL_Texture_Init(GFX_GL_TEXTURE *texture, GLenum target)
{
    assert(texture != NULL);
    texture->target = target;
    glGenTextures(1, &texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Close(GFX_GL_TEXTURE *texture)
{
    assert(texture != NULL);
    glDeleteTextures(1, &texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Bind(GFX_GL_TEXTURE *texture)
{
    assert(texture != NULL);
    glBindTexture(texture->target, texture->id);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_Load(
    GFX_GL_TEXTURE *texture, const void *data, int width, int height,
    GLint internal_format, GLint format)
{
    assert(texture != NULL);

    GFX_GL_Texture_Bind(texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0, internal_format, width, height, 0, format,
        GL_UNSIGNED_BYTE, data);
    GFX_GL_CheckError();

    glGenerateMipmap(GL_TEXTURE_2D);
    GFX_GL_CheckError();
}

void GFX_GL_Texture_LoadFromBackBuffer(GFX_GL_TEXTURE *const texture)
{
    assert(texture != NULL);

    GFX_GL_Texture_Bind(texture);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GFX_GL_CheckError();

    const GLint vp_x = viewport[0];
    const GLint vp_y = viewport[1];
    const GLint vp_w = viewport[2];
    const GLint vp_h = viewport[3];

    const int32_t side = MIN(vp_w, vp_h);
    const int32_t x = vp_x + (vp_w - side) / 2;
    const int32_t y = vp_y + (vp_h - side) / 2;
    const int32_t w = side;
    const int32_t h = side;

    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, w, h, 0);
    GFX_GL_CheckError();
}
