#pragma once

#include <stdint.h>

typedef struct STATS_COMMON {
    uint32_t timer;
    uint32_t kill_count;
    uint32_t ammo_used;
    uint32_t ammo_hits;
    uint32_t distance_travelled;
    uint16_t max_secret_count;
    uint16_t all_secrets_mask; // bit mask containing valid secret slots
    double medipacks_used;
    uint16_t secret_count;
#if TR_VERSION == 1
    uint16_t pickup_count;
    uint32_t max_kill_count;
    uint16_t max_pickup_count;
    int32_t death_count;
#endif
} STATS_COMMON;

typedef struct {
    struct STATS_COMMON;
    uint16_t secret_flags;
} LEVEL_STATS;

typedef struct {
    struct STATS_COMMON;
} FINAL_STATS;
