#pragma once

#include <trx/game/items/types.h>

typedef enum {
    SCALED_SPIKES_MODE_LOOPING,
    SCALED_SPIKES_MODE_EXTENDED,
    SCALED_SPIKES_MODE_ONE_SHOT,
    SCALED_SPIKES_NUMBER_OF,
} SCALED_SPIKES_MODE;

bool ScaledSpikes_TestCollision(const ITEM *item);
