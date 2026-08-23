#pragma once

#include <trx/core/filesystem.h>
#include <trx/game/inventory.h>
#include <trx/game/lara/enum.h>
#include <trx/game/objects/ids.h>
#include <trx/game/savegame/enum.h>
#include <trx/game/stats/types.h>

typedef struct {
    OBJECT_ID object_id;
    const char *key;
} SAVEGAME_INVENTORY_ENTRY;

typedef struct {
    OBJECT_ID object_id;
    const char *key;
    bool required;
} SAVEGAME_RESUME_ITEM;

typedef struct {
    int32_t lara_hitpoints;
    LARA_GUN_STATE gun_status;
    LARA_GUN_TYPE equipped_gun_type;
    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;

    // What Lara arrives in the level carrying.
    INVENTORY_STATE inv;

    struct {
        bool available;
        bool costume;
    } flags;

    bool level_completed;
    int32_t prev_level;
    bool hurt_allies;
    bool burning;

    LEVEL_STATS stats;
} RESUME_INFO;

typedef enum {
    SAVEGAME_SLOT_POOL_NORMAL = 0,
    SAVEGAME_SLOT_POOL_QUICK = 1,
    SAVEGAME_SLOT_POOL_NUMBER_OF,
} SAVEGAME_SLOT_POOL;

typedef struct {
    SAVEGAME_SLOT_POOL pool;
    int32_t index;
} SAVEGAME_SLOT_REF;

typedef struct {
    char *full_path;
    int32_t counter;
    int32_t level_num;
    char *level_title;
    int16_t initial_version;
    bool is_quick;
    struct {
        bool restart;
        bool select_level;
    } features;
} SAVEGAME_INFO;
