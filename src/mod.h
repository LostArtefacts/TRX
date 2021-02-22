#ifndef TOMB1MAIN_MOD_H
#define TOMB1MAIN_MOD_H

#include <stdint.h>

typedef enum {
    Tomb1M_BL_HCENTER = 1 << 0,
    Tomb1M_BL_HLEFT = 1 << 1,
    Tomb1M_BL_HRIGHT = 1 << 2,
    Tomb1M_BL_VTOP = 1 << 3,
    Tomb1M_BL_VBOTTOM = 1 << 4,
} Tomb1M_BAR_LOCATION;

typedef enum {
    Tomb1M_BSM_DEFAULT = 0,
    Tomb1M_BSM_FLASHING = 1,
    Tomb1M_BSM_ALWAYS = 2,
} Tomb1M_BAR_SHOW_MODE;

struct {
    int8_t disable_healing_between_levels;
    int8_t disable_medpacks;
    int8_t disable_magnums;
    int8_t disable_uzis;
    int8_t disable_shotgun;
    int8_t enable_red_healthbar;
    int8_t enable_enemy_healthbar;
    int8_t enable_enhanced_look;
    int8_t enable_enhanced_ui;
    int8_t enable_numeric_keys;
    int8_t enable_shotgun_flash;
    int8_t enable_cheats;
    int8_t healthbar_showing_mode;
    int8_t healthbar_location;
    int8_t airbar_location;
    int8_t enemy_healthbar_location;
    int8_t fix_end_of_level_freeze;
    int8_t fix_tihocan_secret_sound;
    int8_t fix_pyramid_secret_trigger;
    int8_t fix_hardcoded_secret_counts;
} T1MConfig;

#endif
