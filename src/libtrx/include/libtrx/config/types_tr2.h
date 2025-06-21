#pragma once

#include "../colors.h"
#include "../game/gym.h"
#include "../game/input.h"
#include "../game/sound/enum.h"
#include "../gfx/common.h"
#include "../screenshot.h"

typedef enum {
    LIGHTING_CONTRAST_LOW,
    LIGHTING_CONTRAST_MEDIUM,
    LIGHTING_CONTRAST_HIGH,
    LIGHTING_CONTRAST_NUMBER_OF,
} LIGHTING_CONTRAST;

typedef enum {
    AM_4_3 = 0,
    AM_16_9 = 1,
    AM_ANY = 2,
} ASPECT_MODE;

typedef enum {
    RM_UNKNOWN = 0,
    RM_SOFTWARE = 1,
    RM_HARDWARE = 2,
} RENDER_MODE;

typedef enum {
    TAM_DISABLED = 0,
    TAM_BILINEAR_ONLY = 1,
    TAM_ALWAYS = 2,
} TEXEL_ADJUST_MODE;

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
        bool enable_tr3_sidesteps;
        bool enable_responsive_passport;
    } input;

    struct {
        bool enable_3d_pickups;
        bool enable_gun_lighting;
        bool enable_braid;
        bool enable_breeze;
        bool enable_fade_effects;
        bool enable_exit_fade_effects;
        bool fix_item_rots;
        bool fix_texture_issues;
        bool fix_glide_cameras;
        int32_t fov;
        bool use_psx_fov;
        CAMERA_MODE camera_mode;

        RGB_888 water_color;
        int32_t fog_start;
        int32_t fog_end;
    } visuals;

    struct {
        bool enable_debug_pos;
        bool enable_review_markers;
    } debug;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        bool enable_wraparound;
        bool enable_fps_counter;
        double text_scale;
        double bar_scale;

        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
        } enemy_health_bar, lara_health_bar, lara_air_bar;
    } ui;

    struct {
        float sound_volume;
        float music_volume;
        bool enable_lara_mic;
        bool enable_music_in_inventory;
        float inventory_ambient_volume;
        float inventory_music_volume;
        float underwater_ambient_volume;
        float underwater_music_volume;
        MUSIC_LOAD_CONDITION music_load_condition;
    } audio;

    struct {
        bool fix_m16_accuracy;
        bool fix_item_duplication_glitch;
        bool fix_qwop_glitch;
        bool fix_step_glitch;
        bool fix_free_flare_glitch;
        bool fix_pickup_drift_glitch;
        bool fix_floor_data_issues;
        bool fix_flare_throw_priority;
        bool fix_walk_run_jump;
        bool fix_bear_ai;
        bool fix_bridge_collision;
        bool fix_wade_wall_hit;
        bool fix_descending_glitch;
        WALL_GLITCH_MODE wall_glitch_mode;
        bool fix_water_exit;
        bool enable_cheats;
        bool enable_console;
        bool enable_fmv;
        bool enable_legal;
        bool enable_cutscenes;
        bool enable_enhanced_saves;
        bool enable_item_examining;
        bool enable_auto_item_selection;
        bool enable_swing_cancel;
        bool enable_lean_jumping;
        bool enable_smooth_wall_deflect;
        bool enable_step_roll_boost;
        bool enable_enemy_rotation;
        bool enable_ally_targeting;
        int32_t turbo_speed;
        int32_t camera_speed;
        bool enable_game_modes;
        int32_t harpoon_recoil;
        int32_t start_lara_hitpoints;
        bool disable_healing_between_levels;
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
        int32_t fps;
        RENDER_MODE render_mode;
        ASPECT_MODE aspect_mode;
        bool enable_zbuffer;
        bool enable_perspective_filter;
        bool enable_trapezoid_filter;
        bool enable_wireframe;
        float wireframe_width;
        GFX_TEXTURE_FILTER texture_filter;
        SCREENSHOT_FORMAT screenshot_format;
        LIGHTING_CONTRAST lighting_contrast;
        TEXEL_ADJUST_MODE texel_adjust_mode;
        int32_t nearest_adjustment;
        int32_t linear_adjustment;
        int32_t scaler;
        float sizer;
    } rendering;

    struct {
        bool new_game_plus_unlock;
        ASSAULT_STATS assault_stats;
    } profile;
} CONFIG;
