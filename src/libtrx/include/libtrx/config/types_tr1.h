#pragma once

#include "../game/gym.h"
#include "../game/input.h"
#include "../game/output/types.h"
#include "../game/sound/enum.h"
#include "../gfx/common.h"
#include "../screenshot.h"

#define CONFIG_MIN_BRIGHTNESS 0.1f
#define CONFIG_MAX_BRIGHTNESS 2.0f

typedef enum {
    TLM_FULL,
    TLM_SEMI,
    TLM_NONE,
} TARGET_LOCK_MODE;

typedef enum {
    SDM_MINIMAL,
    SDM_DETAILED,
    SDM_FULL,
} STAT_DETAIL_MODE;

typedef struct {
    bool loaded;
    char *language;

    struct {
        union {
            struct {
                int32_t keyboard_layout;
                int32_t controller_layout;
            };
            int32_t layout[INPUT_BACKEND_NUMBER_OF];
        };
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
        CAMERA_MODE camera_mode;
        float brightness;

        bool enable_reflections;
        bool enable_3d_pickups;
        bool enable_braid;
        bool enable_breeze;
        bool enable_gun_lighting;
        bool enable_shotgun_flash;
        bool enable_round_shadow;
        bool enable_skybox;
        bool fix_item_rots;
        bool fix_animated_sprites;
        bool fix_texture_issues;
        bool enable_ps1_crystals;

        RGB_888 water_color;
        int32_t fog_start;
        int32_t fog_end;
    } visuals;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        bool enable_wraparound;
        bool enable_fps_counter;
        double text_scale;
        double bar_scale;
        UI_STYLE menu_style;

        bool enable_smooth_bars;
        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
        } enemy_health_bar, lara_health_bar, lara_air_bar;
    } ui;

    struct {
        float sound_volume;
        float music_volume;
        bool fix_tihocan_secret_sound;
        bool fix_secrets_killing_music;
        bool fix_speeches_killing_music;
        bool enable_music_in_menu;
        bool enable_music_in_inventory;
        bool enable_ps_uzi_sfx;
        bool enable_pitched_sounds;
        bool load_music_triggers;
        float inventory_ambient_volume;
        float inventory_music_volume;
        float underwater_ambient_volume;
        float underwater_music_volume;
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
        bool enable_legal;
        bool enable_credits;
        bool enable_compass_stats;
        bool enable_total_stats;
        bool enable_timer_in_inventory;
        bool enable_demo;
        bool enable_loading_screens;
        bool enable_cutscenes;
        STAT_DETAIL_MODE stat_detail_mode;
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
        bool enable_smooth_wall_deflect;
        bool enable_step_roll_boost;
        bool fix_floor_data_issues;
        bool fix_descending_glitch;
        WALL_GLITCH_MODE wall_glitch_mode;
        bool fix_bridge_collision;
        bool fix_qwop_glitch;
        bool fix_item_duplication_glitch;
        bool fix_alligator_ai;
        bool fix_shotgun_targeting;
        bool fix_bear_ai;
        bool fix_wade_wall_hit;
        bool fix_step_glitch;
        bool fix_water_exit;
        bool revert_to_pistols;
        bool change_pierre_spawn;
        bool disable_trex_collision;
        bool restore_ps1_enemies;
        bool enable_enemy_rotation;
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
        bool enable_trapezoid_filter;
        GFX_TEXTURE_FILTER texture_filter;
        GFX_TEXTURE_FILTER fbo_filter;
        bool enable_wireframe;
        double wireframe_width;
        bool enable_vsync;
        float anisotropy_filter;
        SCREENSHOT_FORMAT screenshot_format;
    } rendering;

    struct {
        bool enable_debug_triggers;
        bool enable_debug_portals;
        bool enable_debug_spheres;
        bool enable_debug_pos;
        bool enable_review_markers;
    } debug;

    struct {
        bool new_game_plus_unlock;
        ASSAULT_STATS assault_stats;
    } profile;
} CONFIG;
