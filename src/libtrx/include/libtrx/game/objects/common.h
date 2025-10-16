#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "draw.h"
#include "ids.h"
#include "types.h"

void Object_Reset(void);

// Retrieve an object by its TRX internal index. Trying to retrieve an invalid
// object is a fatal error.
OBJECT *Object_Get(OBJECT_ID object_id);

// Retrieve an object by its TRX internal index. Trying to retrieve an invalid
// object returns nullptr.
OBJECT *Object_TryGet(OBJECT_ID object_id);

// Retrieve an object by its game ID. Returns nullptr if not found.
OBJECT *Object_GetByGameID(int32_t game_id);

// Convert a game ID to OBJECT_ID.
OBJECT_ID Object_FromGameID(int32_t game_id);

// Convert a OBJECT_ID to a game ID (opposite of Object_FromGameID).
int32_t Object_ToGameID(OBJECT_ID object_id);

// Other functions ============================================================

STATIC_OBJECT_3D *Object_Get3DStatic(int32_t static_id);
STATIC_OBJECT_2D *Object_Get2DStatic(int32_t static_id);

bool Object_IsType(OBJECT_ID object_id, const OBJECT_ID *test_arr);

OBJECT_ID Object_GetCognate(OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

OBJECT_ID Object_GetCognateInverse(
    OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);

void Object_InitialiseMeshes(int32_t mesh_count);
void Object_StoreMesh(OBJECT_MESH *mesh);

int32_t Object_GetMeshCount(void);
OBJECT_MESH *Object_FindMesh(int32_t data_offset);
int32_t Object_GetMeshIndex(const OBJECT_MESH *mesh);
int32_t Object_GetMeshOffset(const OBJECT_MESH *mesh);
void Object_SetMeshOffset(OBJECT_MESH *mesh, int32_t data_offset);

OBJECT_MESH *Object_GetMesh(int32_t index);
void Object_SwapMesh(
    OBJECT_ID object1_id, OBJECT_ID object2_id, int32_t mesh_num);

ANIM *Object_GetAnim(const OBJECT *obj, int32_t anim_idx);
ANIM_BONE *Object_GetBone(const OBJECT *obj, int32_t bone_idx);

// Given a key or puzzle object, find a matching receptacle item number to
// establish the interaction target. Takes into account current Lara's
// position.
int16_t Object_FindReceptacle(OBJECT_ID obj_id);

// Given a receptacle object ID, find a matching key/puzzle object ID.
OBJECT_ID Object_FindReceptacleKey(const OBJECT_ID receptacle_obj_id);

extern void Object_SetReflective(OBJECT_ID obj_id, bool enabled);

bool Object_CanInterpolate(const ITEM *item, int32_t frame_a, int32_t frame_b);

#define REGISTER_OBJECT(object_id, setup_func_)                                \
    __attribute__((constructor)) static void M_RegisterObject##object_id(void) \
    {                                                                          \
        Object_Get(object_id)->setup_func = setup_func_;                       \
    }
