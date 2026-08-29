#pragma once

#include <trx/game/catalog/manager.h>

#include <stdint.h>

#define O_FIRST 0

// Identify an object by its number in a game's files.
typedef int32_t OBJECT_SLOT;

// Identify the same object across all four games.
typedef CATALOG_ID OBJECT_ID;

enum {
    NO_OBJECT = -1,
#define X_CATALOG_ID(enum_value) enum_value,
#include <trx/game/catalog/objects.def>
#undef X_CATALOG_ID
};
