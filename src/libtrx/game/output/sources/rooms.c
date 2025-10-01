#include "game/output/sources/rooms.h"

#include "config.h"
#include "game/level/const.h"
#include "game/output.h"
#include "game/output/mesh_batcher/mesh_builder.h"
#include "game/random.h"
#include "memory.h"
#include "utils.h"

typedef struct {
    MESH_BATCHER *batcher;
    int32_t shade_table[WIBBLE_SIZE];
    int32_t caustics_table[WIBBLE_SIZE];
    size_t mesh_count;
    OUTPUT_MESH **meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_AddRoomVerts(
    MESH_BUILDER *const builder, const size_t vtx_count,
    const int32_t texture_idx, const uint16_t *const face_vertices,
    const TEXTURE_ZW_F *const trapezoid_ratio,
    const ROOM_VERTEX *const room_verts)
{
    for (size_t i = 0; i < vtx_count; i++) {
        const ROOM_VERTEX *const room_vert = &room_verts[face_vertices[i]];
        const XYZ_16 *const pos = &room_vert->pos;
        const OUTPUT_MESH_VERTEX vertex = {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
#if TR_VERSION == 1
            .flags = room_vert->flags & NO_VERT_MOVE ? VERT_NO_CAUSTICS : 0,
#else
            .flags = room_vert->flags != 0 ? VERT_NO_CAUSTICS : 0,
#endif
            .uvw_idx = Output_Textures_GetObjectUVWIndex(texture_idx, i),
            .shade = room_vert->light_adder,
            .color = { 255, 255, 255, 255 },
            .trapezoid_ratio = {
                [0] = trapezoid_ratio != nullptr ? trapezoid_ratio[i].z : 1.0f,
                [1] = trapezoid_ratio != nullptr ? trapezoid_ratio[i].w : 1.0f,
            },
        };
        MeshBuilder_AddVertex(builder, &vertex);
    }
}

static void M_AddRoomFace3(
    MESH_BUILDER *const builder, const FACE3 *const face,
    const ROOM *const room)
{
    M_AddRoomVerts(
        builder, 3, face->texture_idx, face->vertices, nullptr,
        room->mesh.vertices);
    MeshBuilder_AddFace3(
        builder, Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

static void M_AddRoomFace4(
    MESH_BUILDER *const builder, const FACE4 *const face,
    const ROOM *const room)
{
    M_AddRoomVerts(
        builder, 4, face->texture_idx, face->vertices, face->texture_zw,
        room->mesh.vertices);
    MeshBuilder_AddFace4(
        builder, Output_Textures_IsObjectTextureTransparent(face->texture_idx));
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
                [((uint8_t)Output_GetTimeInGame() + caustic) % WIBBLE_SIZE];
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
        for (int32_t j = 0; j < 4; j++) {
            vertex->shade = room->mesh.vertices[face->vertices[j]].light_adder;
            vertex->shade = M_ShadeCaustics(
                p, room, inst->water_effect, vertex->shade, face->vertices[j]);
            vertex++;
        }
    }

    // Triangles
    for (int32_t i = 0; i < room->mesh.num_face3s; i++) {
        const FACE3 *const face = &room->mesh.face3s[i];
        for (int32_t j = 0; j < 3; j++) {
            vertex->shade = room->mesh.vertices[face->vertices[j]].light_adder;
            vertex->shade = M_ShadeCaustics(
                p, room, inst->water_effect, vertex->shade, face->vertices[j]);
            vertex++;
        }
    }

    // Sprites
    for (int32_t i = 0; i < room->mesh.num_sprites; i++) {
        const ROOM_SPRITE *const room_sprite = &room->mesh.sprites[i];
        for (int32_t j = 0; j < 4; j++) {
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

    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->mesh.num_face4s; j++) {
            M_AddRoomFace4(builder, &room->mesh.face4s[j], room);
        }
        for (int32_t j = 0; j < room->mesh.num_face3s; j++) {
            M_AddRoomFace3(builder, &room->mesh.face3s[j], room);
        }

        int32_t stack = 0;
        XYZ_16 prev_pos = { -1, -1, -1 };
        for (int32_t j = 0; j < room->mesh.num_sprites; j++) {
            const ROOM_SPRITE *const sprite = &room->mesh.sprites[j];
            const ROOM_VERTEX *const vert =
                &room->mesh.vertices[sprite->vertex];
            if (vert->pos.x == prev_pos.x && vert->pos.z == prev_pos.z) {
                stack++;
            } else {
                stack = 0;
            }
            MeshBuilder_AddRoomSprite(builder, sprite, room, stack * -0.005f);
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
