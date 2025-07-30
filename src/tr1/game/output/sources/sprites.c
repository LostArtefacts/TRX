#include "game/output/sources/sprites.h"

#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/textures.h"
#include "game/output/utils.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>
#include <libtrx/vector.h>

#pragma pack(push, 1)
typedef struct {
    XYZ_F pos;
    struct {
        float x, y;
    } displacement;
    OUTPUT_UVW uvw;
    OUTPUT_TEXTURE_SIZE texture_size;
} M_SPRITE_VERTEX;

typedef OUTPUT_USHORT M_SPRITE_SHADE;
#pragma pack(pop)

typedef struct {
    MATRIX matrix;
    XYZ_32 pos;
    int32_t sprite_idx;
    RGB_F tint;
    int16_t shade;
} M_INSTANCE;

typedef struct {
    GLuint vao;
    GLuint geom_vbo;
    GLuint shade_vbo;
    size_t vertex_capacity;
    size_t vertex_count;
    M_SPRITE_VERTEX *geom_vbo_data;
    M_SPRITE_SHADE *shade_vbo_data;
} M_SPRITE_BUFFER;

typedef struct {
    const ROOM *room;
    MATRIX matrix;
    RGB_F tint;
    bool wibble;
    bool water_effect;
} M_ROOM;

typedef struct {
    SCENE_SOURCE source;
    OUTPUT_SHADER *shader;
    VECTOR *scheduled;
    M_SPRITE_BUFFER sprite_buf;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_MakeQuad(
    M_SPRITE_VERTEX out_quad[4], int32_t sprite_idx, XYZ_16 pos);
static void M_BufferReallocGPU(M_SPRITE_BUFFER *buffer);
static void M_PrepareBuffer(void);
static void M_FreeBuffer(void);

static void M_RenderBegin(const SCENE_SOURCE *source);
static void M_RenderPass(const SCENE_SOURCE *source, SCENE_PASS pass);
static bool M_IsDirty(const SCENE_SOURCE *source, SCENE_PASS pass);

static void M_MakeQuad(
    M_SPRITE_VERTEX out_quad[4], const int32_t sprite_idx, const XYZ_16 pos)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);

    for (int32_t k = 0; k < 4; k++) {
        const int32_t uvw_idx =
            Output_Textures_GetSpritesUVWsBase() + sprite_idx * 4 + k;
        out_quad[k].pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
        out_quad[k].uvw = Output_Textures_GetUVW(uvw_idx);
        out_quad[k].texture_size = Output_Textures_GetAtlasSize(uvw_idx / 4);
    }

    out_quad[0].displacement.x = sprite->x0;
    out_quad[0].displacement.y = sprite->y0;
    out_quad[1].displacement.x = sprite->x1;
    out_quad[1].displacement.y = sprite->y0;
    out_quad[2].displacement.x = sprite->x1;
    out_quad[2].displacement.y = sprite->y1;
    out_quad[3].displacement.x = sprite->x0;
    out_quad[3].displacement.y = sprite->y1;
}

static void M_BufferReallocGPU(M_SPRITE_BUFFER *const buffer)
{
    // This triggers a reallocation on the GPU and should be used sparingly,
    // even when swapping the entire buffer data.
    glBindBuffer(GL_ARRAY_BUFFER, buffer->geom_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        buffer->vertex_capacity * sizeof(M_SPRITE_VERTEX),
        buffer->geom_vbo_data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, buffer->shade_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        buffer->vertex_capacity * sizeof(M_SPRITE_SHADE),
        buffer->shade_vbo_data, GL_DYNAMIC_DRAW); // shades are always dynamic
}

static void M_PrepareBuffer(void)
{
    M_PRIV *const p = &m_Priv;
    M_SPRITE_BUFFER *const buffer = &p->sprite_buf;
    buffer->vertex_capacity = 500;
    buffer->vertex_count = 0;

    buffer->geom_vbo_data =
        Memory_Alloc(buffer->vertex_capacity * sizeof(M_SPRITE_VERTEX));
    buffer->shade_vbo_data =
        Memory_Alloc(buffer->vertex_capacity * sizeof(M_SPRITE_SHADE));

    glGenVertexArrays(1, &buffer->vao);
    glGenBuffers(1, &buffer->geom_vbo);
    glGenBuffers(1, &buffer->shade_vbo);

    M_BufferReallocGPU(buffer);

    glBindVertexArray(buffer->vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer->geom_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 3, GL_FLOAT, GL_FALSE, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_NORMAL, 2, GL_FLOAT, GL_FALSE, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, displacement));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_UVW, 3, GL_FLOAT, GL_FALSE, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, uvw));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_TEXTURE_SIZE, 4, GL_FLOAT, GL_FALSE,
        sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, texture_size));

    glBindBuffer(GL_ARRAY_BUFFER, buffer->shade_vbo);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_SHADE, 1, OUTPUT_USHORT_GL, GL_FALSE,
        sizeof(M_SPRITE_SHADE), 0);
}

static void M_FreeBuffer(void)
{
    M_PRIV *const p = &m_Priv;
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (p->sprite_buf.vao != 0) {
        glDeleteVertexArrays(1, &p->sprite_buf.vao);
        p->sprite_buf.vao = 0;
    }
    if (p->sprite_buf.geom_vbo != 0) {
        glDeleteBuffers(1, &p->sprite_buf.geom_vbo);
        p->sprite_buf.geom_vbo = 0;
    }
    if (p->sprite_buf.shade_vbo != 0) {
        glDeleteBuffers(1, &p->sprite_buf.shade_vbo);
        p->sprite_buf.shade_vbo = 0;
    }
    Memory_FreePointer(&p->sprite_buf.geom_vbo_data);
    Memory_FreePointer(&p->sprite_buf.shade_vbo_data);
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->scheduled);
    p->sprite_buf.vertex_count = 0;
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_MESHES) {
        return;
    }
    if (p->scheduled->count == 0) {
        return;
    }

    glBindVertexArray(p->sprite_buf.vao);
    GFX_GL_CheckError();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Output_Textures_GetEnvMapTexture());
    GFX_GL_CheckError();

    M_SPRITE_BUFFER *const buffer = &p->sprite_buf;
    if ((size_t)p->scheduled->count * OUTPUT_QUAD_VERTICES
        > buffer->vertex_capacity) {
        buffer->vertex_capacity =
            (p->scheduled->count + 50) * OUTPUT_QUAD_VERTICES;
        buffer->geom_vbo_data = Memory_Realloc(
            buffer->geom_vbo_data,
            buffer->vertex_capacity * sizeof(M_SPRITE_VERTEX));
        buffer->shade_vbo_data = Memory_Realloc(
            buffer->shade_vbo_data,
            buffer->vertex_capacity * sizeof(M_SPRITE_SHADE));
    }

    for (int32_t i = 0; i < p->scheduled->count; i++) {
        const M_INSTANCE *const sprite = Vector_Get(p->scheduled, i);
        M_SPRITE_VERTEX quad[4];
        M_MakeQuad(
            quad, sprite->sprite_idx,
            (XYZ_16) { sprite->pos.x, sprite->pos.y, sprite->pos.z });
        for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
            buffer->geom_vbo_data[buffer->vertex_count] =
                quad[OUTPUT_QUAD_TO_FAN(i)];
            buffer->shade_vbo_data[buffer->vertex_count] = sprite->shade;
            buffer->vertex_count++;
        }
    }
    M_BufferReallocGPU(buffer);
    glVertexAttribI1ui(OUTPUT_MESH_ATTR_FLAGS, VERT_BILLBOARD);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_TEXTURE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

    Output_Shader_UploadWibbleEffect(p->shader, Output_GetWibbleEffect());
    for (int32_t i = 0; i < p->scheduled->count; i++) {
        const M_INSTANCE *const sprite = Vector_Get(p->scheduled, i);
        Output_Shader_UploadTint(p->shader, sprite->tint);
        Output_Shader_UploadViewModelMatrix(p->shader, &sprite->matrix);
        glDrawArrays(
            GL_TRIANGLES, i * OUTPUT_QUAD_VERTICES, OUTPUT_QUAD_VERTICES);
        GFX_GL_CheckError();
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_MESHES && p->scheduled->count > 0;
}

void OutputSource_Sprites_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->shader = Output_GetMeshShader();
    p->scheduled = Vector_CreateAtCapacity(sizeof(M_INSTANCE), 50);
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    SceneCompositor_AddSource(&p->source);
}

void OutputSource_Sprites_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    Vector_Free(p->scheduled);
    M_FreeBuffer();
}

void OutputSource_Sprites_ObserveLevelLoad(void)
{
    M_FreeBuffer();
    M_PrepareBuffer();
}

void OutputSource_Sprites_ObserveLevelUnload(void)
{
    M_FreeBuffer();
}

void OutputSource_Sprites_Stage(
    const int32_t sprite_idx, const int16_t shade, const RGB_F tint)
{
    M_PRIV *const p = &m_Priv;
    const M_INSTANCE sprite = {
        .matrix = *g_MatrixPtr,
        .tint = tint,
        .pos = (XYZ_32) { 0, 0, 0 },
        .sprite_idx = sprite_idx,
        .shade = shade,
    };
    Vector_Add(p->scheduled, &sprite);
}
