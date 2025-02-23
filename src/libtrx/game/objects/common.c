#include "game/objects/common.h"

#include "debug.h"
#include "game/anims.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/matrix.h"
#include "game/output.h"

static OBJECT m_Objects[O_NUMBER_OF] = {};
static STATIC_OBJECT_3D m_StaticObjects3D[MAX_STATIC_OBJECTS] = {};
static STATIC_OBJECT_2D m_StaticObjects2D[MAX_STATIC_OBJECTS] = {};
static OBJECT_MESH **m_MeshPointers = nullptr;
static int32_t m_MeshCount = 0;

OBJECT *Object_Get(const GAME_OBJECT_ID obj_id)
{
    return &m_Objects[obj_id];
}

STATIC_OBJECT_3D *Object_Get3DStatic(const int32_t static_id)
{
    return &m_StaticObjects3D[static_id];
}

STATIC_OBJECT_2D *Object_Get2DStatic(const int32_t static_id)
{
    return &m_StaticObjects2D[static_id];
}

bool Object_IsType(const GAME_OBJECT_ID obj_id, const GAME_OBJECT_ID *test_arr)
{
    for (int32_t i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == obj_id) {
            return true;
        }
    }
    return false;
}

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->key_id == key_id) {
            return pair->value_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->value_id == value_id) {
            return pair->key_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

void Object_InitialiseMeshes(const int32_t mesh_count)
{
    m_MeshPointers =
        GameBuf_Alloc(sizeof(OBJECT_MESH *) * mesh_count, GBUF_MESH_POINTERS);
    m_MeshCount = 0;
}

void Object_StoreMesh(OBJECT_MESH *const mesh)
{
    m_MeshPointers[m_MeshCount] = mesh;
    m_MeshCount++;
}

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    return m_MeshPointers[index];
}

OBJECT_MESH *Object_FindMesh(const int32_t data_offset)
{
    for (int32_t i = 0; i < m_MeshCount; i++) {
        OBJECT_MESH *const mesh = m_MeshPointers[i];
        if (Object_GetMeshOffset(mesh) == data_offset) {
            return mesh;
        }
    }

    return nullptr;
}

int32_t Object_GetMeshOffset(const OBJECT_MESH *const mesh)
{
    return (int32_t)(intptr_t)mesh->priv;
}

void Object_SetMeshOffset(OBJECT_MESH *const mesh, const int32_t data_offset)
{
    mesh->priv = (void *)(intptr_t)data_offset;
}

void Object_SwapMesh(
    const GAME_OBJECT_ID object1_id, const GAME_OBJECT_ID object2_id,
    const int32_t mesh_num)
{
    const OBJECT *const obj1 = Object_Get(object1_id);
    const OBJECT *const obj2 = Object_Get(object2_id);

    OBJECT_MESH *const temp = m_MeshPointers[obj1->mesh_idx + mesh_num];
    m_MeshPointers[obj1->mesh_idx + mesh_num] =
        m_MeshPointers[obj2->mesh_idx + mesh_num];
    m_MeshPointers[obj2->mesh_idx + mesh_num] = temp;
}

ANIM *Object_GetAnim(const OBJECT *const obj, const int32_t anim_idx)
{
    return Anim_GetAnim(obj->anim_idx + anim_idx);
}

ANIM_BONE *Object_GetBone(const OBJECT *const obj, const int32_t bone_idx)
{
    return Anim_GetBone(obj->bone_idx + bone_idx);
}

void Object_DrawInterpolatedObject(
    const OBJECT *const obj, const uint32_t meshes,
    const int16_t *extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    ASSERT(frame1 != nullptr);
    const int32_t clip = Output_GetObjectBounds(&frame1->bounds);
    if (clip == 0) {
        return;
    }

    ASSERT(rate != 0);
    Matrix_Push();

    if (frac != 0) {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_InitInterpolate(frac, rate);
                Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
                Matrix_Rot16_ID(
                    frame1->mesh_rots[mesh_idx], frame2->mesh_rots[mesh_idx]);
            } else {
                const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
                if (bone->matrix_pop) {
                    Matrix_Pop_I();
                }
                if (bone->matrix_push) {
                    Matrix_Push_I();
                }

                Matrix_TranslateRel32_I(bone->pos);
                Matrix_Rot16_ID(
                    frame1->mesh_rots[mesh_idx], frame2->mesh_rots[mesh_idx]);
                if (extra_rotation != nullptr) {
                    if (bone->rot_y) {
                        Matrix_RotY_I(*extra_rotation++);
                    }
                    if (bone->rot_x) {
                        Matrix_RotX_I(*extra_rotation++);
                    }
                    if (bone->rot_z) {
                        Matrix_RotZ_I(*extra_rotation++);
                    }
                }
            }

            if (meshes & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, true);
            }
        }
    } else {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_TranslateRel16(frame1->offset);
                Matrix_Rot16(frame1->mesh_rots[mesh_idx]);
            } else {
                const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
                if (bone->matrix_pop) {
                    Matrix_Pop();
                }
                if (bone->matrix_push) {
                    Matrix_Push();
                }

                Matrix_TranslateRel32(bone->pos);
                Matrix_Rot16(frame1->mesh_rots[mesh_idx]);
                if (extra_rotation != nullptr) {
                    if (bone->rot_y) {
                        Matrix_RotY(*extra_rotation++);
                    }
                    if (bone->rot_x) {
                        Matrix_RotX(*extra_rotation++);
                    }
                    if (bone->rot_z) {
                        Matrix_RotZ(*extra_rotation++);
                    }
                }
            }

            if (meshes & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, false);
            }
        }
    }

    Matrix_Pop();
}
