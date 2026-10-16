#pragma once

#include <trx/game/ui/menu.h>

typedef enum {
    RT_MAIN = 0,
    RT_OPTION = 1,
    RT_KEYS = 2,
    RT_NUMBER_OF,
} RING_TYPE;

typedef enum {
    RNG_OPENING,
    RNG_OPEN,
    RNG_CLOSING,
    RNG_MAIN2OPTION,
    RNG_MAIN2KEYS,
    RNG_KEYS2MAIN,
    RNG_OPTION2MAIN,
    RNG_SELECTING,
    RNG_SELECTED,
    RNG_DESELECTING,
    RNG_DESELECT,
    RNG_CLOSING_ITEM,
    RNG_EXITING_INVENTORY,
    RNG_DONE,
    RNG_FADING_OUT,
} RING_STATUS;
