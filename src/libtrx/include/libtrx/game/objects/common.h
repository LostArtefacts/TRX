#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "ids.h"
#include "types.h"

OBJECT *Object_Get(GAME_OBJECT_ID obj_id);
STATIC_OBJECT_3D *Object_Get3DStatic(int32_t static_id);
STATIC_OBJECT_2D *Object_Get2DStatic(int32_t static_id);

bool Object_IsType(GAME_OBJECT_ID obj_id, const GAME_OBJECT_ID *test_arr);

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);

void Object_InitialiseMeshes(int32_t mesh_count);
void Object_StoreMesh(OBJECT_MESH *mesh);

OBJECT_MESH *Object_FindMesh(int32_t data_offset);
int32_t Object_GetMeshOffset(const OBJECT_MESH *mesh);
void Object_SetMeshOffset(OBJECT_MESH *mesh, int32_t data_offset);

OBJECT_MESH *Object_GetMesh(int32_t index);
void Object_SwapMesh(
    GAME_OBJECT_ID object1_id, GAME_OBJECT_ID object2_id, int32_t mesh_num);

ANIM *Object_GetAnim(const OBJECT *obj, int32_t anim_idx);
ANIM_BONE *Object_GetBone(const OBJECT *obj, int32_t bone_idx);

extern void Object_DrawMesh(int32_t mesh_idx, int32_t clip, bool interpolated);

void Object_DrawInterpolatedObject(
    const OBJECT *obj, uint32_t meshes, const int16_t *extra_rotation,
    const ANIM_FRAME *frame1, const ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);

#define REGISTER_OBJECT(object_id, setup_func_)                                \
    __attribute__((constructor)) static void M_RegisterObject##object_id(void) \
    {                                                                          \
        Object_Get(object_id)->setup_func = setup_func_;                       \
    }
