#pragma once

#include <trx/core/math.h>
#include <trx/game/anims.h>
#include <trx/game/collision.h>
#include <trx/game/items.h>
#include <trx/game/objects/draw.h>
#include <trx/game/objects/ids.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/types.h>

void Object_Reset(void);

// Retrieve an object by its TRX internal index. Trying to retrieve an invalid
// object is a fatal error.
OBJECT *Object_Get(OBJECT_ID object_id);

// Retrieve an object by its TRX internal index. Trying to retrieve an invalid
// object returns nullptr.
OBJECT *Object_TryGet(OBJECT_ID object_id);

// Takes an object slot the catalog does not name, for something the engine
// builds rather than a level supplies. The id it answers with lies past the
// catalog, so a level cannot inject into it, scripts cannot name it, and it
// carries no name of its own - keep it away from items, whose object id a
// savegame has to be able to read back. It lasts until the level is unloaded.
OBJECT_ID Object_Mint(void);

int32_t Object_GetCount(void);

// Copies meshes and animations from a source object to another object. Reports
// failure if the level has no content for the source object.
RESULT Object_BorrowContent(OBJECT_ID object_id, OBJECT_ID source_id);

// Retrieve an object by its slot. Returns nullptr if not found.
OBJECT *Object_GetBySlot(OBJECT_SLOT slot);

// Keep an object slot the catalog has no entry for, under the number the level
// gives it. TR4 wads park geometry that only a cutscene ever draws in whichever
// slots they left free, so the meshes have to survive a load that nothing in
// the catalog names. Dropped with the level.
void Object_StoreUncatalogedSlot(OBJECT_SLOT slot, const OBJECT *obj);

// Retrieve a slot kept that way, or nullptr when the level carries none under
// that number.
const OBJECT *Object_GetUncatalogedSlot(OBJECT_SLOT slot);

// Convert a slot to the identity holding it.
OBJECT_ID Object_SlotToID(OBJECT_SLOT slot);

// Convert an identity to the slot it is bound to.
OBJECT_SLOT Object_IDToSlot(OBJECT_ID id);

// Other functions ============================================================

void Object_InitialiseStaticObjects3D(int32_t count);
void Object_InitialiseStaticObjects2D(int32_t count);
int32_t Object_GetStaticObjects3DCount(void);
int32_t Object_GetStaticObjects2DCount(void);

bool Object_IsValidStatid3D(int32_t static_id);
STATIC_OBJECT_3D *Object_Get3DStatic(int32_t static_id);
STATIC_OBJECT_2D *Object_Get2DStatic(int32_t static_id);

void Object_InitialiseMeshes(int32_t mesh_count);
void Object_StoreMesh(OBJECT_MESH *mesh);

int32_t Object_GetMeshCount(void);
OBJECT_MESH *Object_FindMesh(int32_t data_offset);
int32_t Object_GetMeshIndex(const OBJECT_MESH *mesh);
int32_t Object_GetMeshOffset(const OBJECT_MESH *mesh);
void Object_SetMeshOffset(OBJECT_MESH *mesh, int32_t data_offset);

OBJECT_MESH *Object_GetMesh(int32_t index);
int32_t Object_GetItemMeshIndex(const ITEM *item, int32_t mesh_idx);
void Object_SwapMesh(
    OBJECT_ID object1_id, OBJECT_ID object2_id, int32_t mesh_num);
void Object_SwapAllMeshes(OBJECT_ID object1_id, OBJECT_ID object2_id);
void Object_SwapMeshEx(
    OBJECT_ID object1_id, OBJECT_ID object2_id, int32_t mesh_num1,
    int32_t mesh_num2);
void Object_SwapSprite(OBJECT_ID object1_id, OBJECT_ID object2_id);

ANIM *Object_GetAnim(const OBJECT *obj, int32_t anim_idx);
ANIM_BONE *Object_GetBone(const OBJECT *obj, int32_t bone_idx);
ANIM_BONE *Object_TryGetBone(const OBJECT *obj, int32_t bone_idx);

// Given a key or puzzle object, find a matching receptacle item number to
// establish the interaction target. Takes into account current Lara's
// position.
int16_t Object_FindReceptacle(OBJECT_ID obj_id);

// Given a receptacle object ID, find a matching key/puzzle object ID.
OBJECT_ID Object_FindReceptacleKey(const OBJECT_ID receptacle_obj_id);

void Object_SetReflective(OBJECT_ID obj_id, bool enabled);
void Object_SetMeshReflective(OBJECT_ID obj_id, int32_t mesh_idx, bool enabled);

// Marks every face of the object's meshes to draw at half opacity, like the
// PSX half blend mode. Face scene passes are baked at level load; when
// changing this mid-game, follow up with Output_RefreshObjectMeshes().
void Object_SetSemiTransparent(OBJECT_ID obj_id, bool enabled);

bool Object_CanInterpolate(const ITEM *item, int32_t frame_a, int32_t frame_b);

// Bind the setup routine of an object before catalogue setup.
void Object_Register(OBJECT_ID object_id, void (*setup_func)(OBJECT *obj));

#define REGISTER_OBJECT(object_id, setup_func_)                                \
    __attribute__((constructor)) static void M_RegisterObject##object_id(void) \
    {                                                                          \
        Object_Register(object_id, setup_func_);                               \
    }
