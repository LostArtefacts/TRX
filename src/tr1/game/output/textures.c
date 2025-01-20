#include "game/output/textures.h"

#include "game/output.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

typedef struct {
    float layer;
    float u;
    float v;
} M_FRAME_DATA;

typedef struct {
    GLuint tbo; // buffer to hold UV data
    GLuint tex; // texture to hold UV buffer
    int32_t count;
    M_FRAME_DATA *frames;
} M_FRAMES;

static struct {
    GLuint tex; // 3D texture to hold atlas pages
    M_FRAMES sprites;
} m_LevelData = {};

void Output_Textures_Init(void)
{
}

void Output_Textures_Shutdown(void)
{
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    if (m_LevelData.sprites.tbo != 0) {
        glDeleteBuffers(1, &m_LevelData.sprites.tbo);
        m_LevelData.sprites.tbo = 0;
    }
    if (m_LevelData.sprites.tex != 0) {
        glDeleteTextures(1, &m_LevelData.sprites.tex);
        m_LevelData.sprites.tex = 0;
    }
    if (m_LevelData.tex != 0) {
        glDeleteTextures(1, &m_LevelData.tex);
        m_LevelData.tex = 0;
    }
}

void Output_Textures_UploadLevel(void)
{
    Output_Textures_Shutdown();

    m_LevelData.sprites.count = Output_GetSpriteTextureCount() * 4;
    m_LevelData.sprites.frames =
        Memory_Alloc(m_LevelData.sprites.count * sizeof(M_FRAME_DATA));
    M_FRAME_DATA *frame = m_LevelData.sprites.frames;
    for (int32_t i = 0; i < Output_GetSpriteTextureCount(); i++) {
        const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
        const float u0 = (sprite->offset & 0xFF) / 256.0f;
        const float v0 = (sprite->offset >> 8) / 256.0f;
        const float u1 = u0 + (sprite->width >> 8) / 256.0f;
        const float v1 = v0 + (sprite->height >> 8) / 256.0f;
        frame->layer = sprite->tex_page;
        frame->u = u0;
        frame->v = v0;
        frame++;
        frame->layer = sprite->tex_page;
        frame->u = u1;
        frame->v = v0;
        frame++;
        frame->layer = sprite->tex_page;
        frame->u = u1;
        frame->v = v1;
        frame++;
        frame->layer = sprite->tex_page;
        frame->u = u0;
        frame->v = v1;
        frame++;
    }
    ASSERT(frame == m_LevelData.sprites.frames + m_LevelData.sprites.count);

    glGenBuffers(1, &m_LevelData.sprites.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_LevelData.sprites.tbo);
    glBufferData(
        GL_TEXTURE_BUFFER, m_LevelData.sprites.count * sizeof(M_FRAME_DATA),
        m_LevelData.sprites.frames, GL_DYNAMIC_DRAW);

    glGenTextures(1, &m_LevelData.sprites.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_LevelData.sprites.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_LevelData.sprites.tbo);

    glGenTextures(1, &m_LevelData.tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_LevelData.tex);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TEXTURE_PAGE_WIDTH,
        TEXTURE_PAGE_HEIGHT, Output_GetTexturePageCount(), 0, GL_RGBA,
        GL_UNSIGNED_BYTE, nullptr);
    for (int32_t i = 0; i < Output_GetTexturePageCount(); i++) {
        const RGBA_8888 *const input_ptr = Output_GetTexturePage32(i);

        glTextureSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0, // level
            0, // x offset
            0, // y offset
            0, // z offset
            TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
            i, // depth
            GL_RGBA8, GL_RGBA8, input_ptr);
    }
}

GLuint Output_Textures_GetSpriteFramesTex(void)
{
    return m_LevelData.sprites.tex;
}
