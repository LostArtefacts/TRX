#pragma once

#include <trx/game/catalog/manager.h>

#include <stdint.h>

// Identify a sample by the number stored in the game's files.
typedef int32_t SAMPLE_SLOT;

enum {
    SFX_INVALID = -1,
};

// Identify the same sample across all four games.
typedef CATALOG_ID SAMPLE_ID;

enum {
#define X_CATALOG_ID(enum_value) enum_value,
#include <trx/game/catalog/samples.def>
#undef X_CATALOG_ID
};

SAMPLE_SLOT Sound_IDToSlot(SAMPLE_ID id);
SAMPLE_ID Sound_SlotToID(SAMPLE_SLOT slot);
