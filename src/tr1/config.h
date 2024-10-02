#pragma once

#include "global/types.h"

#include <libtrx/config.h>
#include <libtrx/gfx/common.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool loaded;

    bool disable_healing_between_levels;
    bool disable_medpacks;
    bool disable_magnums;
    bool disable_uzis;
    bool disable_shotgun;
    bool enable_deaths_counter;
    bool enable_enhanced_look;
    bool enable_numeric_keys;
    bool enable_shotgun_flash;
    bool fix_shotgun_targeting;
    bool enable_cheats;
    bool enable_console;
    bool enable_tr3_sidesteps;
    bool enable_braid;
    bool enable_compass_stats;
    bool enable_total_stats;
    bool enable_timer_in_inventory;
    bool enable_smooth_bars;
    bool enable_fade_effects;
    BAR_SHOW_MODE healthbar_show_mode;
    BAR_LOCATION healthbar_location;
    BAR_COLOR healthbar_color;
    BAR_SHOW_MODE airbar_show_mode;
    BAR_LOCATION airbar_location;
    BAR_COLOR airbar_color;
    BAR_SHOW_MODE enemy_healthbar_show_mode;
    BAR_LOCATION enemy_healthbar_location;
    BAR_COLOR enemy_healthbar_color;
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
    bool enable_eidos_logo;
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
    UNDERWATER_MUSIC_MODE underwater_music_mode;
    bool enable_jump_twists;
    bool enabled_inverted_look;
    int32_t camera_speed;
    bool fix_texture_issues;
    bool enable_swing_cancel;
    bool enable_tr2_jumping;
    bool enable_tr2_swimming;
    bool load_current_music;
    bool load_music_triggers;
    bool fix_item_rots;
    bool restore_ps1_enemies;
    bool enable_game_modes;
    bool enable_save_crystals;
    bool enable_uw_roll;
    bool enable_buffering;
    bool enable_lean_jumping;
    bool enable_target_change;
    TARGET_LOCK_MODE target_mode;
    bool enable_loading_screens;
    bool fix_animated_sprites;
    bool enable_skybox;
    bool enable_ps1_crystals;

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
        int32_t fps;
        bool enable_perspective_filter;
        GFX_TEXTURE_FILTER texture_filter;
        GFX_TEXTURE_FILTER fbo_filter;
        bool enable_wireframe;
        double wireframe_width;
        bool enable_vsync;
        bool enable_fps_counter;
        float anisotropy_filter;
        int32_t turbo_speed;
        bool pretty_pixels;
        bool enable_reflections;
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
