#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/objects/ids.h>
#include <trx/game/stats/const.h>

#include <stddef.h>
#include <stdint.h>

// One thing a level is counted on, which is one row of the statistics screen.
typedef enum {
    STATS_CAT_PICKUPS,
    STATS_CAT_KILLS,
    STATS_CAT_SECRETS,
    STATS_CAT_CRYSTALS,
    STATS_CAT_NUMBER_OF,
} STATS_CATEGORY_ID;

typedef struct {
    // What the level holds of each thing it is counted on, addressed by
    // STATS_CATEGORY_ID, less what the game flow declares unobtainable.
    uint32_t maxes[STATS_CAT_NUMBER_OF];
    // How the kills divide, which the statistics screen needs: the allies only
    // count against the player once she has hurt one.
    uint32_t max_kill_ally_count;
    uint32_t max_kill_non_ally_count;
    // How many of the secrets are items to pick up rather than floor triggers.
    uint32_t max_pickup_secret_count;
    uint32_t all_secrets_mask;

    struct {
        int32_t item_num;
        uint32_t secret_mask;
    } secret_item_masks[STATS_MAX_SECRETS];

    struct {
        bool taken;
        OBJECT_ID assigned_object_id;
        int32_t item_num;
    } secret_objects[STATS_MAX_SECRETS];
} LEVEL_MAX_STATS;

typedef struct STATS_COMMON {
    // What Lara has of each thing the level is counted on, addressed by
    // STATS_CATEGORY_ID.
    uint32_t counts[STATS_CAT_NUMBER_OF];
    // The rest is counted against nothing, so it stands on its own.
    uint32_t timer;
    uint32_t ammo_used;
    uint32_t ammo_hits;
    uint32_t distance_travelled;
    double medipacks_used;
    int32_t death_count;
} STATS_COMMON;

typedef struct {
    struct STATS_COMMON;
    uint16_t secret_flags;
} LEVEL_STATS;

typedef struct {
    STATS_COMMON stats;
    LEVEL_MAX_STATS max_stats;
} FINAL_STATS;

// What a level holds of one kind of thing, and how much of it Lara has. The
// count is read at the moment the view is filled; the level and the id say
// where it came from, so a writer can put a new one back.
typedef struct {
    const GF_LEVEL *level;
    STATS_CATEGORY_ID id;
    uint32_t count;
    // What counts towards completion, which is what the level holds less the
    // unobtainable part.
    uint32_t max;
    uint32_t raw;
    uint32_t unobtainable;
} STATS_CATEGORY;
