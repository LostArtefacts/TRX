#include <trx/game/output/sources/objects.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/output/state.h>
#include <trx/version.h>

typedef struct {
    OUTPUT_MESH *mesh_batch;
} M_MESH;

typedef struct {
    MEMORY_ARENA_ALLOCATOR alloc;
    MESH_BATCHER *batcher;
    size_t mesh_count;
    M_MESH *meshes;
} M_PRIV;

typedef struct {
    int32_t mesh_idx;
    const OUTPUT_OBJECT_MESH_POLICY *policy;
} M_POLICY_ENTRY;

typedef void (*M_FACE_VERTEX_FUNC)(
    OUTPUT_MESH_VERTEX *vertex, const FACE *face, int32_t vertex_idx,
    const void *user);

typedef struct {
    const XYZ_F *positions;
    const XYZ_F *normals;
} M_GEOMETRY_UPDATE;

static M_PRIV m_Priv = {};
static VECTOR *m_MeshPolicies = nullptr;

// Returns the policy of the idx-th registration for the given mesh, or
// nullptr once idx runs past them; registration order is preserved.
static const OUTPUT_OBJECT_MESH_POLICY *M_GetMeshPolicy(
    const int32_t mesh_idx, const int32_t idx)
{
    if (m_MeshPolicies == nullptr) {
        return nullptr;
    }
    int32_t left = idx;
    for (int32_t i = 0; i < m_MeshPolicies->count; i++) {
        const M_POLICY_ENTRY *const entry = Vector_Get(m_MeshPolicies, i);
        if (entry->mesh_idx == mesh_idx && left-- == 0) {
            return entry->policy;
        }
    }
    return nullptr;
}

static uint16_t M_GetPolicyVertexFlags(const int32_t mesh_idx)
{
    uint16_t flags = 0;
    const OUTPUT_OBJECT_MESH_POLICY *policy;
    for (int32_t i = 0; (policy = M_GetMeshPolicy(mesh_idx, i)) != nullptr;
         i++) {
        flags |= policy->vertex_flags;
    }
    return flags;
}

static SCENE_PASS M_GetScenePass(
    const FACE *const face, const uint16_t flags, const int32_t mesh_idx)
{
    // Half-opacity faces draw depth-sorted, like the PSX half blend mode.
    if (face->semi_transparent) {
        return SCENE_PASS_TRANSPARENT;
    }
    if ((flags & VERT_FLAT_SHADED) != 0) {
        return SCENE_PASS_OPAQUE;
    }
    if ((face->effects & 0x1u) != 0u) {
        SCENE_PASS pass = SCENE_PASS_BLEND_ADD;
        const OUTPUT_OBJECT_MESH_POLICY *policy;
        for (int32_t i = 0; (policy = M_GetMeshPolicy(mesh_idx, i)) != nullptr;
             i++) {
            if (policy->get_face_pass != nullptr
                && policy->get_face_pass(face, &pass)) {
                break;
            }
        }
        return pass;
    }
    return Output_Textures_GetObjectTextureScenePass(face->texture_idx);
}

static float M_GetReflectivity(const FACE *const face)
{
    if ((face->effects & 0x2u) == 0u) {
        return 1.0f;
    }

    // The OG scales the reflection pass's vertex color by the face's 5-bit
    // reflectivity: (value << 3), applied as (color * m) >> 8.
    return ((face->effects >> 2) & 0x1Fu) * (8.0f / 256.0f);
}

static bool M_IsReflectiveFace(
    const OBJECT_MESH *const obj_mesh, const FACE *const face)
{
    return obj_mesh->enable_reflections || face->enable_reflections;
}

static void M_AddObjectFace(
    MESH_BUILDER *const builder, const OBJECT_MESH *const obj_mesh,
    const FACE *const face, uint16_t flags, const int32_t mesh_idx)
{
    RGBA_8888 color = COLOR_RGBA_8888_WHITE;
    OUTPUT_MESH_VERTEX vertices[4];
    int32_t uvw_idx = -1;

    ASSERT(face->vertex_count <= 4);

    if (flags & VERT_FLAT_SHADED) {
        if (g_TRVersion == 1) {
            color = Output_RGB2RGBA(Output_GetPaletteColor8(face->palette_idx));
        } else {
            color = Output_RGB2RGBA(
                Output_GetPaletteColor16(face->palette_idx >> 8));
        }
    } else if (
        Output_Textures_GetObjectTextureScenePass(face->texture_idx)
        == SCENE_PASS_OPAQUE) {
        flags |= VERT_NO_ALPHA_DISCARD;
    }

    if (M_IsReflectiveFace(obj_mesh, face)) {
        flags |= VERT_REFLECTIVE;
    }

    if (obj_mesh->num_lights <= 0) {
        flags |= VERT_USE_OWN_LIGHT;
    } else {
        flags |= VERT_USE_OBJECT_LIGHT;
    }

    if (face->semi_transparent) {
        // The transparent pass blends premultiplied alpha; the shader only
        // premultiplies flat colors, so textured faces additionally need the
        // color scaled here for the face to render at half opacity.
        color.a = 128;
        if ((flags & VERT_FLAT_SHADED) == 0) {
            color.r = 128;
            color.g = 128;
            color.b = 128;
        }
    }

    const bool may_recolor = (flags & VERT_FLAT_SHADED) == 0;

    for (int32_t i = 0; i < face->vertex_count; i++) {
        if (may_recolor) {
            const OUTPUT_OBJECT_MESH_POLICY *policy;
            for (int32_t j = 0;
                 (policy = M_GetMeshPolicy(mesh_idx, j)) != nullptr; j++) {
                if (policy->get_vertex_color != nullptr
                    && policy->get_vertex_color(face, i, &color)) {
                    break;
                }
            }
        }
        int32_t shade = obj_mesh->num_lights <= 0
                && face->vertices[i] < -obj_mesh->num_lights
            ? obj_mesh->lighting.lights[face->vertices[i]]
            : SHADE_NEUTRAL;
        {
            const OUTPUT_OBJECT_MESH_POLICY *policy;
            for (int32_t j = 0;
                 (policy = M_GetMeshPolicy(mesh_idx, j)) != nullptr; j++) {
                if (policy->get_vertex_shade != nullptr
                    && policy->get_vertex_shade(face, i, &shade)) {
                    break;
                }
            }
        }
        const XYZ_16 normal = face->vertices[i] < obj_mesh->num_lights
            ? obj_mesh->lighting.normals[face->vertices[i]]
            : (XYZ_16) { 1, 0, 0 };
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[i]];

        if ((flags & VERT_FLAT_SHADED) == 0) {
            uvw_idx = Output_Textures_GetObjectUVWIndex(face->texture_idx, i);
        }
        vertices[i] = (OUTPUT_MESH_VERTEX) {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
            .normal = { .x = normal.x, .y = normal.y, .z = normal.z },
            .flags = flags,
            .uvw_idx = uvw_idx,
            .reflectivity = M_GetReflectivity(face),
            .shade = shade,
            .color = color,
            .trapezoid_ratio = {
                [0] = face->texture_zw[i].z,
                [1] = face->texture_zw[i].w,
            },
        };
    }

    MeshBuilder_AddVertices(builder, vertices, face->vertex_count);
    // Half-opacity faces skip the depth prepass, so they cannot clip other
    // effects drawn at the same spot (e.g. the gun flash glow).
    MeshBuilder_AddFan(
        builder, M_GetScenePass(face, flags, mesh_idx), face->double_sided,
        !face->semi_transparent);
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Object_GetMeshCount();
    p->meshes = Memory_ArenaAlloc(&p->alloc, sizeof(M_MESH) * p->mesh_count);

    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const obj_mesh = Object_GetMesh(i);
        M_MESH *const new_batch = &p->meshes[i];

        uint16_t flags = 0;
        if (obj_mesh->enable_reflections) {
            flags |= VERT_REFLECTIVE;
        }

        flags |= M_GetPolicyVertexFlags(i);
        for (int32_t j = 0; j < obj_mesh->tex_faces.count; j++) {
            M_AddObjectFace(
                builder, obj_mesh, &obj_mesh->tex_faces.data[j], flags, i);
        }
        for (int32_t j = 0; j < obj_mesh->flat_faces.count; j++) {
            M_AddObjectFace(
                builder, obj_mesh, &obj_mesh->flat_faces.data[j],
                flags | VERT_FLAT_SHADED, i);
        }

        MeshBuilder_AdjustDepth(builder, obj_mesh->depth_adjustment);
        OUTPUT_MESH *const mesh = MeshBuilder_Seal(builder);
        if (mesh != nullptr) {
            MeshBatcher_AddMesh(p->batcher, mesh);
            new_batch->mesh_batch = mesh;
        }
    }
    MeshBuilder_Destroy(builder);
}

static void M_FreeMeshes(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
            MeshBatcher_RemoveMesh(p->batcher, p->meshes[i].mesh_batch);
            if (p->meshes[i].mesh_batch != nullptr) {
                Output_Mesh_Destroy(p->meshes[i].mesh_batch);
            }
        }
        p->meshes = nullptr;
    }
    Memory_ArenaReset(&p->alloc);
}

// The GPU buffer stores one vertex per face corner, in the same order the
// faces were flattened in M_PrepareMeshes (tex faces, then flat faces), so
// this walk mirrors that layout.
static void M_ForEachFaceVertex(
    const OBJECT_MESH *const mesh, M_MESH *const batch,
    const M_FACE_VERTEX_FUNC func, const void *const user)
{
    OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(batch->mesh_batch->vertices);
    int32_t vertex_idx = 0;

    const struct {
        int16_t count;
        const FACE *data;
    } face_lists[] = {
        { mesh->tex_faces.count, mesh->tex_faces.data },
        { mesh->flat_faces.count, mesh->flat_faces.data },
    };
    for (int32_t list = 0; list < 2; list++) {
        for (int32_t i = 0; i < face_lists[list].count; i++) {
            const FACE *const face = &face_lists[list].data[i];
            for (int32_t j = 0; j < face->vertex_count; j++) {
                func(&vertices[vertex_idx + j], face, j, user);
            }
            vertex_idx += face->vertex_count;
        }
    }
}

static void M_UpdateVertexFlags(
    OUTPUT_MESH_VERTEX *const vertex, const FACE *const face,
    const int32_t vertex_idx, const void *const user)
{
    const OBJECT_MESH *const mesh = user;
    vertex->flags &= ~(VERT_REFLECTIVE | VERT_NO_LIGHTING);
    if (M_IsReflectiveFace(mesh, face)) {
        vertex->flags |= VERT_REFLECTIVE;
    }
}

static void M_ResyncVertexGeometry(
    OUTPUT_MESH_VERTEX *const vertex, const FACE *const face,
    const int32_t vertex_idx, const void *const user)
{
    const M_GEOMETRY_UPDATE *const update = user;
    const int32_t idx = face->vertices[vertex_idx];
    const XYZ_F *const pos = &update->positions[idx];
    vertex->pos.x = pos->x;
    vertex->pos.y = pos->y;
    vertex->pos.z = pos->z;
    if (update->normals != nullptr) {
        vertex->normal = update->normals[idx];
    }
}

static void M_UpdateFlags(const OBJECT_MESH *const mesh, M_MESH *const batch)
{
    M_ForEachFaceVertex(mesh, batch, M_UpdateVertexFlags, mesh);
}

static void M_ResyncGeometry(
    const OBJECT_MESH *const mesh, M_MESH *const batch,
    const XYZ_F *const positions, const XYZ_F *const normals)
{
    const M_GEOMETRY_UPDATE update = {
        .positions = positions,
        .normals = normals,
    };
    M_ForEachFaceVertex(mesh, batch, M_ResyncVertexGeometry, &update);
}

static void M_Stage(const OBJECT_MESH *const mesh)
{
    M_PRIV *const p = &m_Priv;
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    if (batch->mesh_batch == nullptr) {
        return;
    }

    OUTPUT_LIGHT_INFO light_info = Output_GetLightInfo();
    const OUTPUT_OBJECT_MESH_POLICY *policy;
    for (int32_t i = 0;
         (policy = M_GetMeshPolicy(Object_GetMeshIndex(mesh), i)) != nullptr;
         i++) {
        if (policy->adjust_light_info != nullptr) {
            policy->adjust_light_info(&light_info);
        }
    }
    Output_Lights_FillInstanceLight(&light_info, g_WMatrixPtr);

    const VIEWPORT_RECT *const scissor = Output_GetObjectScissor();
    const MESH_INSTANCE inst = {
        .mesh = batch->mesh_batch,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        .tint = Output_GetTint(),
        .wibble = false,
        .water_effect =
            (mesh->enable_caustics && Output_GetWaterEffect()) ? 1 : 0,
        .light_info = light_info,
        .room = Output_GetCurrentRoom(),
        .enable_scissor = scissor != nullptr,
        .scissor = scissor != nullptr ? *scissor : (VIEWPORT_RECT) {},
    };
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_BLEND_ADD);
}

void OutputSource_Objects_Init(MESH_BATCHER *const batcher)
{
    M_PRIV *const p = &m_Priv;
    p->batcher = batcher;
}

void OutputSource_Objects_AddMeshPolicy(
    const int32_t mesh_idx, const OUTPUT_OBJECT_MESH_POLICY *const policy)
{
    if (m_MeshPolicies == nullptr) {
        m_MeshPolicies = Vector_Create(sizeof(M_POLICY_ENTRY));
    }
    const M_POLICY_ENTRY entry = {
        .mesh_idx = mesh_idx,
        .policy = policy,
    };
    Vector_Add(m_MeshPolicies, &entry);
}

void OutputSource_Objects_RemoveMeshPolicy(
    const OUTPUT_OBJECT_MESH_POLICY *const policy)
{
    if (m_MeshPolicies == nullptr) {
        return;
    }
    for (int32_t i = m_MeshPolicies->count - 1; i >= 0; i--) {
        const M_POLICY_ENTRY *const entry = Vector_Get(m_MeshPolicies, i);
        if (entry->policy == policy) {
            Vector_RemoveAt(m_MeshPolicies, i);
        }
    }
}

void OutputSource_Objects_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
    Memory_ArenaFree(&p->alloc);
    if (m_MeshPolicies != nullptr) {
        Vector_Free(m_MeshPolicies);
        m_MeshPolicies = nullptr;
    }
}

void OutputSource_Objects_ObserveLevelLoad(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
    M_PrepareMeshes(p);
}

void OutputSource_Objects_ObserveLevelUnload(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
}

void OutputSource_Objects_ObserveObjectMeshSwap(
    const int32_t mesh_idx_1, const int32_t mesh_idx_2)
{
    M_PRIV *const p = &m_Priv;
    if (p->meshes == nullptr) {
        return;
    }

    SWAP(p->meshes[mesh_idx_1], p->meshes[mesh_idx_2]);
    if (m_MeshPolicies != nullptr) {
        for (int32_t i = 0; i < m_MeshPolicies->count; i++) {
            M_POLICY_ENTRY *const entry = Vector_Get(m_MeshPolicies, i);
            if (entry->mesh_idx == mesh_idx_1) {
                entry->mesh_idx = mesh_idx_2;
            } else if (entry->mesh_idx == mesh_idx_2) {
                entry->mesh_idx = mesh_idx_1;
            }
        }
    }
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh_idx_1);
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh_idx_2);
}

void OutputSource_Objects_ObserveObjectMeshUpdate(const int32_t mesh_idx)
{
    M_PRIV *const p = &m_Priv;
    if (p->meshes == nullptr) {
        return;
    }
    M_MESH *const batch = &p->meshes[mesh_idx];
    if (batch->mesh_batch == nullptr) {
        return;
    }
    M_UpdateFlags(Object_GetMesh(mesh_idx), batch);
    MeshBatcher_UpdateMeshGeometry(p->batcher, batch->mesh_batch);
}

void OutputSource_Objects_ObserveObjectMeshGeometry(
    const int32_t mesh_idx, const XYZ_F *const positions,
    const XYZ_F *const normals)
{
    M_PRIV *const p = &m_Priv;
    if (p->meshes == nullptr) {
        return;
    }
    M_MESH *const batch = &p->meshes[mesh_idx];
    if (batch->mesh_batch == nullptr) {
        return;
    }
    M_ResyncGeometry(Object_GetMesh(mesh_idx), batch, positions, normals);
    MeshBatcher_UpdateMeshGeometry(p->batcher, batch->mesh_batch);
}

void OutputSource_Objects_StageObjectMesh(const OBJECT_MESH *const mesh)
{
    M_Stage(mesh);
}

const SCENE_SOURCE *OutputSource_Objects_GetSource(void)
{
    M_PRIV *const p = &m_Priv;
    return MeshBatcher_AsSource(p->batcher);
}
