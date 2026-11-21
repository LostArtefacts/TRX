#pragma once

#include <trx/game/objects/types.h>

#include <stdint.h>

typedef enum {
    PICKUP_MODE_NORMAL,
    PICKUP_MODE_PLINTH_LOW,
    PICKUP_MODE_PLINTH_HIGH,
    PICKUP_MODE_HIDDEN,
    PICKUP_MODE_CROWBAR,
    PICKUP_MODE_SARCOPHAGUS,
    PICKUP_MODE_PLINTH_SCION,
    PICKUP_MODE_NUMBER_OF,
} PICKUP_MODE;

// Set up an object with pickup behaviour.
void Pickup_Setup(OBJECT *obj);
bool Pickup_Trigger(int16_t item_num);
OBJECT_BOUNDS Pickup_Bounds(const ITEM *item);
void Pickup_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
uint32_t Pickup_GetSecretMask(const ITEM *item);
// Lets off the twinkle of a pickup aid over the item at random intervals,
// while Lara is in range. The timer holds the frames left until the next one.
void Pickup_ControlAids(const ITEM *item, int32_t *timer);
int16_t Pickup_FindNearbyCrowbarPryPickup(void);
void Pickup_Collect(GAME_VECTOR pos, PICKUP_MODE mode);
