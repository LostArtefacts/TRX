#pragma once

#include "../lara/enum.h"
#include "../stats/types.h"

typedef struct {
    LEVEL_STATS stats;
    uint8_t small_medipacks;
    uint8_t large_medipacks;
    uint16_t pistol_ammo;
    uint16_t magnum_ammo;
    uint16_t uzi_ammo;
    uint16_t shotgun_ammo;
    LARA_GUN_STATE gun_status;
    LARA_GUN_TYPE equipped_gun_type;
#if TR_VERSION == 1
    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;
    uint8_t num_scions;
    int32_t lara_hitpoints;
#elif TR_VERSION == 2
    uint16_t m16_ammo;
    uint16_t grenade_ammo;
    uint16_t harpoon_ammo;
    uint8_t flares;
#endif

    union {
        uint16_t all;
        struct {
            uint16_t available : 1;
            uint16_t has_pistols : 1;
            uint16_t has_magnums : 1;
            uint16_t has_uzis : 1;
            uint16_t has_shotgun : 1;
#if TR_VERSION == 1
            uint16_t costume : 1;
            uint16_t pad : 10;
#elif TR_VERSION == 2
            uint16_t has_m16 : 1;
            uint16_t has_grenade : 1;
            uint16_t has_harpoon : 1;
            uint16_t pad : 8;
#endif
        };
    } flags;
} RESUME_INFO;
