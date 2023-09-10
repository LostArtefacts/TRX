#pragma once

#include "gfx/context.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREENSHOT_FORMAT_JPEG,
    SCREENSHOT_FORMAT_PNG,
} SCREENSHOT_FORMAT;

typedef enum {
    UI_STYLE_PS1 = 0,
    UI_STYLE_PC = 1,
} UI_STYLE;

typedef enum {
    BL_TOP_LEFT = 0,
    BL_TOP_CENTER = 1,
    BL_TOP_RIGHT = 2,
    BL_BOTTOM_LEFT = 3,
    BL_BOTTOM_CENTER = 4,
    BL_BOTTOM_RIGHT = 5,
    BL_CUSTOM = 6,
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
    BC_PURPLE = 9,
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
    bool loaded;

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
    bool fix_floor_data_issues;
    bool fix_secrets_killing_music;
    bool fix_speeches_killing_music;
    bool fix_descending_glitch;
    bool fix_wall_jump_glitch;
    bool fix_bridge_collision;
    bool fix_qwop_glitch;
    bool fix_alligator_ai;
    bool change_pierre_spawn;
    bool fix_bear_ai;
    int32_t fov_value;
    bool fov_vertical;
    bool enable_demo;
    bool enable_fmv;
    bool enable_cine;
    bool enable_music_in_menu;
    bool enable_music_in_inventory;
    int32_t resolution_width;
    int32_t resolution_height;
    float brightness;
    bool enable_round_shadow;
    bool enable_3d_pickups;
    bool enable_detailed_stats;
    int32_t start_lara_hitpoints;
    bool walk_to_items;
    bool disable_trex_collision;
    int32_t maximum_save_slots;
    bool revert_to_pistols;
    bool enable_enhanced_saves;
    bool enable_pitched_sounds;
    bool enable_ps_uzi_sfx;
    bool enable_jump_twists;
    bool enabled_inverted_look;
    int32_t camera_speed;
    bool fix_texture_issues;
    bool enable_swing_cancel;
    bool enable_tr2_jumping;
    bool load_current_music;
    bool load_music_triggers;
    bool fix_item_rots;
    bool restore_ps1_enemies;
    bool enable_game_modes;
    bool enable_save_crystals;

    struct {
        int32_t layout;
        int32_t cntlr_layout;
    } input;

    struct {
        GFX_RENDER_MODE render_mode;
        bool enable_fullscreen;
        bool enable_maximized;
        int32_t window_x;
        int32_t window_y;
        int32_t window_width;
        int32_t window_height;
        bool enable_perspective_filter;
        bool enable_bilinear_filter;
        bool enable_vsync;
        bool enable_fps_counter;
        float anisotropy_filter;
    } rendering;

    struct {
        double text_scale;
        double bar_scale;
        UI_STYLE menu_style;
    } ui;

    struct {
        bool new_game_plus_unlock;
    } profile;

    int32_t sound_volume;
    int32_t music_volume;

    SCREENSHOT_FORMAT screenshot_format;
} CONFIG;

extern CONFIG g_Config;

bool Config_ReadFromJSON(const char *json);
bool Config_Read(void);
void Config_Init(void);
bool Config_Write(void);
