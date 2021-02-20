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
    int8_t healthbar_showing_mode;
    int8_t fix_end_of_level_freeze;
    int8_t fix_tihocan_secret_sound;
    int8_t fix_pyramid_secret_trigger;
    int8_t fix_hardcoded_secret_counts;
} Tomb1MConfig;

struct {
    int stored_lara_health;
    int medipack_cooldown;
    // replicate glrage patch FPS counter repositioning
    int fps_x;
    int fps_y;
} Tomb1MData;

void Tomb1MRenderBar(int value, int value_max, int bar_type);

int Tomb1MGetRenderScale(int base);
int Tomb1MGetRenderScaleGLRage(int unit);
int Tomb1MGetRenderHeightDownscaled();
int Tomb1MGetRenderWidthDownscaled();
int Tomb1MGetRenderHeight();
int Tomb1MGetRenderWidth();
int Tomb1MGetSecretCount();
void Tomb1MFixPyramidSecretTrigger();

#endif
