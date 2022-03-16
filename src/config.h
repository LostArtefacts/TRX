#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREENSHOT_FORMAT_JPEG,
    SCREENSHOT_FORMAT_PNG,
} SCREENSHOT_FORMAT;

typedef enum {
    BL_TOP_LEFT = 0,
    BL_TOP_CENTER = 1,
    BL_TOP_RIGHT = 2,
    BL_BOTTOM_LEFT = 3,
    BL_BOTTOM_CENTER = 4,
    BL_BOTTOM_RIGHT = 5,
} BAR_LOCATION;

typedef enum {
    BC_GOLD = 0,
    BC_BLUE = 1,
    BC_GREY = 2,
    BC_RED = 3,
    BC_SILVER = 4,
    BC_GREEN = 5,
    BC_GOLD2 = 6,
    BC_BLUE2 = 7,
    BC_PINK = 8,
} BAR_COLOR;

typedef enum {
    BSM_DEFAULT = 0,
    BSM_FLASHING_OR_DEFAULT = 1,
    BSM_FLASHING_ONLY = 2,
    BSM_ALWAYS = 3,
    BSM_NEVER = 4,
    BSM_PS1 = 5,
} BAR_SHOW_MODE;

typedef struct {
    bool disable_healing_between_levels;
    bool disable_medpacks;
    bool disable_magnums;
    bool disable_uzis;
    bool disable_shotgun;
    bool enable_deaths_counter;
    bool enable_enemy_healthbar;
    bool enable_enhanced_look;
    bool enable_numeric_keys;
    bool enable_shotgun_flash;
    bool fix_shotgun_targeting;
    bool enable_cheats;
    bool enable_tr3_sidesteps;
    bool enable_braid;
    bool enable_compass_stats;
    bool enable_total_stats;
    bool enable_timer_in_inventory;
    bool enable_smooth_bars;
    bool enable_fade_effects;
    int8_t healthbar_showing_mode;
    int8_t healthbar_location;
    int8_t healthbar_color;
    int8_t airbar_showing_mode;
    int8_t airbar_location;
    int8_t airbar_color;
    int8_t enemy_healthbar_location;
    int8_t enemy_healthbar_color;
    bool fix_tihocan_secret_sound;
    bool fix_pyramid_secret_trigger;
    bool fix_secrets_killing_music;
    bool fix_descending_glitch;
    bool fix_wall_jump_glitch;
    bool fix_bridge_collision;
    bool fix_qwop_glitch;
    bool fix_alligator_ai;
    int32_t fov_value;
    bool fov_vertical;
    bool disable_demo;
    bool disable_fmv;
    bool disable_cine;
    bool disable_music_in_menu;
    bool disable_music_in_inventory;
    int32_t resolution_width;
    int32_t resolution_height;
    bool enable_xbox_one_controller;
    float brightness;
    bool enable_round_shadow;
    bool enable_3d_pickups;
    bool enable_detailed_stats;
    int32_t start_lara_hitpoints;
    bool walk_to_items;

    struct {
        int32_t layout;
    } input;

    struct {
        bool enable_perspective_filter;
        bool enable_bilinear_filter;
        bool enable_vsync;
        bool enable_fps_counter;
        float anisotropy_filter;
    } rendering;

    struct {
        double text_scale;
        double bar_scale;
    } ui;

    int32_t sound_volume;
    int32_t music_volume;

    SCREENSHOT_FORMAT screenshot_format;
} CONFIG;

extern CONFIG g_Config;

bool Config_ReadFromJSON(const char *json);
bool Config_Read();
