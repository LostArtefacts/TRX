#include <trx/game/output/sources/rooms.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/output/bind.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

#include <math.h>

typedef struct {
    MESH_BATCHER *batcher;
    size_t mesh_count;
    OUTPUT_MESH **meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static SCENE_PASS M_GetScenePass(const FACE *const face)
{
    return Output_Textures_GetObjectTextureScenePass(face->texture_idx);
}

// Ports the OG CalcTriFaceNormal: N = cross(p3 - p2, p1 - p2).
static XYZ_F M_CalcTriNormal(const XYZ_F p1, const XYZ_F p2, const XYZ_F p3)
{
    const XYZ_F u = { p1.x - p2.x, p1.y - p2.y, p1.z - p2.z };
    const XYZ_F v = { p3.x - p2.x, p3.y - p2.y, p3.z - p2.z };
    return (XYZ_F) {
        .x = v.z * u.y - v.y * u.z,
        .y = v.x * u.z - v.z * u.x,
        .z = v.y * u.x - v.x * u.y,
    };
}

static XYZ_F M_NormalizeF(const XYZ_F v)
{
    const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len2 <= 0.0f) {
        return (XYZ_F) {};
    }
    const float inv_len = 1.0f / sqrtf(len2);
    return (XYZ_F) { v.x * inv_len, v.y * inv_len, v.z * inv_len };
}

static XYZ_F M_GetVertexPos(const ROOM *const room, const int32_t idx)
{
    const XYZ_16 pos = room->mesh.vertices[idx].pos;
    return (XYZ_F) { pos.x, pos.y, pos.z };
}

// Port of the OG CreateVertexNormals (drawroom.cpp): each face normal is
// accumulated into all of its vertices, then normalized. Quads use the sum
// of their two triangle normals. Needed by the TR4 room dynamic light model.
static XYZ_F *M_CalcRoomVertexNormals(const ROOM *const room)
{
    XYZ_F *const normals =
        Memory_Alloc(sizeof(XYZ_F) * room->mesh.num_vertices);

    for (int32_t i = 0; i < room->mesh.all_faces.count; i++) {
        const FACE *const face = &room->mesh.all_faces.data[i];
        XYZ_F fn = M_CalcTriNormal(
            M_GetVertexPos(room, face->vertices[0]),
            M_GetVertexPos(room, face->vertices[1]),
            M_GetVertexPos(room, face->vertices[2]));
        if (face->vertex_count == 4) {
            const XYZ_F fn2 = M_CalcTriNormal(
                M_GetVertexPos(room, face->vertices[0]),
                M_GetVertexPos(room, face->vertices[2]),
                M_GetVertexPos(room, face->vertices[3]));
            fn.x += fn2.x;
            fn.y += fn2.y;
            fn.z += fn2.z;
        }
        fn = M_NormalizeF(fn);

        for (int32_t j = 0; j < face->vertex_count; j++) {
            const uint16_t vtx_idx = face->vertices[j];
            bool seen = false;
            for (int32_t k = 0; k < j; k++) {
                if (face->vertices[k] == vtx_idx) {
                    seen = true;
                    break;
                }
            }
            if (seen) {
                continue;
            }
            normals[vtx_idx].x += fn.x;
            normals[vtx_idx].y += fn.y;
            normals[vtx_idx].z += fn.z;
        }
    }

    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        normals[i] = M_NormalizeF(normals[i]);
    }
    return normals;
}

static void M_AddRoomFace(
    MESH_BUILDER *const builder, const FACE *const face, const ROOM *const room,
    const XYZ_F *const vertex_normals)
{
    OUTPUT_MESH_VERTEX vertices[4];

    ASSERT(face->vertex_count <= 4);

    for (int32_t i = 0; i < face->vertex_count; i++) {
        const ROOM_VERTEX *const room_vert =
            &room->mesh.vertices[face->vertices[i]];

        uint16_t flags = 0;
        const bool disable_wibble = g_TRVersion == 4
            ? room_vert->flags.move
            : room_vert->flags.disable_wibble;
        if (disable_wibble) {
            flags |= VERT_NO_WIBBLE;
        }
        if (room_vert->flags.move) {
            flags |= VERT_MOVE;
        }
        if (room_vert->flags.glow) {
            flags |= VERT_GLOW;
        }
        if (Output_Textures_GetObjectTextureScenePass(face->texture_idx)
            == SCENE_PASS_OPAQUE) {
            flags |= VERT_NO_ALPHA_DISCARD;
        }
        flags |= VERT_USE_DYNAMIC_LIGHT;
        if (g_TRVersion == 4) {
            // TR4 room colors are stored in the OG 128-neutral scale; the
            // shader doubles them and splits off the overbright excess.
            flags |= VERT_OVERBRIGHT;
        }

        // Match the object mesh normal convention (14-bit fixed point).
        XYZ_F normal = {};
        if (vertex_normals != nullptr) {
            normal = vertex_normals[face->vertices[i]];
            normal.x *= 1 << 14;
            normal.y *= 1 << 14;
            normal.z *= 1 << 14;
        }

        const XYZ_16 *const pos = &room_vert->pos;
        vertices[i] = (OUTPUT_MESH_VERTEX) {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
            .normal = normal,
            .flags = flags,
            .uvw_idx = Output_Textures_GetObjectUVWIndex(face->texture_idx, i),
            .reflectivity = 1.0f,
            .shade = room_vert->light_base,
            .light_table_idx = room_vert->light_table_value,
            .color = room_vert->color,
            .trapezoid_ratio = {
                [0] = face->texture_zw[i].z,
                [1] = face->texture_zw[i].w,
            },
        };
    }
    MeshBuilder_AddVertices(builder, vertices, face->vertex_count);
    MeshBuilder_AddFan(builder, M_GetScenePass(face), face->double_sided, true);
}

static int32_t M_GetWaterEffect(const ROOM *const room)
{
    if (g_TRVersion >= 3) {
        return 2 + (int32_t)room->water_scheme;
    }
    return Output_GetWaterEffect() ? 1 : 0;
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Room_GetCount();
    p->meshes = Memory_Alloc(sizeof(OUTPUT_MESH *) * p->mesh_count);

    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        XYZ_F *vertex_normals = nullptr;
        if (g_TRVersion == 4) {
            vertex_normals = M_CalcRoomVertexNormals(room);
        }
        for (int32_t j = 0; j < room->mesh.all_faces.count; j++) {
            M_AddRoomFace(
                builder, &room->mesh.all_faces.data[j], room, vertex_normals);
        }
        Memory_FreePointer(&vertex_normals);

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
        const int16_t room_1 = Room_GetIndex(room);
        const int16_t room_2 = room->flipped_room;
        SWAP(m_Priv.meshes[room_1], m_Priv.meshes[room_2]);
    }
}

void OutputSource_Rooms_StageRoom(const ROOM *const room)
{
    M_PRIV *const p = &m_Priv;
    OUTPUT_MESH *const mesh = p->meshes[Room_GetIndex(room)];
    const OUTPUT_ROOM_BIND *const bind = Output_Bind_GetRoom(room);
    const MESH_INSTANCE inst = {
        .mesh = mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        .tint = Output_GetTint(),
        .wibble = Output_GetWibbleEffect(),
        .water_effect = M_GetWaterEffect(room),
        .enable_scissor = Room_IsOverlapping(Room_GetIndex(room)),
        .scissor = {
            .x = bind->bound_left,
            .y = bind->bound_bottom,
            .width = bind->bound_right - bind->bound_left,
            .height = bind->bound_bottom - bind->bound_top,
        },
        .room = Output_GetCurrentRoom(),
    };
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_BLEND_ADD);
}
