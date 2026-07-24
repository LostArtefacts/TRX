#pragma once

#include <trx/core/handle.h>
#include <trx/game/anims.h>
#include <trx/game/items/types.h>

void Item_Reset(void);
void Item_InitialiseItems(int32_t num_items);

// Resolve a slot index to its item, unchecked. The lookup everything uses.
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

int16_t Item_GetNextSimulated(void);

// Reserve an item slot. Item_Create takes the next free slot for a spawn made
// during play; that slot is recycled once the item is destroyed.
// Item_CreateLevelItem counts the slot into the level total instead, so it is
// never recycled - the fixed slots the level format loads into. Item_Spawn is
// Item_Create plus placement: a new object of obj_id at the given item's
// position, initialised.
int16_t Item_Create(void);
int16_t Item_CreateLevelItem(void);
int16_t Item_Spawn(const ITEM *item, OBJECT_ID obj_id);

void Item_Initialise(int16_t item_num);
void Item_Control(void);

// Remove the item from the game; any other handle to it becomes stale.
void Item_Destroy(int16_t item_num);

// Fire the on_trigger event: a trigger of any kind was aimed at the item, with
// its fundamentals. Item_Trigger calls this for every trigger it acts on; kept
// in the manager, next to the event stack, so the primitive stays clear of it.
void Item_NotifyTriggered(int16_t item_num, const ITEM_TRIGGER *trigger);

// Three ways to take an item out of view, chosen by whether it comes back.
// Setting is_visible false is the reversible hide: a trigger can restore it, so
// it stays linked in the room and keeps its place, only unseen, unhittable, and
// untargetable. Item_DetachFromRoom removes it from the world as a positional
// object while its slot lives on (carried away, or consumed into another form).
// Item_Destroy is terminal and frees the slot.
//
// Unlink the item from the room item chain that collision and
// Item_FindTypeInRoom walk, and from the room draw queues.
void Item_DetachFromRoom(int16_t item_num);

// Item_AddSimulated is the bare half of Item_Activate below: it only puts the
// slot on the simulation list. Item_Activate adds the trigger semantics on top
// - creature AI, ambush reveal, and for a plain item forcing is_visible on and
// is_finished off. Object code driving its own or a spawned item's simulation
// calls Item_AddSimulated when those are already where they belong; stand-ins
// for a trigger (floordata, scripts) call Item_Activate.
//
// Put the item on the simulation list so its control routine runs. A
// control-less item cannot simulate and is left off.
void Item_AddSimulated(int16_t item_num);

// Take the item off the simulation list, whatever put it there.
void Item_RemoveSimulated(int16_t item_num);

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

// Stop an item: take it off the simulation list and take a creature's AI away.
// It stays where it is and keeps its hit points; it stops running. A trigger
// can still bring it back, which is what separates this from Item_Destroy.
void Item_Deactivate(int16_t item_num);

// Return a recycled slot to play in a room, re-linking and activating it. The
// caller resets the slot's object state -- position, animation, hit points,
// creature data -- first.
void Item_Respawn(int16_t item_num, int16_t room_num);

void Item_UpdateRoom(int16_t item_num, int16_t room_num);

int32_t Item_GlobalReplace(OBJECT_ID src_obj_id, OBJECT_ID dst_obj_id);

// Set the name of the item, storing a copy of the provided string.
// Returns false if the name is already used by another item.
bool Item_SetName(int16_t item_num, const char *name);

// Retrieve an item by its name, or nullptr if not found
ITEM *Item_GetByName(const char *name);
