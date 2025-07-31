#include "game/output/sources/objects.h"

#include "game/output.h"
#include "game/output/scene_compositor.h"
#include "game/output/utils.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

typedef struct {
    OUTPUT_MESH *mesh_batch;
    int32_t *light_idx_map;
} M_MESH;

typedef struct {
    MESH_BATCHER *batcher;
    size_t mesh_count;
    M_MESH *meshes;
} M_PRIV;

static M_PRIV m_Priv = {};

static int32_t *M_PrepareLightIndexMap(
    const OBJECT_MESH *obj_mesh, int32_t vertex_count);
static void M_PrepareMeshes(M_PRIV *p);
static void M_FreeMeshes(M_PRIV *p);
static void M_UpdateShades(MESH_INSTANCE *inst, void *user_data);
static void M_Stage(const OBJECT_MESH *mesh, bool background);

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
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const obj_mesh = Object_GetMesh(i);
        M_MESH *const new_batch = &p->meshes[i];
        OUTPUT_MESH *const batch = Output_Mesh_Create();

        uint16_t flags = 0;
        if (obj_mesh->enable_reflections) {
            flags |= VERT_REFLECTIVE;
        }
        if (obj_mesh->disable_lighting) {
            flags |= VERT_NO_LIGHTING;
        }

        for (int32_t j = 0; j < obj_mesh->num_tex_face4s; j++) {
            Output_Mesh_AddObjectFace4(
                batch, obj_mesh, &obj_mesh->tex_face4s[j], flags);
        }
        for (int32_t j = 0; j < obj_mesh->num_tex_face3s; j++) {
            Output_Mesh_AddObjectFace3(
                batch, obj_mesh, &obj_mesh->tex_face3s[j], flags);
        }
        for (int32_t j = 0; j < obj_mesh->num_flat_face4s; j++) {
            Output_Mesh_AddObjectFace4(
                batch, obj_mesh, &obj_mesh->flat_face4s[j],
                flags | VERT_FLAT_SHADED);
        }
        for (int32_t j = 0; j < obj_mesh->num_flat_face3s; j++) {
            Output_Mesh_AddObjectFace3(
                batch, obj_mesh, &obj_mesh->flat_face3s[j],
                flags | VERT_FLAT_SHADED);
        }
        Output_Mesh_Seal(batch);
        MeshBatcher_AddMesh(p->batcher, batch);

        new_batch->mesh_batch = batch;
        new_batch->light_idx_map =
            M_PrepareLightIndexMap(obj_mesh, batch->vertices->count);
    }
}

static void M_FreeMeshes(M_PRIV *const p)
{
    if (p->meshes != nullptr) {
        for (int32_t i = 0; i < (int32_t)p->mesh_count; i++) {
            MeshBatcher_RemoveMesh(p->batcher, p->meshes[i].mesh_batch);
            Output_Mesh_Destroy(p->meshes[i].mesh_batch);
            Memory_FreePointer(&p->meshes[i].light_idx_map);
        }
        Memory_FreePointer(&p->meshes);
    }
}

static void M_UpdateShades(MESH_INSTANCE *const inst, void *const user_data)
{
    const OBJECT_MESH *const mesh = user_data;
    M_PRIV *const p = &m_Priv;
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
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
    if (mesh->disable_lighting) {
        flags |= VERT_NO_LIGHTING;
    }
    OUTPUT_MESH_VERTEX *const vertices =
        Vector_GetData(batch->mesh_batch->vertices);
    for (int32_t i = 0; i < batch->mesh_batch->vertices->count; i++) {
        vertices[i].flags &= ~mask;
        vertices[i].flags |= flags;
    }
}

static void M_Stage(const OBJECT_MESH *const mesh, const bool background)
{
    M_PRIV *const p = &m_Priv;
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];

    const MESH_INSTANCE inst = {
        .mesh = batch->mesh_batch,
        .matrix = *g_MatrixPtr,
        .tint = Output_GetTint(),
        .wibble = false,
        .water_effect = Output_GetWaterEffect(),
        .ls_adder = Output_GetLightAdder(),
        .ls_divider = Output_GetLightDivider(),
        .ls_vector_view = Output_GetLightVectorView(),
        .update_light_func = M_UpdateShades,
        .update_light_func_data = (void *)mesh,
    };
    MeshBatcher_Stage(
        p->batcher, &inst,
        background ? SCENE_PASS_BACKGROUND : SCENE_PASS_MESHES);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
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
    const OBJECT_MESH *const mesh_1, const OBJECT_MESH *const mesh_2)
{
    M_PRIV *const p = &m_Priv;
    if (p->meshes == nullptr) {
        return;
    }

    const int16_t mesh_num_1 = Object_GetMeshIndex(mesh_1);
    const int16_t mesh_num_2 = Object_GetMeshIndex(mesh_2);
    SWAP2(p->meshes[mesh_num_1], p->meshes[mesh_num_2]);
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh_1);
    OutputSource_Objects_ObserveObjectMeshUpdate(mesh_2);
}

void OutputSource_Objects_ObserveObjectMeshUpdate(const OBJECT_MESH *const mesh)
{
    M_PRIV *const p = &m_Priv;
    if (p->meshes == nullptr) {
        return;
    }
    M_MESH *const batch = &p->meshes[Object_GetMeshIndex(mesh)];
    M_UpdateFlags(mesh, batch);
    MeshBatcher_UpdateMeshGeometry(p->batcher, batch->mesh_batch);
}

void OutputSource_Objects_StageSkyboxMesh(const OBJECT_MESH *const mesh)
{
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
