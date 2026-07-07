#pragma once

#include <trx/game/objects/types.h>

#include <stdint.h>

typedef enum {
    PICKUP_MODE_NORMAL,
    PICKUP_MODE_PLINTH_LOW,
    PICKUP_MODE_PLINTH_HIGH,
    PICKUP_MODE_NUMBER_OF,
} PICKUP_MODE;

bool Pickup_Trigger(int16_t item_num);
const OBJECT_BOUNDS *Pickup_Bounds(void);
void Pickup_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
uint32_t Pickup_GetSecretMask(const ITEM *item);
