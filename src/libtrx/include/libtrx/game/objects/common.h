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
OBJECT *Object_Get(GAME_OBJECT_ID object_id);

// Filesystem helper functions ================================================
// In general, in TRX the objects are identified in three ways:
// - GAME_OBJECT_ID - a TRX internal ID, essentially an arbitrary linear enum
// - game ID - an ID used by the original game in its level files
// - UUID (aka GUID) - a 128-bit globally unique identifier that allows us to
//   clearly and immutably identify the objects across all game engines.
//   Preferred where possible over the OG IDs (in filesystem applications).
// For all purposes outside of filesystem interaction and exposing the objects
// to the world, the code should just use the GAME_OBJECT_ID enum.

// Retrieve an object by its game ID. Returns nullptr if not found.
OBJECT *Object_GetByGameID(int32_t game_id);
// Retrieve an object by its UUID. Returns nullptr if not found.
OBJECT *Object_GetByUUID(const UUID uuid);

// Convert a game ID to GAME_OBJECT_ID.
GAME_OBJECT_ID Object_UnmapGameID(int32_t game_id);
// Convert a UUID to GAME_OBJECT_ID.
GAME_OBJECT_ID Object_UnmapUUID(const UUID uuid);

// Convert a GAME_OBJECT_ID to a game ID (opposite of Object_UnmapGameID).
int32_t Object_MakeGameID(GAME_OBJECT_ID object_id);

// Other functions ============================================================

STATIC_OBJECT_3D *Object_Get3DStatic(int32_t static_id);
STATIC_OBJECT_2D *Object_Get2DStatic(int32_t static_id);

bool Object_IsType(GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);

void Object_InitialiseMeshes(int32_t mesh_count);
void Object_StoreMesh(OBJECT_MESH *mesh);

int32_t Object_GetMeshCount(void);
OBJECT_MESH *Object_FindMesh(int32_t data_offset);
int32_t Object_GetMeshIndex(const OBJECT_MESH *mesh);
int32_t Object_GetMeshOffset(const OBJECT_MESH *mesh);
void Object_SetMeshOffset(OBJECT_MESH *mesh, int32_t data_offset);

OBJECT_MESH *Object_GetMesh(int32_t index);
void Object_SwapMesh(
    GAME_OBJECT_ID object1_id, GAME_OBJECT_ID object2_id, int32_t mesh_num);

ANIM *Object_GetAnim(const OBJECT *obj, int32_t anim_idx);
ANIM_BONE *Object_GetBone(const OBJECT *obj, int32_t bone_idx);

// Given a key or puzzle object, find a matching receptacle item number to
// establish the interaction target. Takes into account current Lara's
// position.
int16_t Object_FindReceptacle(GAME_OBJECT_ID obj_id);

// Given a receptacle object ID, find a matching key/puzzle object ID.
GAME_OBJECT_ID Object_FindReceptacleKey(const GAME_OBJECT_ID receptacle_obj_id);

extern void Object_SetReflective(GAME_OBJECT_ID obj_id, bool enabled);

#define REGISTER_OBJECT(object_id, setup_func_)                                \
    __attribute__((constructor)) static void M_RegisterObject##object_id(void) \
    {                                                                          \
        Object_Get(object_id)->setup_func = setup_func_;                       \
    }
