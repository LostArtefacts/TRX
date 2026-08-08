#pragma once

#include <trx/core/handle.h>
#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/items/types.h>
#include <trx/game/objects/property.h>

void Item_Reset(void);
void Item_InitialiseItems(int32_t num_items);

// Resolve a slot index to its item, unchecked. The lookup everything uses.
ITEM *Item_Get(int16_t num);
// The item's slot, or NO_ITEM for one the pool does not hold.
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

// The starting hit points an object gives its items, declared once for every
// object that has any. Moving the ceiling carries the current hit points with
// it, so a level states the maximum alone and an item that has already been
// hurt stays hurt by as much.
static inline void Item_Property_SetMaxHitPoints(
    ITEM *item, const TRX_VALUE *value)
{
    item->hit_points += value->as_int - item->max_hit_points;
    item->max_hit_points = value->as_int;
    CLAMP(item->hit_points, 0, item->max_hit_points);
}

#define ITEM_PROPERTY_MAX_HIT_POINTS(value_)                                   \
    OBJECT_PROPERTY_ITEM(                                                      \
        max_hit_points, value_, nullptr, Item_Property_SetMaxHitPoints,        \
        "Maximum hit points.")

void Item_Control(void);

// Begin fading a body out, if the rules call for bodies to fade at all.
void Item_StartFade(ITEM *item);

// Whether a body is still on its way out. The item is destroyed once the fade
// runs out, so anything that reclaims a dead item's slot has to wait for this.
bool Item_IsFading(const ITEM *item);

// Remove the item from the game; any other handle to it becomes stale. Fires
// on_destroy during live play, while the item can still be read from the
// handler.
void Item_Destroy(int16_t item_num);

// Three ways to take an item out of view, chosen by whether it comes back.
// Item_SetVisible(false) is the reversible hide: a trigger can restore it, so
// it stays linked in the room and keeps its place, only unseen, unhittable, and
// untargetable. Item_DetachFromRoom removes it from the world as a positional
// object while its slot lives on (carried away, or consumed into another form).
// Item_Destroy is terminal and frees the slot.
//
// Unlink the item from the room item chain that collision and
// Item_FindTypeInRoom walk, and from the room draw queues. Fires on_leave_world
// during live play if it was present.
void Item_DetachFromRoom(int16_t item_num);

// Item_AddSimulated is the bare half of Item_Activate below: it only puts the
// slot on the simulation list. Item_Activate adds the trigger semantics on top
// - creature AI, ambush reveal, and for a plain item forcing is_visible on and
// is_finished off. Object code driving its own or a spawned item's simulation
// calls Item_AddSimulated when those are already where they belong; stand-ins
// for a trigger (floordata, scripts) call Item_Activate.
//
// Put the item on the simulation list so its control routine runs. A
// control-less item cannot simulate and is left off. Fires on_enter_sim during
// live play when it goes on.
void Item_AddSimulated(int16_t item_num);

// Take the item off the simulation list, however it got there. Fires
// on_leave_sim during live play if it was simulated.
void Item_RemoveSimulated(int16_t item_num);

// Bring an item to life the way a trigger does: start its control routine, and
// enable a creature's AI so it does more than stand there. An object with a
// custom activation runs that instead. An already-active item is left alone.
//
// `force` only matters once all the AI slots are taken, and only for a creature
// lying in wait. A trigger passes false and leaves it hidden until a slot frees
// on its own, which is what the games do. Something that asked for this
// creature by name passes true and takes a slot off whichever creature is
// furthest away. Fires on_activate during live play when it acts.
void Item_Activate(int16_t item_num, bool force);

// Stop an item: take it off the simulation list and take a creature's AI away.
// It stays where it is and keeps its hit points; it stops running. A trigger
// can still bring it back, which is what separates this from Item_Destroy.
// Fires on_deactivate during live play if it was running.
void Item_Deactivate(int16_t item_num);

// Return a recycled slot to play in a room, re-linking and activating it. The
// caller resets the slot's object state -- position, animation, hit points,
// creature data -- first.
void Item_Respawn(int16_t item_num, int16_t room_num);

// Set is_visible, the drawn-and-in-the-world axis. The single writer of its
// runtime transitions: Item_Initialise and the savegame reader seed the field
// directly, everything else routes here. A transition fires the on_show or
// on_hide event during live play; re-setting the current value is a no-op, so
// the per-frame control routines do not repeat it.
void Item_SetVisible(ITEM *item, bool value);

// Set is_finished, the object-local "my run is over" marker. As with
// Item_SetVisible, the single writer of its runtime transitions; the seed
// comes from Item_Initialise and the savegame reader. Becoming finished during
// live play fires the on_finish event; re-setting the current value is a no-op,
// which the per-frame control routines that mark themselves finished every
// frame rely on.
//
// Not the same as item->trigger.spent. spent is the trigger system's one-shot
// latch: it makes Item_Trigger return early for this item, so no further
// trigger reaches the object, and it is what IF_ONE_SHOT saves. is_finished is
// the object's own phase; Item_Trigger does not read it, so it does not by
// itself stop a re-trigger. Set spent to consume a one-shot trigger; set
// is_finished to record that the object's run has played out.
void Item_SetFinished(ITEM *item, bool value);

void Item_UpdateRoom(int16_t item_num, int16_t room_num);
void Item_InitialiseDrawQueues(void);

int32_t Item_GlobalReplace(OBJECT_ID src_obj_id, OBJECT_ID dst_obj_id);

// Set the name of the item, storing a copy of the provided string.
// Returns false if the name is already used by another item.
bool Item_SetName(int16_t item_num, const char *name);

// Retrieve an item by its name, or nullptr if not found
ITEM *Item_GetByName(const char *name);
