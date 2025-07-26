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
    // This signifies whether the config was already read from disk.
    bool loaded;

    // This holds paths passed to Config_Read(), so that Config_Write() knows
    // where to save the updates.
    char *default_path;
    char *enforced_path;

    // This field is used to force trigger a change event for fields that are
    // not stored in the CONFIG struct.
    bool dirty;

    // Start of user fields
    char *language;

    struct {
        INPUT_BACKEND backend; // Not decisive - mostly for UI visuals
        union {
            struct {
                int32_t keyboard_layout;
                int32_t controller_layout;
            };
            int32_t layout[INPUT_BACKEND_NUMBER_OF];
        };
        bool enable_tr3_sidesteps;
        bool enable_responsive_passport;
        QUICK_GUNS_MODE quick_guns_mode;
    } input;

    struct {
        bool enable_3d_pickups;
        bool enable_gun_lighting;
        bool enable_fire_lighting;
        bool enable_braid;
        bool enable_breeze;
        bool enable_fade_effects;
        bool enable_exit_fade_effects;
        bool fix_item_rots;
        bool fix_texture_issues;
        bool fix_animated_sprites;
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
        bool enable_invulnerability;
    } debug;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        bool enable_wraparound;
        bool enable_fps_counter;
        double text_scale;
        double bar_scale;
        bool enable_stats_level_header;

        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
        } lara_health_bar, lara_air_bar;
        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
            BAR_COLOR color_allies;
        } enemy_health_bar;
    } ui;

    struct {
        float sound_volume;
        float music_volume;
        bool enable_lara_mic;
        bool enable_underwater_anim_sfx;
        bool enable_music_in_inventory;
        bool mute_out_of_focus;
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
        JUMP_LOCK_MODE jump_lock_mode;
        WALL_GLITCH_MODE wall_glitch_mode;
        bool fix_water_exit;
        LOOK_MODE look_mode;
        bool enable_inverted_look;
        bool enable_uw_roll;
        bool enable_tr2_swimming;
        bool enable_jump_twists;
        bool enable_cheats;
        bool enable_console;
        bool enable_fmv;
        bool enable_legal;
        bool enable_credits;
        bool enable_cutscenes;
        bool enable_enhanced_saves;
        bool enable_item_examining;
        bool enable_auto_item_selection;
        bool enable_swing_cancel;
        bool enable_lean_jumping;
        bool enable_smooth_wall_deflect;
        bool enable_step_roll_boost;
        bool enable_slide_to_run;
        bool enable_neutral_twists;
        bool enable_controlled_drops;
        bool enable_enemy_rotation;
        bool enable_ally_targeting;
        bool revert_to_pistols;
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
        int32_t fs_width;
        int32_t fs_height;
    } window;

    struct {
        int32_t fps;
        RENDER_MODE render_mode;
        ASPECT_MODE aspect_mode;
        bool enable_zbuffer;
        bool enable_perspective_filter;
        bool enable_trapezoid_filter;
        bool enable_lighting;
        bool enable_wireframe;
        float wireframe_width;
        GFX_TEXTURE_FILTER ui_filter;
        GFX_TEXTURE_FILTER texture_filter;
        GFX_TEXTURE_FILTER upscaling_filter;
        SCREENSHOT_FORMAT screenshot_format;
        LIGHTING_CONTRAST lighting_contrast;
        TEXEL_ADJUST_MODE texel_adjust_mode;
        int32_t nearest_adjustment;
        int32_t linear_adjustment;
        int32_t upscaling_factor;
        float borders;
    } rendering;

    struct {
        bool new_game_plus_unlock;
        ASSAULT_STATS assault_stats;
    } profile;
} CONFIG;
