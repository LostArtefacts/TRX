#include <trx/game/output/sources/objects.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/output/state.h>
#include <trx/version.h>

typedef struct {
    OUTPUT_MESH *mesh_batch;
    int32_t *light_idx_map;
} M_MESH;

typedef struct {
    MESH_BATCHER *batcher;
    int16_t skybox_shade;
    size_t mesh_count;
    M_MESH *meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static bool M_IsMeshSkybox(const int32_t mesh_idx)
{
    const OBJECT *const object = Object_Get(O_SKYBOX);
    if (!object->loaded) {
        return false;
    }
    return mesh_idx >= object->mesh_idx
        && mesh_idx < object->mesh_idx + object->mesh_count;
}

static SCENE_PASS M_GetScenePass(const FACE *const face, const uint16_t flags)
{
    if ((flags & VERT_FLAT_SHADED) != 0) {
        return SCENE_PASS_OPAQUE;
    }
    return Output_Textures_GetObjectTextureScenePass(face->texture_idx);
}

static void M_AddObjectFace(
    MESH_BUILDER *const builder, const OBJECT_MESH *const obj_mesh,
    const FACE *const face, uint16_t flags)
{
    RGBA_8888 color = (RGBA_8888) { 255, 255, 255, 255 };
    int32_t uvw_idx = -1;

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

    if (obj_mesh->num_lights <= 0) {
        flags |= VERT_USE_OWN_LIGHT;
    } else {
        flags |= VERT_USE_OBJECT_LIGHT;
    }

    for (int32_t i = 0; i < face->vertex_count; i++) {
        const int32_t shade = obj_mesh->num_lights <= 0
                && face->vertices[i] < -obj_mesh->num_lights
            ? obj_mesh->lighting.lights[face->vertices[i]]
            : SHADE_NEUTRAL;
        const XYZ_16 normal = face->vertices[i] < obj_mesh->num_lights
            ? obj_mesh->lighting.normals[face->vertices[i]]
            : (XYZ_16) { 1, 0, 0 };
        const XYZ_16 *const pos = &obj_mesh->vertices[face->vertices[i]];

        if ((flags & VERT_FLAT_SHADED) == 0) {
            uvw_idx = Output_Textures_GetObjectUVWIndex(face->texture_idx, i);
        }
        const OUTPUT_MESH_VERTEX vertex = {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
            .normal = { .x = normal.x, .y = normal.y, .z = normal.z },
            .flags = flags,
            .uvw_idx = uvw_idx,
            .shade = shade,
            .color = color,
            .trapezoid_ratio = {
                [0] = face->texture_zw[i].z,
                [1] = face->texture_zw[i].w,
            },
        };
        MeshBuilder_AddVertex(builder, &vertex);
    }
    MeshBuilder_AddFan(
        builder, M_GetScenePass(face, flags), face->double_sided);
}

static int32_t *M_PrepareLightIndexMap(
    const OBJECT_MESH *const obj_mesh, const int32_t vertex_count)
{
    int32_t v = 0;
    int32_t *const light_idx_map = Memory_Alloc(sizeof(int32_t) * vertex_count);
    for (int32_t i = 0; i < obj_mesh->all_faces.count; i++) {
        const FACE *const face = &obj_mesh->all_faces.data[i];
        for (int32_t j = 0; j < face->vertex_count; j++) {
            light_idx_map[v++] = face->vertices[j];
        }
    }
    return light_idx_map;
}

static void M_PrepareMeshes(M_PRIV *const p)
{
    p->mesh_count = Object_GetMeshCount();
    p->meshes = Memory_Alloc(sizeof(M_MESH) * p->mesh_count);

    MESH_BUILDER *const builder = MeshBuilder_Create();
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const obj_mesh = Object_GetMesh(i);
        M_MESH *const new_batch = &p->meshes[i];

        uint16_t flags = 0;
        if (obj_mesh->enable_reflections) {
            flags |= VERT_REFLECTIVE;
        }

        if (M_IsMeshSkybox(i)) {
            flags |= VERT_USE_OWN_LIGHT;
        }
        for (int32_t j = 0; j < obj_mesh->tex_faces.count; j++) {
            M_AddObjectFace(
                builder, obj_mesh, &obj_mesh->tex_faces.data[j], flags);
        }
        for (int32_t j = 0; j < obj_mesh->flat_faces.count; j++) {
            M_AddObjectFace(
                builder, obj_mesh, &obj_mesh->flat_faces.data[j],
                flags | VERT_FLAT_SHADED);
        }

        MeshBuilder_AdjustDepth(builder, obj_mesh->depth_adjustment);
        OUTPUT_MESH *const mesh = MeshBuilder_Seal(builder);
        if (mesh != nullptr) {
            MeshBatcher_AddMesh(p->batcher, mesh);
            new_batch->mesh_batch = mesh;
            new_batch->light_idx_map =
                M_PrepareLightIndexMap(obj_mesh, mesh->vertices->count);
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
            Memory_FreePointer(&p->meshes[i].light_idx_map);
        }
        Memory_FreePointer(&p->meshes);
    }
}

static void M_UpdateShadesSkybox(
    MESH_INSTANCE *const inst, void *const user_data)
{
    const OBJECT_MESH *const mesh = user_data;
    const M_PRIV *const p = &m_Priv;

    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    if (batch->mesh_batch == nullptr) {
        return;
    }
    OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(batch->mesh_batch->vertices);

    const int16_t shade =
        g_Config.rendering.enable_lighting ? p->skybox_shade : SHADE_NEUTRAL;
    for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
        vertices[i].shade = shade;
    }
}

static void M_UpdateFlags(const OBJECT_MESH *const mesh, M_MESH *const batch)
{
    uint16_t mask = VERT_REFLECTIVE | VERT_NO_LIGHTING;
    uint16_t flags = 0;
    if (mesh->enable_reflections) {
        flags |= VERT_REFLECTIVE;
    }
    OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(batch->mesh_batch->vertices);
    for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
        vertices[i].flags &= ~mask;
        vertices[i].flags |= flags;
    }
}

static void M_Stage(const OBJECT_MESH *const mesh)
{
    M_PRIV *const p = &m_Priv;
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    if (batch->mesh_batch == nullptr) {
        return;
    }

    OUTPUT_LIGHT_INFO light_info = Output_GetLightInfo();
    if (g_TRVersion >= 3 && M_IsMeshSkybox(Object_GetMeshIndex(mesh))) {
        light_info.tr3_ambient = (RGB_F) { 1.0f, 1.0f, 1.0f };
        for (int32_t i = 0; i < 3; i++) {
            light_info.tr3_light_color[i] = (RGB_F) { 0.0f, 0.0f, 0.0f };
            light_info.tr3_light_dir_view[i] = (XYZ_32) { 0, 0, 0 };
        }
    }

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

void OutputSource_Objects_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
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

void OutputSource_Objects_StageSkyboxMesh(
    const OBJECT_MESH *const mesh, const int16_t shade)
{
    M_PRIV *const p = &m_Priv;
    p->skybox_shade = shade;
    M_Stage(mesh);
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
