#ifndef TR1MAIN_MOD_H
#define TR1MAIN_MOD_H

#include "util.h"

typedef enum {
    TRM1_BAR_LARA_HEALTH = 0,
    TRM1_BAR_LARA_AIR = 1,
    TRM1_BAR_ENEMY_HEALTH = 2,
    TRM1_BAR_NUMBER = 3,
} TR1M_BAR;

struct {
    int disable_healing_between_levels;
    int disable_medpacks;
    int enable_red_healthbar;
    int enable_enemy_healthbar;
    int fix_end_of_level_freeze;
} TR1MConfig;

struct {
    int stored_lara_health;
} TR1MData;

int TR1MGetOverlayScale(int base);
void TR1MRenderBar(int value, int value_max, int bar_type);

#endif
