#include "game/output/sprites.h"

#include "game/output.h"
#include "game/output/shader.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/room.h"

#include <libtrx/gfx/gl/utils.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/vector.h>

#pragma pack(push, 1)
typedef struct {
    // attribute 0
    XYZ_F pos;

    // attribute 1
    struct {
        float x, y;
    } displacement;

    // attribute 2
    int32_t uvw_idx;
} M_SPRITE_VERTEX;

typedef uint16_t M_SPRITE_SHADE;
#pragma pack(pop)

typedef struct {
    MATRIX matrix;
    XYZ_32 pos;
    int32_t sprite_idx;
    uint16_t shade;
} M_DYNAMIC_SPRITE;

typedef struct {
    GLuint vao;
    GLuint geom_vbo;
    GLuint shade_vbo;
    size_t vertex_capacity;
    size_t vertex_count;
    GLenum usage;
    M_SPRITE_VERTEX *geom_vbo_data;
    M_SPRITE_SHADE *shade_vbo_data;
} M_SPRITE_BUFFER;

typedef struct {
    int32_t quad_start;
    int32_t quad_count;
} M_ROOM_BATCH;

static OUTPUT_SHADER *m_Shader = nullptr;

static struct {
    M_SPRITE_BUFFER sprite_buf;
    size_t room_batch_count;
    M_ROOM_BATCH *room_batches;
} m_LevelData = {};

static struct {
    VECTOR *source;
    M_SPRITE_BUFFER sprite_buf;
} m_Dynamic = {};

static void M_MakeQuad(
    M_SPRITE_VERTEX out_quad[4], const int32_t sprite_idx, const XYZ_16 pos)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);

    for (int32_t k = 0; k < 4; k++) {
        out_quad[k].pos = (XYZ_F) { .x = pos.x, .y = pos.y, .z = pos.z };
        out_quad[k].uvw_idx =
            Output_Textures_GetSpritesUVWsBase() + sprite_idx * 4 + k;
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

static void M_BufferReallocGPU(M_SPRITE_BUFFER *const buffer)
{
    // This triggers a reallocation on the GPU and should be used sparingly,
    // even when swapping the entire buffer data.
    glBindBuffer(GL_ARRAY_BUFFER, buffer->geom_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        buffer->vertex_capacity * sizeof(M_SPRITE_VERTEX),
        buffer->geom_vbo_data, buffer->usage);

    glBindBuffer(GL_ARRAY_BUFFER, buffer->shade_vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER,
        buffer->vertex_capacity * sizeof(M_SPRITE_SHADE),
        buffer->shade_vbo_data, GL_DYNAMIC_DRAW); // shades are always dynamic
}

static void M_PrepareBuffer(
    M_SPRITE_BUFFER *const buffer, const size_t capacity, const GLenum usage)
{
    buffer->vertex_capacity = capacity;
    buffer->vertex_count = 0;
    buffer->usage = usage;

    buffer->geom_vbo_data = Memory_Alloc(capacity * sizeof(M_SPRITE_VERTEX));
    buffer->shade_vbo_data = Memory_Alloc(capacity * sizeof(M_SPRITE_SHADE));

    glGenVertexArrays(1, &buffer->vao);
    glGenBuffers(1, &buffer->geom_vbo);
    glGenBuffers(1, &buffer->shade_vbo);

    M_BufferReallocGPU(buffer);

    glBindVertexArray(buffer->vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer->geom_vbo);
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
        (void *)(intptr_t)offsetof(M_SPRITE_VERTEX, uvw_idx));

    glBindBuffer(GL_ARRAY_BUFFER, buffer->shade_vbo);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(M_SPRITE_SHADE), 0);
}

static void M_FreeBuffer(M_SPRITE_BUFFER *const buffer)
{
    if (buffer->vao != 0) {
        glDeleteVertexArrays(1, &buffer->vao);
        buffer->vao = 0;
    }
    if (buffer->geom_vbo != 0) {
        glDeleteBuffers(1, &buffer->geom_vbo);
        buffer->geom_vbo = 0;
    }
    if (buffer->shade_vbo != 0) {
        glDeleteBuffers(1, &buffer->shade_vbo);
        buffer->shade_vbo = 0;
    }
    Memory_FreePointer(&buffer->geom_vbo_data);
    Memory_FreePointer(&buffer->shade_vbo_data);
}

static void M_PushToBuffer(
    M_SPRITE_BUFFER *const buffer, const XYZ_32 pos, const int32_t sprite_idx,
    const uint16_t shade)
{
}

static void M_FlushBuffer(M_SPRITE_BUFFER *const buffer)
{
    buffer->vertex_count = 0;
}

static void M_DrawBuffer(
    M_SPRITE_BUFFER *const buffer, const int32_t start, const int32_t count)
{
    glBindVertexArray(buffer->vao);
    GFX_GL_CheckError();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, Output_Textures_GetUVWsTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Output_Textures_GetEnvMapTexture());
    GFX_GL_CheckError();

    glDrawArrays(GL_TRIANGLES, start, count);
    GFX_GL_CheckError();
}

static void M_PrepareLevelBatches(void)
{
    m_LevelData.room_batches =
        Memory_Alloc(m_LevelData.room_batch_count * sizeof(M_ROOM_BATCH));

    int32_t current_quad = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_ROOM_BATCH *const room_batch = &m_LevelData.room_batches[i];
        room_batch->quad_start = current_quad;
        room_batch->quad_count = room->mesh.num_sprites;
        current_quad += room->mesh.num_sprites;
    }
}

static void M_UpdateRoomGeometry(const ROOM *const room)
{
    int32_t current_vertex = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
            const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[j];
            const ROOM_VERTEX *const mesh_vertex =
                &room->mesh.vertices[room_sprite->vertex];
            const int16_t sprite_idx = room_sprite->texture;
            const XYZ_16 pos = mesh_vertex->pos;

            M_SPRITE_VERTEX quad[4];
            M_MakeQuad(quad, sprite_idx, pos);
            for (int32_t k = 0; k < OUTPUT_QUAD_VERTICES; k++) {
                m_LevelData.sprite_buf.geom_vbo_data[current_vertex] =
                    quad[OUTPUT_QUAD_TO_FAN(k)];
                current_vertex++;
            }
        }
    }
}

static void M_UpdateRoomShades(const ROOM *const room)
{
    const M_ROOM_BATCH *const room_batch = M_GetRoomBatch(room);
    int32_t current_vertex = room_batch->quad_start * OUTPUT_QUAD_VERTICES;
    for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
        const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[j];
        for (int32_t k = 0; k < OUTPUT_QUAD_VERTICES; k++) {
            m_LevelData.sprite_buf.shade_vbo_data[current_vertex] =
                room->mesh.vertices[room_sprite->vertex].light_adder;
            current_vertex++;
        }
    }
}

static void M_FreeBuffers(void)
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    M_FreeBuffer(&m_LevelData.sprite_buf);
    M_FreeBuffer(&m_Dynamic.sprite_buf);
    Memory_FreePointer(&m_LevelData.room_batches);
}

static void M_PrepareLevelBuffers(void)
{
    m_LevelData.room_batch_count = Room_GetCount();

    int32_t num_quads = 0;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        num_quads += Room_Get(i)->mesh.num_sprites;
    }
    M_PrepareBuffer(&m_Dynamic.sprite_buf, 500, GL_DYNAMIC_DRAW);
    M_PrepareBuffer(
        &m_LevelData.sprite_buf, num_quads * OUTPUT_QUAD_VERTICES,
        GL_STATIC_DRAW);

    m_LevelData.sprite_buf.vertex_count = num_quads * OUTPUT_QUAD_VERTICES;
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        M_UpdateRoomGeometry(room);
    }
    M_BufferReallocGPU(&m_LevelData.sprite_buf);
}

void Output_Sprites_Init(void)
{
    m_Shader = Output_Shader_Create("shaders/sprites.glsl");
    m_Dynamic.source = Vector_CreateAtCapacity(sizeof(M_DYNAMIC_SPRITE), 50);
}

void Output_Sprites_Shutdown(void)
{
    M_FreeBuffers();
    Output_Shader_Free(m_Shader);
    m_Shader = nullptr;
}

void Output_Sprites_ObserveLevelLoad(void)
{
    M_FreeBuffers();
    M_PrepareLevelBuffers();
    M_PrepareLevelBatches();
}

void Output_Sprites_RenderRoomSprites(
    const MATRIX *const matrix, const RGB_F tint, const ROOM *const room)
{
    M_UpdateRoomShades(room);

    const M_ROOM_BATCH *const room_batch = M_GetRoomBatch(room);
    glBindBuffer(GL_ARRAY_BUFFER, m_LevelData.sprite_buf.shade_vbo);
    GFX_GL_CheckError();
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_ARRAY_BUFFER,
        room_batch->quad_start * OUTPUT_QUAD_VERTICES * sizeof(M_SPRITE_SHADE),
        room_batch->quad_count * OUTPUT_QUAD_VERTICES * sizeof(M_SPRITE_SHADE),
        &m_LevelData.sprite_buf
             .shade_vbo_data[room_batch->quad_start * OUTPUT_QUAD_VERTICES]);
    GFX_GL_CheckError();

    Output_Shader_UploadWibbleEffect(m_Shader, Output_GetWibbleEffect());
    Output_Shader_UploadMatrix(m_Shader, matrix);
    Output_Shader_UploadTint(m_Shader, tint);
    GFX_GL_CheckError();

    const M_ROOM_BATCH *const batch = M_GetRoomBatch(room);
    M_DrawBuffer(
        &m_LevelData.sprite_buf, batch->quad_start * OUTPUT_QUAD_VERTICES,
        batch->quad_count * OUTPUT_QUAD_VERTICES);
}

void Output_Sprites_RenderSingleSprite(
    const MATRIX *const matrix, const XYZ_32 pos, const int32_t sprite_idx,
    const uint16_t shade)
{
    const M_DYNAMIC_SPRITE sprite = {
        .matrix = *matrix,
        .pos = pos,
        .sprite_idx = sprite_idx,
        .shade = shade,
    };
    Vector_Add(m_Dynamic.source, &sprite);
}

void Output_Sprites_RenderBegin(void)
{
    Output_Shader_UploadCommonUniforms(m_Shader);
    Output_Shader_UploadProjectionMatrix(m_Shader);
}

bool Output_Sprites_Flush(void)
{
    if (m_Dynamic.source->count == 0) {
        return false;
    }

    M_SPRITE_BUFFER *const buffer = &m_Dynamic.sprite_buf;
    if ((size_t)m_Dynamic.source->count * OUTPUT_QUAD_VERTICES
        > buffer->vertex_capacity) {
        buffer->vertex_capacity =
            (m_Dynamic.source->count + 50) * OUTPUT_QUAD_VERTICES;
        buffer->geom_vbo_data = Memory_Realloc(
            buffer->geom_vbo_data,
            buffer->vertex_capacity * sizeof(M_SPRITE_VERTEX));
        buffer->shade_vbo_data = Memory_Realloc(
            buffer->shade_vbo_data,
            buffer->vertex_capacity * sizeof(M_SPRITE_SHADE));
    }

    for (int32_t i = 0; i < m_Dynamic.source->count; i++) {
        const M_DYNAMIC_SPRITE *const sprite = Vector_Get(m_Dynamic.source, i);
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

    Output_Shader_UploadWibbleEffect(m_Shader, Output_GetWibbleEffect());
    Output_Shader_UploadTint(m_Shader, (RGB_F) { 1.0f, 1.0f, 1.0f });
    for (int32_t i = 0; i < m_Dynamic.source->count; i++) {
        const M_DYNAMIC_SPRITE *const sprite = Vector_Get(m_Dynamic.source, i);
        Output_Shader_UploadMatrix(m_Shader, &sprite->matrix);
        M_DrawBuffer(
            &m_Dynamic.sprite_buf, i * OUTPUT_QUAD_VERTICES,
            OUTPUT_QUAD_VERTICES);
    }

    Vector_Clear(m_Dynamic.source);
    M_FlushBuffer(&m_Dynamic.sprite_buf);
    return true;
}

void Output_Sprites_UploadProjectionMatrix(void)
{
    Output_Shader_UploadProjectionMatrix(m_Shader);
}
