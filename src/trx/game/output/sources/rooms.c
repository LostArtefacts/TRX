#include <trx/game/output/sources/rooms.h>

#include <trx/config.h>
#include <trx/game/output.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/random.h>
#include <trx/memory.h>
#include <trx/utils.h>
#include <trx/version.h>

typedef struct {
    MESH_BATCHER *batcher;
    int32_t shade_table[WIBBLE_SIZE];
    int32_t caustics_table[WIBBLE_SIZE];
    size_t mesh_count;
    OUTPUT_MESH **meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static SCENE_PASS M_GetScenePass(const FACE *const face)
{
    return Output_Textures_GetObjectTextureScenePass(face->texture_idx);
}

static void M_AddRoomFace(
    MESH_BUILDER *const builder, const FACE *const face, const ROOM *const room)
{
    for (int32_t i = 0; i < face->vertex_count; i++) {
        const ROOM_VERTEX *const room_vert =
            &room->mesh.vertices[face->vertices[i]];

        uint16_t flags = 0;
        if (room_vert->is_wibble_disabled) {
            flags |= VERT_NO_CAUSTICS;
        }
        if (Output_Textures_GetObjectTextureScenePass(face->texture_idx)
            == SCENE_PASS_OPAQUE) {
            flags |= VERT_NO_ALPHA_DISCARD;
        }
        flags |= VERT_USE_DYNAMIC_LIGHT;

        const XYZ_16 *const pos = &room_vert->pos;
        const OUTPUT_MESH_VERTEX vertex = {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
            .flags = flags,
            .uvw_idx = Output_Textures_GetObjectUVWIndex(face->texture_idx, i),
            .shade = room_vert->light_base,
            .light_table_idx = room_vert->light_table_value,
            .color = room_vert->color,
            .trapezoid_ratio = {
                [0] = face->texture_zw[i].z,
                [1] = face->texture_zw[i].w,
            },
        };
        MeshBuilder_AddVertex(builder, &vertex);
    }
    MeshBuilder_AddFan(builder, M_GetScenePass(face), face->double_sided);
}

static int16_t M_ShadeCaustics(
    const M_PRIV *const p, const ROOM *const room, const bool is_water_effect,
    int16_t source, int32_t vtx_idx)
{
    if (is_water_effect) {
        const uint8_t caustic =
            p->caustics_table
                [(room->mesh.num_vertices - vtx_idx) % WIBBLE_SIZE];
        source +=
            p->shade_table
                [((int32_t)Output_GetTimeInGame() + caustic) % WIBBLE_SIZE];
        CLAMP(source, 0, SHADE_MAX);
    } else {
        CLAMPG(source, SHADE_MAX);
    }
    return source;
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Room_GetCount();
    p->meshes = Memory_Alloc(sizeof(OUTPUT_MESH *) * p->mesh_count);

    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->mesh.all_faces.count; j++) {
            M_AddRoomFace(builder, &room->mesh.all_faces.data[j], room);
        }

        int32_t stack = 0;
        XYZ_16 prev_pos = { -1, -1, -1 };
        for (int32_t j = 0; j < room->mesh.sprites.count; j++) {
            const ROOM_SPRITE *const sprite = &room->mesh.sprites.data[j];
            const ROOM_VERTEX *const vert =
                &room->mesh.vertices[sprite->vertex];
            if (vert->pos.x == prev_pos.x && vert->pos.z == prev_pos.z) {
                stack++;
            } else {
                stack = 0;
            }
            MeshBuilder_AddRoomSprite(
                builder, sprite, room, stack * -0.005f, VERT_USE_DYNAMIC_LIGHT);
            prev_pos = vert->pos;
        }

        OUTPUT_MESH *const mesh = MeshBuilder_Seal(builder);
        if (mesh != nullptr) {
            MeshBatcher_AddMesh(p->batcher, mesh);
        }

        p->meshes[i] = mesh;
    }
    MeshBuilder_Destroy(builder);
}

static void M_FreeMeshes(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
            MeshBatcher_RemoveMesh(p->batcher, p->meshes[i]);
            if (p->meshes[i] != nullptr) {
                Output_Mesh_Destroy(p->meshes[i]);
            }
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
        SWAP(m_Priv.meshes[room_1], m_Priv.meshes[room_2]);
    }
}

void OutputSource_Rooms_StageRoom(const ROOM *const room)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh = p->meshes[Room_GetNumber(room)];
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
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
        .room = Output_GetCurrentRoom(),
    };
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_BLEND_ADD);
}
