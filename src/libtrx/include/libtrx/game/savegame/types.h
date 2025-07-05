#pragma once

#include "../../filesystem.h"
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
    int32_t lara_hitpoints;
    LARA_GUN_STATE gun_status;
    LARA_GUN_TYPE equipped_gun_type;
    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;
#if TR_VERSION == 1
    uint8_t num_scions;
#elif TR_VERSION == 2
    uint16_t m16_ammo;
    uint16_t grenade_ammo;
    uint16_t harpoon_ammo;
    uint16_t flares;
#endif

    struct {
        bool available;
        bool costume;
        bool has_pistols;
        bool has_magnums;
        bool has_uzis;
        bool has_shotgun;
#if TR_VERSION >= 2
        bool has_m16;
        bool has_grenade;
        bool has_harpoon;
#endif
    } flags;
} RESUME_INFO;

typedef struct {
    SAVEGAME_FORMAT format;
    char *full_path;
    int32_t counter;
    int32_t level_num;
    char *level_title;
    int16_t initial_version;
    struct {
        bool restart;
        bool select_level;
    } features;
} SAVEGAME_INFO;

typedef struct {
    bool allow_load;
    bool allow_save;
    SAVEGAME_FORMAT format;
    const char *(*get_save_file_pattern_func)(void);
    bool (*fill_info_func)(MYFILE *fp, SAVEGAME_INFO *info);
    bool (*load_from_file_func)(MYFILE *fp);
    bool (*load_only_resume_info_func)(MYFILE *fp);
    void (*save_to_file_func)(MYFILE *fp, SAVEGAME_INFO *savegame_info);
    bool (*update_death_counters_func)(
        MYFILE *fp, int32_t level_num, int32_t death_count);
} SAVEGAME_STRATEGY;
