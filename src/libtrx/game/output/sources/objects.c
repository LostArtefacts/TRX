#include "game/output/sources/objects.h"

#include "config.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/output/mesh_batcher/mesh_builder.h"
#include "game/output/state.h"
#include "memory.h"
#include "utils.h"
#include "version.h"

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

static void M_AddObjectVerts(
    MESH_BUILDER *const builder, const size_t vtx_count,
    const OBJECT_MESH *const obj_mesh, const uint16_t *vertices,
    const uint16_t texture_idx, const uint16_t palette_idx,
    const uint16_t flags, const TEXTURE_ZW_F *const trapezoid_ratio)
{
    RGBA_8888 color = (RGBA_8888) { 255, 255, 255, 255 };
    int16_t uvw_idx = -1;
    if (flags & VERT_FLAT_SHADED) {
        if (g_TRVersion == 1) {
            color = Output_RGB2RGBA(Output_GetPaletteColor8(palette_idx));
        } else {
            color = Output_RGB2RGBA(Output_GetPaletteColor16(palette_idx >> 8));
        }
    }

    for (size_t i = 0; i < vtx_count; i++) {
        const XYZ_16 normal = vertices[i] < obj_mesh->num_lights
            ? obj_mesh->lighting.normals[vertices[i]]
            : (XYZ_16) {};
        const XYZ_16 *const pos = &obj_mesh->vertices[vertices[i]];
        if ((flags & VERT_FLAT_SHADED) == 0) {
            uvw_idx = Output_Textures_GetObjectUVWIndex(texture_idx, i);
        }
        const OUTPUT_MESH_VERTEX vertex = {
            .pos = { .x = pos->x, .y = pos->y, .z = pos->z },
            .normal = { .x = normal.x, .y = normal.y, .z = normal.z },
            .flags = flags,
            .uvw_idx = uvw_idx,
            .shade = SHADE_NEUTRAL,
            .color = color,
            .trapezoid_ratio = {
                [0] = trapezoid_ratio != nullptr ? trapezoid_ratio[i].z : 1.0f,
                [1] = trapezoid_ratio != nullptr ? trapezoid_ratio[i].w : 1.0f,
            },
        };
        MeshBuilder_AddVertex(builder, &vertex);
    }
}

static void M_AddObjectFace3(
    MESH_BUILDER *const builder, const OBJECT_MESH *const obj_mesh,
    const FACE3 *const face, const uint16_t flags)
{
    M_AddObjectVerts(
        builder, 3, obj_mesh, face->vertices, face->texture_idx,
        face->palette_idx, flags, nullptr);
    MeshBuilder_AddFace3(
        builder,
        !obj_mesh->disable_transparency_sort && (flags & VERT_FLAT_SHADED) == 0
            && Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

static void M_AddObjectFace4(
    MESH_BUILDER *const builder, const OBJECT_MESH *const obj_mesh,
    const FACE4 *const face, const uint16_t flags)
{
    M_AddObjectVerts(
        builder, 4, obj_mesh, face->vertices, face->texture_idx,
        face->palette_idx, flags, face->texture_zw);
    MeshBuilder_AddFace4(
        builder,
        !obj_mesh->disable_transparency_sort && (flags & VERT_FLAT_SHADED) == 0
            && Output_Textures_IsObjectTextureTransparent(face->texture_idx));
}

static int32_t *M_PrepareLightIndexMap(
    const OBJECT_MESH *const obj_mesh, const int32_t vertex_count)
{
    int32_t v = 0;
    int32_t *const light_idx_map = Memory_Alloc(sizeof(int32_t) * vertex_count);
#define L_SET(v, j) light_idx_map[v] = face->vertices[j];
    for (int32_t i = 0; i < obj_mesh->num_tex_face4s; i++) {
        const FACE4 *const face = &obj_mesh->tex_face4s[i];
        for (int32_t j = 0; j < 4; j++) {
            L_SET(v, j);
            v++;
        }
    }
    for (int32_t i = 0; i < obj_mesh->num_tex_face3s; i++) {
        const FACE3 *const face = &obj_mesh->tex_face3s[i];
        for (int32_t j = 0; j < 3; j++) {
            L_SET(v, j);
            v++;
        }
    }
    for (int32_t i = 0; i < obj_mesh->num_flat_face4s; i++) {
        const FACE4 *const face = &obj_mesh->flat_face4s[i];
        for (int32_t j = 0; j < 4; j++) {
            L_SET(v, j);
            v++;
        }
    }
    for (int32_t i = 0; i < obj_mesh->num_flat_face3s; i++) {
        const FACE3 *const face = &obj_mesh->flat_face3s[i];
        for (int32_t j = 0; j < 3; j++) {
            L_SET(v, j);
            v++;
        }
    }
#undef L_SET
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

        for (int32_t j = 0; j < obj_mesh->num_tex_face4s; j++) {
            M_AddObjectFace4(
                builder, obj_mesh, &obj_mesh->tex_face4s[j], flags);
        }
        for (int32_t j = 0; j < obj_mesh->num_tex_face3s; j++) {
            M_AddObjectFace3(
                builder, obj_mesh, &obj_mesh->tex_face3s[j], flags);
        }
        for (int32_t j = 0; j < obj_mesh->num_flat_face4s; j++) {
            M_AddObjectFace4(
                builder, obj_mesh, &obj_mesh->flat_face4s[j],
                flags | VERT_FLAT_SHADED);
        }
        for (int32_t j = 0; j < obj_mesh->num_flat_face3s; j++) {
            M_AddObjectFace3(
                builder, obj_mesh, &obj_mesh->flat_face3s[j],
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

static void M_UpdateShades(MESH_INSTANCE *const inst, void *const user_data)
{
    const OBJECT_MESH *const mesh = user_data;
    const M_PRIV *const p = &m_Priv;

    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    if (batch->mesh_batch == nullptr) {
        return;
    }
    OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(batch->mesh_batch->vertices);

    int32_t *const light_idx_map = batch->light_idx_map;

    if (!g_Config.rendering.enable_lighting) {
        for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
            vertices[i].shade = SHADE_NEUTRAL;
        }
        return;
    }

    const MATRIX *const matrix = &inst->matrix;
    int32_t ls_adder = inst->ls_adder;
    int32_t ls_divider = inst->ls_divider;
    XYZ_32 ls_vector_view = inst->ls_vector_view;

    if (mesh->num_lights <= 0) {
        for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
            const int32_t j = light_idx_map[i];
            int16_t shade = ls_adder + mesh->lighting.lights[j];
            CLAMP(shade, 0, SHADE_MAX);
            vertices[i].shade = shade;
        }
    } else if (ls_divider == 0) {
        int16_t shade = ls_adder;
        CLAMP(shade, 0, SHADE_MAX);
        for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
            vertices[i].shade = shade;
        }
    } else {
        // clang-format off
        const int32_t xv = (
            matrix->_00 * ls_vector_view.x +
            matrix->_10 * ls_vector_view.y +
            matrix->_20 * ls_vector_view.z
        ) / ls_divider;

        const int32_t yv = (
            matrix->_01 * ls_vector_view.x +
            matrix->_11 * ls_vector_view.y +
            matrix->_21 * ls_vector_view.z
        ) / ls_divider;

        const int32_t zv = (
            matrix->_02 * ls_vector_view.x +
            matrix->_12 * ls_vector_view.y +
            matrix->_22 * ls_vector_view.z
        ) / ls_divider;
        // clang-format on

        for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
            const int32_t j = light_idx_map[i];
            const XYZ_16 *const normal = &mesh->lighting.normals[j];
            int16_t shade = ls_adder
                + ((normal->x * xv + normal->y * yv + normal->z * zv) >> 16);
            CLAMP(shade, 0, SHADE_MAX);
            vertices[i].shade = shade;
        }
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

static void M_Stage(const OBJECT_MESH *const mesh, const bool skybox)
{
    M_PRIV *const p = &m_Priv;
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    if (batch->mesh_batch == nullptr) {
        return;
    }

    const MESH_INSTANCE inst = {
        .mesh = batch->mesh_batch,
        .matrix = *g_MatrixPtr,
        .tint = Output_GetTint(),
        .wibble = false,
        .water_effect = Output_GetWaterEffect(),
        .ls_adder = Output_GetLightAdder(),
        .ls_divider = Output_GetLightDivider(),
        .ls_vector_view = Output_GetLightVectorView(),
        .update_light_func = skybox ? M_UpdateShadesSkybox : M_UpdateShades,
        .update_light_func_data = (void *)mesh,
    };
    if (skybox) {
        MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_SKYBOX);
    } else {
        MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_MESHES);
        MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
    }
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
    M_Stage(mesh, true);
}

void OutputSource_Objects_StageObjectMesh(const OBJECT_MESH *const mesh)
{
    M_Stage(mesh, false);
}

const SCENE_SOURCE *OutputSource_Objects_GetSource(void)
{
    M_PRIV *const p = &m_Priv;
    return MeshBatcher_AsSource(p->batcher);
}
