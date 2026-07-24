#pragma once

#include <trx/core/handle.h>
#include <trx/game/anims.h>
#include <trx/game/items/types.h>

void Item_Reset(void);
void Item_InitialiseItems(int32_t num_items);
ITEM *Item_Get(int16_t num);
int16_t Item_GetIndex(const ITEM *item);

// A handle to the item currently in the slot, and the item a handle still names
// or nullptr once its slot has been recycled or the level replaced. Scripts
// hold these across time; the generation is what tells one occupant from the
// next.
TRX_HANDLE Item_GetHandle(int16_t item_num);
ITEM *Item_FromHandle(TRX_HANDLE handle);

int32_t Item_GetLevelCount(void);
int32_t Item_GetTotalCount(void);

int16_t Item_GetNextActive(void);

int16_t Item_Create(void);
int16_t Item_CreateLevelItem(void);
int16_t Item_Spawn(const ITEM *item, OBJECT_ID obj_id);

void Item_Initialise(int16_t item_num);
void Item_Control(void);
void Item_Kill(int16_t item_num);
void Item_KillAllActive(void);
void Item_RemoveActive(int16_t item_num);

// Fire the on_trigger event: a trigger of any kind was aimed at the item, with
// its fundamentals. Item_Trigger calls this for every trigger it acts on; kept
// in the manager, next to the event stack, so the primitive stays clear of it.
void Item_NotifyTriggered(int16_t item_num, const ITEM_TRIGGER *trigger);
void Item_DetachFromRoom(int16_t item_num);
void Item_ClearKilled(void);
void Item_AddActive(int16_t item_num);

// Bring an item to life the way a trigger does: start its control routine, and
// enable a creature's AI so it does more than stand there. An object with a
// custom activation runs that instead. An already-active item is left alone.
//
// `force` only matters once all the AI slots are taken, and only for a creature
// lying in wait. A trigger passes false and leaves it hidden until a slot frees
// on its own, which is what the games do. Something that asked for this
// creature by name passes true and takes a slot off whichever creature is
// furthest away.
void Item_Activate(int16_t item_num, bool force);

// Stop an item: take it off the active list and take a creature's AI away. It
// stays where it is and keeps its hit points; it stops running. A trigger can
// still bring it back, which is what separates this from Item_Kill.
void Item_Deactivate(int16_t item_num);

void Item_UpdateRoom(int16_t item_num, int16_t room_num);

int32_t Item_GlobalReplace(OBJECT_ID src_obj_id, OBJECT_ID dst_obj_id);

// Set the name of the item, storing a copy of the provided string.
// Returns false if the name is already used by another item.
bool Item_SetName(int16_t item_num, const char *name);

// Retrieve an item by its name, or nullptr if not found
ITEM *Item_GetByName(const char *name);
