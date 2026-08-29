#pragma once

#include <trx/game/catalog/manager.h>

#include <stdint.h>

// Identify a music track by the number stored in a game's files.
typedef int32_t MUSIC_SLOT;

enum {
    MX_INACTIVE = -1,
};

// Identify the same music track across all four games.
typedef CATALOG_ID MUSIC_ID;

enum {
#define X_CATALOG_ID(enum_value) enum_value,
#include <trx/game/catalog/music.def>
#undef X_CATALOG_ID
};

MUSIC_SLOT Music_IDToSlot(MUSIC_ID id);
MUSIC_ID Music_SlotToID(MUSIC_SLOT slot);
