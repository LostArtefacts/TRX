#include "game/output/sources/rooms.h"

#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/utils.h"
#include "game/random.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

typedef struct {
    MESH_BATCHER *batcher;
    int32_t shade_table[WIBBLE_SIZE];
    int32_t caustics_table[WIBBLE_SIZE];
    size_t mesh_count;
    OUTPUT_MESH **meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static int16_t M_ShadeCaustics(
    const M_PRIV *p, const ROOM *room, const bool is_water_effect,
    int16_t source, int32_t vtx_idx);
static void M_UpdateShades(MESH_INSTANCE *inst, void *user_data);
static void M_PrepareMeshes(M_PRIV *p);
static void M_FreeMeshes(M_PRIV *p);

static int16_t M_ShadeCaustics(
    const M_PRIV *const p, const ROOM *const room, const bool is_water_effect,
    int16_t source, int32_t vtx_idx)
{
    if (is_water_effect) {
        const uint8_t caustic =
            p->caustics_table
                [(room->mesh.num_vertices - vtx_idx) % WIBBLE_SIZE];
        source +=
            p->shade_table[((uint8_t)Output_GetTime() + caustic) % WIBBLE_SIZE];
        CLAMP(source, 0, SHADE_MAX);
    } else {
        CLAMPG(source, SHADE_MAX);
    }
    return source;
}

static void M_UpdateShades(MESH_INSTANCE *const inst, void *const user_data)
{
    M_PRIV *const p = &m_Priv;
    const ROOM *const room = user_data;
    OUTPUT_MESH *const mesh = p->meshes[Room_GetNumber(room)];
    OUTPUT_MESH_VERTEX *vertex = Vector_GetData(mesh->vertices);

    if (!g_Config.rendering.enable_lighting) {
        for (int32_t i = 0; i < mesh->vertices->count; i++) {
            vertex[i].shade = SHADE_NEUTRAL;
        }
        return;
    }

    // Quads
    for (int32_t i = 0; i < room->mesh.num_face4s; i++) {
        const FACE4 *const face = &room->mesh.face4s[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            const int32_t k = OUTPUT_QUAD_TO_FAN(j);
            vertex->shade = room->mesh.vertices[face->vertices[k]].light_adder;
            vertex->shade = M_ShadeCaustics(
                p, room, inst->water_effect, vertex->shade, face->vertices[k]);
            vertex++;
        }
    }

    // Triangles
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        const FACE3 *const face = &room->mesh.face3s[i];
        for (int32_t j = 0; j < OUTPUT_TRI_VERTICES; j++) {
            const int32_t k = OUTPUT_TRI_TO_FAN(j);
            vertex->shade = room->mesh.vertices[face->vertices[k]].light_adder;
            vertex->shade = M_ShadeCaustics(
                p, room, inst->water_effect, vertex->shade, face->vertices[k]);
            vertex++;
        }
    }

    // Sprites
    for (int32_t i = 0; i < room->mesh.num_sprites; i++) {
        const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[i];
        for (int32_t j = 0; j < OUTPUT_QUAD_VERTICES; j++) {
            vertex->shade =
                room->mesh.vertices[room_sprite->vertex].light_adder;
            vertex++;
        }
    }
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Room_GetCount();
    p->meshes = Memory_Alloc(sizeof(OUTPUT_MESH *) * p->mesh_count);
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        OUTPUT_MESH *const mesh = Output_Mesh_Create();

        for (int32_t j = 0; j < room->mesh.num_face4s; j++) {
            Output_Mesh_AddRoomFace4(mesh, &room->mesh.face4s[j], room);
        }
        for (int32_t j = 0; j < room->mesh.num_face3s; j++) {
            Output_Mesh_AddRoomFace3(mesh, &room->mesh.face3s[j], room);
        }
        for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
            Output_Mesh_AddRoomSprite(mesh, &room->mesh.sprites[j], room);
        }
        Output_Mesh_Seal(mesh);
        MeshBatcher_AddMesh(p->batcher, mesh);

        p->meshes[i] = mesh;
    }
}

static void M_FreeMeshes(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
            MeshBatcher_RemoveMesh(p->batcher, p->meshes[i]);
            Output_Mesh_Destroy(p->meshes[i]);
        }
        Memory_FreePointer(&p->meshes);
    }
}

void OutputSource_Rooms_Init(MESH_BATCHER *const batcher)
{
    M_PRIV *const p = &m_Priv;
    p->batcher = batcher;
    for (int32_t i = 0; i < WIBBLE_SIZE; i++) {
        const int16_t angle = (i * DEG_360) / WIBBLE_SIZE;
        p->shade_table[i] = Math_Sin(angle) * SHADE_CAUSTICS >> W2V_SHIFT;
        p->caustics_table[i] = (Random_GetDraw() >> 5) - 0x01FF;
    }
}

void OutputSource_Rooms_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
}

void OutputSource_Rooms_ObserveLevelLoad(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
    M_PrepareMeshes(p);
}

void OutputSource_Rooms_ObserveLevelUnload(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
}

void OutputSource_Rooms_ObserveRoomFlip(const ROOM *const room)
{
    if (room->flip_status == RFS_UNFLIPPED && room->flipped_room != NO_ROOM) {
        const int16_t room_1 = Room_GetNumber(room);
        const int16_t room_2 = room->flipped_room;
        SWAP2(m_Priv.meshes[room_1], m_Priv.meshes[room_2]);
    }
}

void OutputSource_Rooms_StageRoom(const ROOM *const room)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh = p->meshes[Room_GetNumber(room)];
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .matrix = *g_MatrixPtr,
        .tint = Output_GetTint(),
        .wibble = Output_GetWibbleEffect(),
        .water_effect = Output_GetWaterEffect(),
        .enable_scissor = true,
        .scissor = {
            .x = room->bound_left,
            .y = room->bound_bottom,
            .width = room->bound_right - room->bound_left,
            .height = room->bound_bottom - room->bound_top,
        },
        .update_light_func = M_UpdateShades,
        .update_light_func_data = (void *)room,
    };
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_MESHES);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
}
