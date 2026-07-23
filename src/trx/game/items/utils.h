#pragma once

#include <trx/game/items/types.h>

#define ITEM_ADJUST_ROT(source, target, rot)                                   \
    do {                                                                       \
        if ((int16_t)(target - source) > rot) {                                \
            source += rot;                                                     \
        } else if ((int16_t)(target - source) < -rot) {                        \
            source -= rot;                                                     \
        } else {                                                               \
            source = target;                                                   \
        }                                                                      \
    } while (0)

bool Item_IsTriggerActiveRO(const ITEM *item);
bool Item_IsTriggerActive(ITEM *item);

// Every code bit set, which is what a lone trigger with nothing to wait for
// carries.
#define ITEM_TRIGGER_MASK_ALL 31

// The item's code bits, counted the way a level editor counts them: five bits,
// 1 to 31. An item runs its trigger only once every bit is set, so several
// triggers can be made to agree before anything happens.
int32_t Item_GetTriggerMask(const ITEM *item);
void Item_SetTriggerMask(ITEM *item, int32_t mask);

// Fire a trigger at the item, exactly as a floordata trigger does. The kind
// selects the flag operation: a forward trigger ORs in the mask and, once every
// code bit is set, starts the item running; a switch XORs it; an antitrigger
// (ITEM_TRIGGER_ANTI) clears the code bits and leaves the item on the active
// list to stand itself down, which is how a door animates shut rather than
// freezing half open. Room_Handle builds one of these from a floordata trigger;
// the console and scripts build them directly. Use Item_Deactivate to stop an
// item outright.
void Item_Trigger(int16_t item_num, const ITEM_TRIGGER *trigger);

bool Item_IsAlive(const ITEM *item);
bool Item_IsTargetable(const ITEM *item);
bool Item_CanTakeDamage(const ITEM *item);
bool Item_CanBeProjectileTarget(const ITEM *item);

void Item_TakeDamage(
    ITEM *item, int16_t damage, ITEM_DAMAGE_FLAGS flags, const ITEM *sender);

bool Item_IsMeshVisible(const ITEM *item, int32_t mesh_num);
void Item_SetMeshVisible(ITEM *item, int32_t mesh_num, bool visible);
void Item_SetMeshVisibleMask(ITEM *item, uint32_t mesh_mask, bool visible);
void Item_ResetMeshBits(ITEM *item);

// Mesh_bits: which meshes to affect.
// Damage:
// * Positive values - deal damage, enable body part explosions.
// * Negative values - deal damage, disable body part explosions.
// * Zero - don't deal any damage, disable body part explosions.
int32_t Item_Shatter(int16_t item_num, int32_t mesh_bits, int16_t damage);

bool Item_ShouldSpawnBlood(const ITEM *item);

int16_t Item_FindTypeInRoom(int16_t room_num, OBJECT_ID obj_id);
int16_t Item_FindTypeAtPos(int16_t room_num, XYZ_32 pos, OBJECT_ID obj_id);
