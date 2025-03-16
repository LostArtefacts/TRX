#include "game/output/textures.h"

#include "game/output.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

typedef struct {
    float u;
    float v;
    float layer;
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

static void M_FillSpriteFrames(void)
{
    M_FRAME_DATA *frame = m_LevelData.sprites.frames;
    for (int32_t i = 0; i < Output_GetSpriteTextureCount(); i++) {
        const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
        const float u0 = (sprite->offset & 0xFF) / 256.0f;
        const float v0 = (sprite->offset >> 8) / 256.0f;
        const float u1 = u0 + (sprite->width >> 8) / 256.0f;
        const float v1 = v0 + (sprite->height >> 8) / 256.0f;
        // clang-format off
        frame->u = u0; frame->v = v0; frame->layer = sprite->tex_page; frame++;
        frame->u = u1; frame->v = v0; frame->layer = sprite->tex_page; frame++;
        frame->u = u1; frame->v = v1; frame->layer = sprite->tex_page; frame++;
        frame->u = u0; frame->v = v1; frame->layer = sprite->tex_page; frame++;
        // clang-format on
    }
    ASSERT(frame == m_LevelData.sprites.frames + m_LevelData.sprites.count);
}

static void M_UploadSpriteFrames(void)
{
    glBindTexture(GL_TEXTURE_BUFFER, m_LevelData.sprites.tex);
    GFX_TRACK_DATA(
        glBufferData, GL_TEXTURE_BUFFER,
        m_LevelData.sprites.count * sizeof(M_FRAME_DATA),
        m_LevelData.sprites.frames, GL_DYNAMIC_DRAW);
}

static void M_PrepareSpriteFrames(void)
{
    m_LevelData.sprites.count = Output_GetSpriteTextureCount() * 4;
    m_LevelData.sprites.frames =
        Memory_Alloc(m_LevelData.sprites.count * sizeof(M_FRAME_DATA));
    M_FillSpriteFrames();

    glGenBuffers(1, &m_LevelData.sprites.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_LevelData.sprites.tbo);

    GLint limit;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    ASSERT(m_LevelData.sprites.count * sizeof(M_FRAME_DATA) <= (size_t)limit);

    glGenTextures(1, &m_LevelData.sprites.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_LevelData.sprites.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_LevelData.sprites.tbo);
}

static void M_UploadAtlas(void)
{
    glGenTextures(1, &m_LevelData.tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_LevelData.tex);
    glTexStorage3D(
        GL_TEXTURE_2D_ARRAY,
        1, // number of mipmaps
        GL_RGBA8, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
        Output_GetTexturePageCount());

    // TODO: handle bilinear toggle
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    for (int32_t i = 0; i < Output_GetTexturePageCount(); i++) {
        const RGBA_8888 *const input_ptr = Output_GetTexturePage32(i);

        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0, // mipmap level
            0, // x offset
            0, // y offset
            i, // z offset
            TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
            1, // depth
            GL_RGBA, GL_UNSIGNED_BYTE, input_ptr);
    }
}

void Output_Textures_UploadLevel(void)
{
    Output_Textures_Shutdown();
    M_PrepareSpriteFrames();
    M_UploadSpriteFrames();
    M_UploadAtlas();
}

void Output_Textures_Update(void)
{
    if (m_LevelData.sprites.tex == 0) {
        return;
    }
    M_FillSpriteFrames();
    M_UploadSpriteFrames();
}

GLuint Output_Textures_GetSpriteFramesTex(void)
{
    return m_LevelData.sprites.tex;
}

GLuint Output_Textures_GetAtlasTex(void)
{
    return m_LevelData.tex;
}
