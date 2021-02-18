#ifndef TOMB1MAIN_MOD_H
#define TOMB1MAIN_MOD_H

#include "util.h"

typedef enum {
    Tomb1M_BAR_LARA_HEALTH = 0,
    Tomb1M_BAR_LARA_AIR = 1,
    Tomb1M_BAR_ENEMY_HEALTH = 2,
    Tomb1M_BAR_NUMBER = 3,
} Tomb1M_BAR;

typedef enum {
    Tomb1M_BSM_DEFAULT = 0,
    Tomb1M_BSM_FLASHING = 1,
    Tomb1M_BSM_ALWAYS = 2,
} Tomb1M_BAR_SHOW_MODE;

struct {
    int disable_healing_between_levels;
    int disable_medpacks;
    int disable_magnums;
    int disable_uzis;
    int disable_shotgun;
    int enable_red_healthbar;
    int enable_enemy_healthbar;
    int enable_enhanced_look;
    int enable_enhanced_ui;
    int enable_numeric_keys;
    int healthbar_showing_mode;
    int fix_end_of_level_freeze;
    int fix_tihocan_secret_sound;
} Tomb1MConfig;

struct {
    int stored_lara_health;
    int medipack_cooldown;
    // replicate glrage patch FPS counter repositioning
    int fps_x;
    int fps_y;
} Tomb1MData;

int Tomb1MGetRenderScale(int base);
int Tomb1MGetRenderScaleGLRage(int unit);
void Tomb1MRenderBar(int value, int value_max, int bar_type);
int Tomb1MGetRenderHeightDownscaled();
int Tomb1MGetRenderWidthDownscaled();
int Tomb1MGetRenderHeight();
int Tomb1MGetRenderWidth();

#endif
