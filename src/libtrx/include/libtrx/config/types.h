#pragma once

#include "../colors.h"
#include "../game/input.h"
#include "../game/sound/enum.h"
#include "../gfx/common.h"
#include "../screenshot.h"
#include "./const.h"
#include "./enum.h"

typedef struct {
    struct {
        uint32_t time;
        uint32_t attempt_num;
    } entries[MAX_ASSAULT_TIMES];
    uint32_t total_attempts;
} ASSAULT_STATS;

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
        bool new_game_plus_unlock;
        ASSAULT_STATS assault_stats;
    } profile;

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
#if TR_VERSION == 1
        bool enable_buffering;
#endif
        QUICK_GUNS_MODE quick_guns_mode;
    } input;

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
        bool enable_fade_effects;
        bool enable_exit_fade_effects;

#if TR_VERSION == 1
        int32_t fov_value;
        bool fov_vertical;
#else
        int32_t fov;
        bool use_ps1_fov;
#endif

        CAMERA_MODE camera_mode;
        float brightness;

        bool enable_reflections;
        bool enable_3d_pickups;
        bool enable_braid;
        bool enable_breeze;
        bool enable_gun_lighting;
        bool enable_fire_lighting;
#if TR_VERSION == 1
        bool enable_shotgun_flash;
#endif
        bool enable_round_shadow;
        bool enable_skybox;
#if TR_VERSION == 1
        bool enable_ps1_crystals;
#endif

        bool fix_item_rots;
        bool fix_animated_sprites;
        bool fix_texture_issues;
#if TR_VERSION == 2
        bool fix_glide_cameras;
#endif

        RGB_888 water_color;
        bool fog_transparency;
        RGB_888 fog_color;
        int32_t fog_start;
        int32_t fog_end;
    } visuals;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        bool enable_wraparound;
        bool enable_fps_counter;

        float text_scale;
        float bar_scale;
        float pickup_scale;

        UI_STYLE menu_style;
#if TR_VERSION == 1
        STAT_DETAIL_MODE stat_detail_mode;
#endif
        bool enable_stats_level_header;

        BACKGROUND_TYPE inventory_background_style;
        BACKGROUND_TYPE stats_background_style;

        bool enable_smooth_bars;
        BAR_LOOK bar_look;
        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
        } lara_health_bar, lara_air_bar, lara_sprint_bar, lara_exposure_bar;
        struct {
            BAR_SHOW_MODE show_mode;
            BAR_LOCATION location;
            BAR_COLOR color;
            BAR_COLOR color_allies;
        } enemy_health_bar;
    } ui;

    struct {
        float master_volume;
        float sound_volume;
        float music_volume;
        float inventory_music_volume;
        float underwater_music_volume;
        float ambient_volume;
        float inventory_ambient_volume;
        float underwater_ambient_volume;
        float cutscene_volume;
        float fmv_volume;

#if TR_VERSION == 1
        bool fix_tihocan_secret_sound;
        bool fix_secrets_killing_music;
        bool fix_speeches_killing_music;
#else
        bool enable_lara_mic;
#endif
        bool enable_music_in_menu;
        bool enable_music_in_inventory;
#if TR_VERSION == 1
        bool enable_ps_uzi_sfx;
#endif
        bool enable_pitched_sounds;
#if TR_VERSION == 1
        bool load_music_triggers;
#else
        bool enable_barefoot_sfx;
#endif
        bool enable_underwater_anim_sfx;
        bool mute_out_of_focus;

        MUSIC_LOAD_CONDITION music_load_condition;
    } audio;

    struct {
        bool disable_healing_between_levels;
#if TR_VERSION == 1
        bool disable_medpacks;
        bool disable_magnums;
        bool disable_uzis;
        bool disable_shotgun;
        bool enable_deaths_counter;
        bool enable_pickup_aids;
#endif
        bool enable_save_crystals;
        bool enable_enhanced_saves;

        bool enable_cheats;
        bool enable_console;
        bool enable_game_modes;
        bool enable_fmv;
        bool enable_legal;
        bool enable_credits;
        bool enable_cutscenes;
        bool enable_demo;
#if TR_VERSION == 1
        bool enable_loading_screens;
        bool enable_compass_stats;
        bool enable_total_stats;
#endif

        bool enable_jump_twists;
        bool enable_uw_roll;
        bool enable_tr2_swimming;
#if TR_VERSION == 1
        bool enable_wading;
        bool enable_tr2_swim_cancel;
        bool enable_tr2_jumping;
#endif
        bool enable_swing_cancel;
        bool enable_smooth_wall_deflect;
        bool enable_lean_jumping;
        bool enable_step_roll_boost;
        bool enable_slide_to_run;
        bool enable_neutral_twists;
        bool enable_controlled_drops;
        bool enable_ledge_jumps;
        bool enable_sprint;
        bool enable_responsive_sprint;
        int32_t idle_pose_timeout;
        bool enable_idle_pose_camera;

        bool enable_auto_item_selection;
        bool enable_item_examining;
        bool enable_target_change;
#if TR_VERSION == 1
        bool enable_walk_to_items;
        bool restore_ps1_enemies;
#else
        bool enable_ally_targeting;
#endif
        bool enable_enemy_rotation;

        bool enable_timer_in_inventory;
        LOOK_MODE look_mode;
        bool enable_inverted_look;
        bool revert_to_pistols;

        int32_t turbo_speed;
        int32_t camera_speed;
        int32_t start_lara_hitpoints;
#if TR_VERSION == 1
        int32_t maximum_save_slots;
#else
        int32_t harpoon_recoil;
#endif

        JUMP_LOCK_MODE jump_lock_mode;
        TARGET_LOCK_MODE target_mode;
        bool fix_qwop_glitch;
        bool fix_step_glitch;
        bool fix_item_duplication_glitch;
        bool fix_descending_glitch;
        bool fix_lara_pickup_embed;
        bool fix_water_exit;
        WALL_GLITCH_MODE wall_glitch_mode;
        bool fix_alligator_ai;
        bool disable_trex_collision;
        bool change_pierre_spawn;
#if TR_VERSION == 1
        bool fix_shotgun_targeting;
#else
        bool fix_m16_accuracy;
        bool fix_free_flare_glitch;
        bool fix_flare_throw_priority;
#endif
        bool fix_walk_run_jump;
        bool fix_wade_wall_hit;

        bool fix_floor_data_issues;
        bool fix_bridge_collision;
        bool fix_bear_ai;
    } gameplay;

    struct {
        ASPECT_MODE aspect_mode;
        int32_t fps;
        bool enable_trapezoid_filter;
        bool enable_lighting;
        GFX_TEXTURE_FILTER ui_filter;
        GFX_TEXTURE_FILTER texture_filter;
        GFX_TEXTURE_FILTER upscaling_filter;
        bool enable_wireframe;
        float wireframe_width;
        bool enable_vsync;
        float anisotropy_filter;
        SCREENSHOT_FORMAT screenshot_format;
        LIGHTING_CONTRAST lighting_contrast;
        BILLBOARD_LOCK_MODE sprite_lock_mode;
        int32_t upscaling_factor;
        float borders;
    } rendering;

    struct {
        bool enable_debug_triggers;
        bool enable_debug_portals;
        bool enable_debug_room_clip;
        bool enable_debug_spheres;
        bool enable_debug_cuboids;
        bool enable_debug_pos;
        bool enable_review_markers;
        bool enable_invulnerability;
        bool enable_endless_sprint;
    } debug;
} CONFIG;
