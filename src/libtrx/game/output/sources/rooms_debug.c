#include "config.h"
#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/sources/rooms.h"
#include "game/output/utils.h"
#include "game/output/vertex_range.h"
#include "game/random.h"
#include "gfx/gl/utils.h"
#include "memory.h"
#include "utils.h"
#include "vector.h"

typedef struct {
    XYZW_F pos;
    RGBA_8888 color;
} M_VERTEX;

typedef struct {
    const ROOM *room;
    MATRIX matrix;
} M_INSTANCE;

typedef struct {
    OUTPUT_VERTEX_RANGE triggers;
    OUTPUT_VERTEX_RANGE portals;
} M_ROOM_MESH;

typedef struct {
    SCENE_SOURCE source;
    OUTPUT_SHADER *shader;
    VECTOR *vertices;
    size_t mesh_count;
    M_ROOM_MESH *meshes;
    VECTOR *scheduled;
    GLuint vao;
    GLuint vbo;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_PrepareRoomTriggers(
    const M_PRIV *const p, M_ROOM_MESH *const mesh, const ROOM *const room)
{
    mesh->triggers.vertex_start = p->vertices->count;
    const RGBA_8888 color = { .r = 255, .g = 0, .b = 255, .a = 128 };
    const XZ_16 offsets[4] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
                const int32_t j = OUTPUT_QUAD_TO_FAN(i);
                XYZ_16 vertex_pos = {
                    .x = (x + offsets[j].x) * WALL_L,
                    .z = (z + offsets[j].z) * WALL_L,
                };
                XYZ_32 world_pos = {
                    .x = room->pos.x + x * WALL_L + offsets[j].x * (WALL_L - 1),
                    .z = room->pos.z + z * WALL_L + offsets[j].z * (WALL_L - 1),
                    .y = room->pos.y,
                };

                int16_t room_num = room - Room_Get(0);
                sector = Room_GetSector(
                    world_pos.x, world_pos.y, world_pos.z, &room_num);
                vertex_pos.y =
                    Room_GetHeight(
                        sector, world_pos.x, world_pos.y, world_pos.z)
                    + (Output_GetWaterEffect() ? -16 : -2);

                M_VERTEX vertex = {
                    .pos = {
                        .x = vertex_pos.x,
                        .y = vertex_pos.y,
                        .z = vertex_pos.z,
                        .w = 0.0f,
                    },
                    .color = color,
                };
                Vector_Add(p->vertices, &vertex);
            }
        }
    }
    mesh->triggers.vertex_count =
        p->vertices->count - mesh->triggers.vertex_start;
}

static void M_PrepareRoomPortals(
    const M_PRIV *const p, M_ROOM_MESH *const mesh, const ROOM *const room)
{
    mesh->portals.vertex_start = p->vertices->count;
    const RGBA_8888 color = { 0, 0, 255, 255 };
    if (room->portals == nullptr) {
        mesh->portals.vertex_count = 0;
        return;
    }
    for (int32_t i = 0; i < room->portals->count; i++) {
        const XYZ_16 *const portal = room->portals->portal[i].vertex;
        const XYZW_F positions[4] = {
            { portal[0].x, portal[0].y, portal[0].z, 0.0f },
            { portal[1].x, portal[1].y, portal[1].z, 0.0f },
            { portal[2].x, portal[2].y, portal[2].z, 0.0f },
            { portal[3].x, portal[3].y, portal[3].z, 0.0f },
        };
        const int32_t indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
        for (int32_t j = 0; j < 8; j++) {
            const M_VERTEX vertex = {
                .pos = positions[indices[j]],
                .color = color,
            };
            Vector_Add(p->vertices, &vertex);
        }
    }
    mesh->portals.vertex_count =
        p->vertices->count - mesh->portals.vertex_start;
}

static void M_FreeRoom(M_ROOM_MESH *const mesh)
{
}

static void M_PrepareBuffers(M_PRIV *const p)
{
    p->mesh_count = Room_GetCount();
    p->meshes = Memory_Alloc(sizeof(M_ROOM_MESH) * p->mesh_count);
    p->vertices = Vector_Create(sizeof(M_VERTEX));

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        M_PrepareRoomTriggers(p, &p->meshes[i], Room_Get(i));
        M_PrepareRoomPortals(p, &p->meshes[i], Room_Get(i));
    }

    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    GFX_TRACK_DATA(
        glBufferData, GL_ARRAY_BUFFER, p->vertices->count * sizeof(M_VERTEX),
        Vector_GetData(p->vertices), GL_STATIC_DRAW);
}

static void M_FreeBuffers(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
            M_FreeRoom(&p->meshes[i]);
        }
        Memory_FreePointer(&p->meshes);
    }
    if (p->vertices != nullptr) {
        Vector_Free(p->vertices);
        p->vertices = nullptr;
    }
}

static void M_RenderBegin(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->scheduled);
}

static void M_RenderPass(
    const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    M_PRIV *const p = &m_Priv;
    if (pass != SCENE_PASS_TRANSPARENT) {
        return;
    }

    glBindVertexArray(p->vao);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    glVertexAttrib3f(OUTPUT_MESH_ATTR_NORMAL, 0.0f, 0.0f, 0.0f);
    glVertexAttrib3f(OUTPUT_MESH_ATTR_UVW, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(OUTPUT_MESH_ATTR_TEXTURE_SIZE, 0.0f, 0.0f, 1.0f, 1.0f);
    glVertexAttrib2f(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO, 1.0f, 1.0f);
    glVertexAttribI1ui(
        OUTPUT_MESH_ATTR_FLAGS,
        VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_CAUSTICS);
    glVertexAttrib1f(OUTPUT_MESH_ATTR_SHADE, SHADE_NEUTRAL);

    glDepthMask(GL_FALSE);
    for (int32_t i = 0; i < p->scheduled->count; i++) {
        const M_INSTANCE *const instance = Vector_Get(p->scheduled, i);
        const M_ROOM_MESH *const mesh =
            &p->meshes[Room_GetNumber(instance->room)];
        Output_Shader_UploadViewModelMatrix(p->shader, &instance->matrix);
        if (g_Config.debug.enable_debug_triggers) {
            glDrawArrays(
                GL_TRIANGLES, mesh->triggers.vertex_start,
                mesh->triggers.vertex_count);
        }

        if (g_Config.debug.enable_debug_portals) {
            GLint bound_polygon_mode[2];
            glDisable(GL_DEPTH_TEST);
            glGetIntegerv(GL_POLYGON_MODE, &bound_polygon_mode[0]);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArrays(
                GL_LINES, mesh->portals.vertex_start,
                mesh->portals.vertex_count);
            glPolygonMode(GL_FRONT_AND_BACK, bound_polygon_mode[0]);
            glEnable(GL_DEPTH_TEST);
        }
    }
    glDepthMask(GL_TRUE);
}

static bool M_IsDirty(const SCENE_SOURCE *const source, const SCENE_PASS pass)
{
    const M_PRIV *const p = &m_Priv;
    return pass == SCENE_PASS_TRANSPARENT && p->scheduled->count > 0;
}

void OutputSource_RoomsDebug_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->shader = Output_GetMeshShader();
    p->scheduled = Vector_Create(sizeof(M_INSTANCE));
    p->source.render_begin = M_RenderBegin;
    p->source.render_pass = M_RenderPass;
    p->source.is_dirty = M_IsDirty;
    SceneCompositor_AddSource(&p->source);

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);

    glGenBuffers(1, &p->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbo);

    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_POS);
    glEnableVertexAttribArray(OUTPUT_MESH_ATTR_COLOR);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_NORMAL);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_UVW);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TEXTURE_SIZE);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_TRAPEZOID_RATIO);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_FLAGS);
    glDisableVertexAttribArray(OUTPUT_MESH_ATTR_SHADE);
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_POS, 4, GL_FLOAT, GL_FALSE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, pos));
    glVertexAttribPointer(
        OUTPUT_MESH_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(M_VERTEX),
        (void *)(intptr_t)offsetof(M_VERTEX, color));
}

void OutputSource_RoomsDebug_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->scheduled != nullptr) {
        Vector_Free(p->scheduled);
    }
    M_FreeBuffers(p);
}

void OutputSource_RoomsDebug_ObserveLevelLoad(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareBuffers(p);
}

void OutputSource_RoomsDebug_ObserveLevelUnload(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeBuffers(p);
}

void OutputSource_RoomsDebug_ObserveRoomFlip(const ROOM *const room)
{
    M_PRIV *const p = &m_Priv;
    if (room->flip_status == RFS_UNFLIPPED && room->flipped_room != NO_ROOM) {
        const int16_t room_1 = Room_GetNumber(room);
        const int16_t room_2 = room->flipped_room;
        SWAP(p->meshes[room_1], p->meshes[room_2]);
    }
}

void OutputSource_RoomsDebug_StageRoom(const ROOM *const room)
{
    M_PRIV *const p = &m_Priv;
    Vector_Add(
        p->scheduled, &(M_INSTANCE) { .room = room, .matrix = *g_MatrixPtr });
}
