#include "game/output/sprites.h"

#include "game/output.h"
#include "game/output/textures.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_output.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <string.h>

#define M_QUAD_VERTICES 6

typedef enum {
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_FRAMES,
    M_UNIFORM_SMOOTHING_ENABLED,
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_VIEWPORT_CENTER,
    M_UNIFORM_VIEWPORT_SIZE,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_PROJECTION_MATRIX_OG,
    M_UNIFORM_PHD_PERSP,
    M_UNIFORM_PHD_RES_Z,
    M_UNIFORM_PHD_RES_Z_BUF,
    M_UNIFORM_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_OFFSET,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

#pragma pack(push, 1)
typedef struct {
    // attribute 0
    XYZ_F pos;

    // attribute 1
    struct {
        float x, y;
    } displacement;

    // attribute 2
    int32_t texture_idx;
} M_SPRITE_VERTEX;

typedef uint16_t M_SPRITE_SHADE;
#pragma pack(pop)

typedef struct {
    int32_t quad_start;
    int32_t quad_count;
} M_ROOM_BATCH;

static GFX_GL_PROGRAM m_Program;
static GLint m_Uniforms[M_UNIFORM_NUMBER_OF];

static struct {
    GLuint vao;
    GLuint geom_vbo;
    GLuint shade_vbo;
    int32_t total_vertex_count;
    M_SPRITE_VERTEX *geom_vbo_data;
    M_SPRITE_SHADE *shade_vbo_data;

    size_t room_batch_count;
    M_ROOM_BATCH *room_batches;
} m_LevelData = {};
static const int32_t m_QuadToFan[] = { 0, 1, 2, 0, 2, 3 };

static void M_MakeQuad(
    M_SPRITE_VERTEX out_quad[4], const int32_t sprite_idx, const XYZ_16 pos)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);

    for (int32_t k = 0; k < 4; k++) {
        out_quad[k].pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
    }

    for (int32_t k = 0; k < 4; k++) {
        out_quad[k].texture_idx = sprite_idx * 4 + k;
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

static M_ROOM_BATCH *M_GetRoomBatch(const ROOM *const room)
{
    // Room data gets swapped when flipping, but the VBOs do not. So a room 2
    // that gets flipped to room 17 ends up getting the data from room 2,
    // whereas the VBO needs to take data from room 17.
    const int16_t room_num =
        Room_GetFlipStatus() && room->flipped_room != NO_ROOM_NEG
        ? room->flipped_room
        : Room_GetNumber(room);
    return &m_LevelData.room_batches[room_num];
}

static void M_PrepareLevelBatches(void)
{
    const size_t room_batch_size =
        m_LevelData.room_batch_count * sizeof(M_ROOM_BATCH);
    char *common_buffer = Memory_Alloc(room_batch_size);
    m_LevelData.room_batches = (M_ROOM_BATCH *)common_buffer;

    int32_t current_quad = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_ROOM_BATCH *const room_batch = &m_LevelData.room_batches[i];
        room_batch->quad_start = current_quad;
        room_batch->quad_count = room->mesh.num_sprites;
        current_quad += room->mesh.num_sprites;
    }

    // sanity check
    int32_t vertex_count = 0;
    for (int i = 0; i < Room_GetCount(); i++) {
        vertex_count +=
            m_LevelData.room_batches[i].quad_count * M_QUAD_VERTICES;
    }
    ASSERT(vertex_count == m_LevelData.total_vertex_count);
}

static void M_UploadLevelGeometry(void)
{
    int32_t current_quad = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
            const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[j];
            const ROOM_VERTEX *const mesh_vertex =
                &room->mesh.vertices[room_sprite->vertex];

            M_SPRITE_VERTEX quad[4];
            M_MakeQuad(quad, room_sprite->texture, mesh_vertex->pos);

            for (int32_t k = 0; k < M_QUAD_VERTICES; k++) {
                m_LevelData.geom_vbo_data[current_quad * M_QUAD_VERTICES + k] =
                    quad[m_QuadToFan[k]];
            }

            current_quad++;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.geom_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        m_LevelData.total_vertex_count * sizeof(M_SPRITE_VERTEX),
        m_LevelData.geom_vbo_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.shade_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        m_LevelData.total_vertex_count * sizeof(M_SPRITE_SHADE),
        m_LevelData.shade_vbo_data, GL_STATIC_DRAW);
}

static void M_UploadRoomShades(const ROOM *const room)
{
    const M_ROOM_BATCH *const room_batch = M_GetRoomBatch(room);
    int32_t current_vertex = room_batch->quad_start * M_QUAD_VERTICES;
    for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
        const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[j];
        for (int32_t k = 0; k < M_QUAD_VERTICES; k++) {
            m_LevelData.shade_vbo_data[current_vertex] =
                room->mesh.vertices[room_sprite->vertex].light_adder;
            current_vertex++;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.shade_vbo);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        room_batch->quad_start * M_QUAD_VERTICES * sizeof(M_SPRITE_SHADE),
        room_batch->quad_count * M_QUAD_VERTICES * sizeof(M_SPRITE_SHADE),
        &m_LevelData.shade_vbo_data[room_batch->quad_start * M_QUAD_VERTICES]);
}

void Output_Sprites_Init(void)
{
    GFX_GL_Program_Init(&m_Program);
    GFX_GL_Program_AttachShader(
        &m_Program, GL_VERTEX_SHADER, "shaders/sprites.glsl",
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_AttachShader(
        &m_Program, GL_FRAGMENT_SHADER, "shaders/sprites.glsl",
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_FragmentData(&m_Program, "outColor");
    GFX_GL_Program_Link(&m_Program);

    const char *const uniform_names[] = {
        [M_UNIFORM_TEX_ATLAS] = "uTexture",
        [M_UNIFORM_TEX_FRAMES] = "uFrame",
        [M_UNIFORM_SMOOTHING_ENABLED] = "uSmoothingEnabled",
        [M_UNIFORM_BRIGHTNESS_MULTIPLIER] = "uBrightnessMultiplier",
        [M_UNIFORM_VIEWPORT_CENTER] = "uViewportCenter",
        [M_UNIFORM_VIEWPORT_SIZE] = "uViewportSize",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_PROJECTION_MATRIX_OG] = "uMatProjectionOG",
        [M_UNIFORM_PHD_PERSP] = "uPhdPersp",
        [M_UNIFORM_PHD_RES_Z] = "uPhdResZ",
        [M_UNIFORM_PHD_RES_Z_BUF] = "uPhdResZBuf",
        [M_UNIFORM_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_OFFSET] = "uWibbleOffset",
    };
    for (int32_t i = 0; i < M_UNIFORM_NUMBER_OF; i++) {
        m_Uniforms[i] =
            GFX_GL_Program_UniformLocation(&m_Program, uniform_names[i]);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&m_Program);
    glUniform1i(m_Uniforms[M_UNIFORM_TEX_ATLAS], 0);
    glUniform1i(m_Uniforms[M_UNIFORM_TEX_FRAMES], 1);
}

static void M_FreeBuffers(void)
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (m_LevelData.vao != 0) {
        glDeleteVertexArrays(1, &m_LevelData.vao);
        m_LevelData.vao = 0;
    }
    if (m_LevelData.geom_vbo != 0) {
        glDeleteBuffers(1, &m_LevelData.geom_vbo);
        m_LevelData.geom_vbo = 0;
    }
    if (m_LevelData.shade_vbo != 0) {
        glDeleteBuffers(1, &m_LevelData.shade_vbo);
        m_LevelData.shade_vbo = 0;
    }
    Memory_FreePointer(&m_LevelData.room_batches);
    Memory_FreePointer(&m_LevelData.geom_vbo_data);
    Memory_FreePointer(&m_LevelData.shade_vbo_data);
}

static void M_PrepareLevelBuffers(void)
{
    m_LevelData.total_vertex_count = 0;
    m_LevelData.room_batch_count = Room_GetCount();
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        m_LevelData.total_vertex_count +=
            room->mesh.num_sprites * M_QUAD_VERTICES;
    }

    m_LevelData.geom_vbo_data = Memory_Realloc(
        m_LevelData.geom_vbo_data,
        m_LevelData.total_vertex_count * sizeof(M_SPRITE_VERTEX));
    m_LevelData.shade_vbo_data = Memory_Realloc(
        m_LevelData.shade_vbo_data,
        m_LevelData.total_vertex_count * sizeof(M_SPRITE_SHADE));
    memset(
        m_LevelData.shade_vbo_data, 0,
        m_LevelData.total_vertex_count * sizeof(M_SPRITE_SHADE));

    glGenVertexArrays(1, &m_LevelData.vao);
    glGenBuffers(1, &m_LevelData.geom_vbo);
    glGenBuffers(1, &m_LevelData.shade_vbo);

    glBindVertexArray(m_LevelData.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.geom_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, displacement));

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(
        2, 1, GL_UNSIGNED_INT, sizeof(M_SPRITE_VERTEX),
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, texture_idx));

    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.shade_vbo);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(M_SPRITE_SHADE), 0);
}

void Output_Sprites_Shutdown(void)
{
    M_FreeBuffers();
    GFX_GL_Program_Close(&m_Program);
}

void Output_Sprites_UploadLevel(void)
{
    M_FreeBuffers();
    M_PrepareLevelBuffers();
    M_PrepareLevelBatches();
    M_UploadLevelGeometry();
}

void Output_Sprites_UploadProjectionMatrix(void)
{
    GFX_GL_Program_Bind(&m_Program);

    GLfloat projection[4][4];
    Output_GetProjectionMatrix(projection);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, m_Uniforms[M_UNIFORM_PROJECTION_MATRIX], 1, GL_TRUE,
        &projection[0][0]);

    const float left = 0.0f;
    const float top = 0.0f;
    const float right = GFX_Context_GetDisplayWidth();
    const float bottom = GFX_Context_GetDisplayHeight();
    GLfloat projection_og[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, +1.0f, 0.0f },
        { -(right + left) / (right - left), -(top + bottom) / (top - bottom),
          0.0f, 1.0f },
    };
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, m_Uniforms[M_UNIFORM_PROJECTION_MATRIX_OG], 1,
        GL_FALSE, &projection_og[0][0]);

    GFX_TRACK_UNIFORM(glUniform1f, m_Uniforms[M_UNIFORM_PHD_PERSP], g_PhdPersp);
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_PHD_RES_Z],
        g_FltResZ / (float)(1 << W2V_SHIFT));
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_PHD_RES_Z_BUF], g_FltResZBuf);
}

void Output_Sprites_UploadModelMatrix(void)
{
    GLfloat model_view[4][4];
    Output_GetModelMatrix(model_view);
    GFX_GL_Program_Bind(&m_Program);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, m_Uniforms[M_UNIFORM_MODEL_MATRIX], 1, GL_TRUE,
        &model_view[0][0]);
}

void Output_Sprites_UploadUniforms(void)
{
    GFX_GL_Program_Bind(&m_Program);
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_SMOOTHING_ENABLED],
        g_Config.rendering.texture_filter);
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);
    GFX_TRACK_UNIFORM(
        glUniform2f, m_Uniforms[M_UNIFORM_VIEWPORT_CENTER],
        Viewport_GetCenterX(), Viewport_GetCenterY());
    GFX_TRACK_UNIFORM(
        glUniform2f, m_Uniforms[M_UNIFORM_VIEWPORT_SIZE],
        GFX_Context_GetDisplayWidth(), GFX_Context_GetDisplayHeight());
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_WIBBLE_OFFSET],
        Output_GetWibbleOffset());
}

void Output_Sprites_RenderRoomSprites(const ROOM *const room)
{
    M_UploadRoomShades(room);

    const M_ROOM_BATCH *const batch = M_GetRoomBatch(room);

    Output_Sprites_UploadUniforms();
    Output_Sprites_UploadProjectionMatrix();
    glBindVertexArray(m_LevelData.vao);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, Output_Textures_GetSpriteUVWsTexture());
    glActiveTexture(GL_TEXTURE0);
    GFX_GL_CheckError();

    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());
    glDrawArrays(
        GL_TRIANGLES, batch->quad_start * M_QUAD_VERTICES,
        batch->quad_count * M_QUAD_VERTICES);
    GFX_GL_CheckError();
}
