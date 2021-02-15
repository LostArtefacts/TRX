#ifndef TR1MAIN_MOD_H
#define TR1MAIN_MOD_H

#include "util.h"

typedef enum {
    TR1M_BAR_LARA_HEALTH = 0,
    TR1M_BAR_LARA_AIR = 1,
    TR1M_BAR_ENEMY_HEALTH = 2,
    TR1M_BAR_NUMBER = 3,
} TR1M_BAR;

typedef enum {
    TR1M_BSM_DEFAULT = 0,
    TR1M_BSM_FLASHING = 1,
    TR1M_BSM_ALWAYS = 2,
} TR1M_BAR_SHOW_MODE;

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
} TR1MConfig;

struct {
    int stored_lara_health;
    int medipack_cooldown;
    // replicate glrage patch FPS counter repositioning
    int fps_x;
    int fps_y;
} TR1MData;

int TR1MGetRenderScale(int base);
int TR1MGetRenderScaleGLRage(int unit);
void TR1MRenderBar(int value, int value_max, int bar_type);
int TR1MGetRenderHeightDownscaled();
int TR1MGetRenderWidthDownscaled();
int TR1MGetRenderHeight();
int TR1MGetRenderWidth();

#endif
