#pragma once

#include "../game/sound/enum.h"
#include "../gfx/common.h"
#include "../screenshot.h"

#include <stdint.h>

#define CONFIG_MIN_BRIGHTNESS 0.1f
#define CONFIG_MAX_BRIGHTNESS 2.0f
#define CONFIG_MIN_TEXT_SCALE 0.5
#define CONFIG_MAX_TEXT_SCALE 2.0
#define CONFIG_MIN_BAR_SCALE 0.5
#define CONFIG_MAX_BAR_SCALE 1.5

typedef enum {
    BSM_DEFAULT,
    BSM_FLASHING_OR_DEFAULT,
    BSM_FLASHING_ONLY,
    BSM_ALWAYS,
    BSM_NEVER,
    BSM_PS1,
    BSM_BOSS_ONLY,
} BAR_SHOW_MODE;

typedef enum {
    BL_TOP_LEFT,
    BL_TOP_CENTER,
    BL_TOP_RIGHT,
    BL_BOTTOM_LEFT,
    BL_BOTTOM_CENTER,
    BL_BOTTOM_RIGHT,
    BL_CUSTOM,
} BAR_LOCATION;

typedef enum {
    BC_GOLD,
    BC_BLUE,
    BC_GREY,
    BC_RED,
    BC_SILVER,
    BC_GREEN,
    BC_GOLD2,
    BC_BLUE2,
    BC_PINK,
    BC_PURPLE,
} BAR_COLOR;

typedef enum {
    MUSIC_LOAD_NEVER,
    MUSIC_LOAD_NON_AMBIENT,
    MUSIC_LOAD_ALWAYS,
} MUSIC_LOAD_CONDITION;

typedef enum {
    TLM_FULL,
    TLM_SEMI,
    TLM_NONE,
} TARGET_LOCK_MODE;

typedef enum {
    UI_STYLE_PS1,
    UI_STYLE_PC,
} UI_STYLE;

typedef struct {
    bool loaded;

    struct {
        int32_t keyboard_layout;
        int32_t controller_layout;
        bool enable_numeric_keys;
        bool enable_tr3_sidesteps;
        bool enable_responsive_passport;
        bool enable_buffering;
    } input;

    struct {
        bool enable_fade_effects;
        bool enable_exit_fade_effects;
        int32_t fov_value;
        bool fov_vertical;
        float brightness;

        bool enable_reflections;
        bool enable_3d_pickups;
        bool enable_braid;
        bool enable_gun_lighting;
        bool enable_shotgun_flash;
        bool enable_round_shadow;
        bool enable_skybox;
        bool fix_item_rots;
        bool fix_animated_sprites;
        bool fix_texture_issues;
        bool enable_ps1_crystals;
    } visuals;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        double text_scale;
        double bar_scale;
        UI_STYLE menu_style;

        bool enable_smooth_bars;
        BAR_SHOW_MODE healthbar_show_mode;
        BAR_LOCATION healthbar_location;
        BAR_COLOR healthbar_color;
        BAR_SHOW_MODE airbar_show_mode;
        BAR_LOCATION airbar_location;
        BAR_COLOR airbar_color;
        BAR_SHOW_MODE enemy_healthbar_show_mode;
        BAR_LOCATION enemy_healthbar_location;
        BAR_COLOR enemy_healthbar_color;
    } ui;

    struct {
        int32_t sound_volume;
        int32_t music_volume;
        bool fix_tihocan_secret_sound;
        bool fix_secrets_killing_music;
        bool fix_speeches_killing_music;
        bool enable_music_in_menu;
        bool enable_music_in_inventory;
        bool enable_ps_uzi_sfx;
        bool enable_pitched_sounds;
        bool load_music_triggers;
        UNDERWATER_MUSIC_MODE underwater_music_mode;
        MUSIC_LOAD_CONDITION music_load_condition;
    } audio;

    struct {
        bool disable_healing_between_levels;
        bool disable_medpacks;
        bool disable_magnums;
        bool disable_uzis;
        bool disable_shotgun;
        bool enable_deaths_counter;
        bool enable_pickup_aids;
        bool enable_enhanced_look;
        bool enable_cheats;
        bool enable_console;
        bool enable_fmv;
        bool enable_compass_stats;
        bool enable_total_stats;
        bool enable_timer_in_inventory;
        bool enable_demo;
        bool enable_eidos_logo;
        bool enable_loading_screens;
        bool enable_cine;
        bool enable_detailed_stats;
        bool enable_walk_to_items;
        bool enable_enhanced_saves;
        bool enable_jump_twists;
        bool enable_inverted_look;
        bool enable_wading;
        bool enable_game_modes;
        bool enable_save_crystals;
        bool enable_uw_roll;
        bool enable_lean_jumping;
        bool enable_target_change;
        bool enable_item_examining;
        bool enable_auto_item_selection;
        bool enable_tr2_jumping;
        bool enable_tr2_swimming;
        bool enable_tr2_swim_cancel;
        bool enable_swing_cancel;
        bool fix_floor_data_issues;
        bool fix_descending_glitch;
        bool fix_wall_jump_glitch;
        bool fix_bridge_collision;
        bool fix_qwop_glitch;
        bool fix_item_duplication_glitch;
        bool fix_alligator_ai;
        bool fix_shotgun_targeting;
        bool fix_bear_ai;
        bool revert_to_pistols;
        bool change_pierre_spawn;
        bool disable_trex_collision;
        bool restore_ps1_enemies;
        int32_t turbo_speed;
        int32_t start_lara_hitpoints;
        int32_t maximum_save_slots;
        int32_t camera_speed;
        TARGET_LOCK_MODE target_mode;
    } gameplay;

    struct {
        bool is_fullscreen;
        bool is_maximized;
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    } window;

    struct {
        int32_t resolution_width;
        int32_t resolution_height;
        GFX_RENDER_MODE render_mode;
        int32_t fps;
        bool enable_perspective_filter;
        GFX_TEXTURE_FILTER texture_filter;
        GFX_TEXTURE_FILTER fbo_filter;
        bool enable_debug_triggers;
        bool enable_debug_portals;
        bool enable_wireframe;
        double wireframe_width;
        bool enable_vsync;
        bool enable_fps_counter;
        float anisotropy_filter;
        bool pretty_pixels;
        SCREENSHOT_FORMAT screenshot_format;
    } rendering;

    struct {
        bool new_game_plus_unlock;
    } profile;
} CONFIG;
