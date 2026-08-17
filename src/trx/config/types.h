#pragma once

#include <trx/config/const.h>
#include <trx/config/enum.h>
#include <trx/core/colors.h>
#include <trx/gl/enum.h>

typedef struct {
    uint32_t time;
    uint32_t attempt_num;
} GYM_TRACK_ENTRY;

typedef struct {
    GYM_TRACK_ENTRY entries[MAX_ASSAULT_TIMES];
    uint32_t total_attempts;
} GYM_TRACK_STATS;

typedef struct {
    int32_t config_version;
    char *language;

    struct {
        bool new_game_plus_unlock;
    } profile;

    struct {
        INPUT_BACKEND backend; // Not decisive - mostly for UI visuals
        union {
            struct {
                int32_t keyboard_layout;
                int32_t controller_layout;
                int32_t touch_layout;
            };
            int32_t layout[INPUT_BACKEND_NUMBER_OF];
        };
        bool enable_tr3_sidesteps;
        bool enable_responsive_passport;
        bool enable_buffering_func_keys;
        bool enable_buffering_inventory;
        bool enable_touch_controls;
        bool enable_controller;
        float touch_opacity;
        float touch_button_scale;
        float touch_dpad_scale;
        float touch_dpad_deadzone;
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

        int32_t fov;

        CAMERA_MODE camera_mode;
        bool enable_glide_cameras;
        float game_brightness;
        float ui_brightness;
        float background_brightness;
        float gamma;

        bool enable_reflections;
        bool enable_3d_pickups;
        bool enable_braid;
        bool golden_lara;
        BREEZE_MODE breeze_mode;
        bool enable_gun_lighting;
        bool enable_gun_glow;
        bool enable_fire_lighting;
        bool enable_static_lighting;
        bool enable_shotgun_flash;
        bool enable_responsive_mesh_tint;
        char *lara_outfit;
        SUNGLASSES_MODE sunglasses_mode;
        SHADOW_TYPE shadow_type;
        BLOOD_EFFECTS blood_effects;
        bool enable_skybox;
        bool enable_weather;
        bool enable_poison_vision;
        bool enable_footprints;
        bool enable_droplets;
        bool enable_ps1_crystals;
        bool enable_ps1_rain;

        bool fix_item_rots;
        bool fix_animated_sprites;
        bool fix_texture_issues;

        RGB_888 water_color;
        bool fog_transparency;
        RGB_888 fog_color;
        int32_t fog_start;
        int32_t fog_end;
        bool enable_fog_bulbs;
    } visuals;

    struct {
        bool enable_game_ui;
        bool enable_photo_mode_ui;
        bool enable_wraparound;
        bool enable_fps_counter;
        bool show_pickups_overlay;
        bool show_title_version;

        float text_scale;
        float bar_scale;
        float pickup_scale;

        UI_STYLE menu_style;
        struct {
            STATS_STYLE style;
            bool show_totals;
            bool show_level_header;
            bool show_time_taken;
            bool show_secrets;
            bool show_crystals;
            bool show_pickups;
            bool show_kills;
            bool show_ammo;
            bool show_medipacks_used;
            bool show_distance_travelled;
            bool show_deaths;
        } stats;

        BACKGROUND_TYPE inventory_background_style;
        BACKGROUND_TYPE stats_background_style;
        BACKGROUND_TYPE pause_background_style;
        bool inventory_fade_effects;
        bool stats_fade_effects;
        bool pause_fade_effects;
        bool instant_save_load_screen;

        bool enable_smooth_bars;
        char *bar_look;
        bool show_bars;
        bool enable_bar_flashing;
        struct {
            UI_ELEMENT_LOCATION location;
            char *color;
            char *color_ps1;
            char *poison_color;
            char *poison_color_ps1;
        } lara_health_bar;
        struct {
            UI_ELEMENT_LOCATION location;
            char *color;
            char *color_ps1;
        } lara_air_bar, lara_sprint_bar, lara_exposure_bar;
        struct {
            BAR_SHOW_MODE show_mode;
            UI_ELEMENT_LOCATION location;
            char *color;
            char *color_ps1;
            char *color_allies;
            char *color_allies_ps1;
        } enemy_health_bar;
        struct {
            UI_ELEMENT_LOCATION location;
        } ammo_counter;
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

        bool fix_chainblock_secret_sound;
        bool fix_secrets_killing_music;
        bool fix_speeches_killing_music;
        bool fix_sliding_sfx;
        bool enable_lara_mic;
        bool enable_music_in_menu;
        bool enable_music_in_inventory;
        bool enable_music_on_death;
        bool enable_ps1_sfx;
        bool enable_pitched_sounds;
        bool load_music_triggers;
        bool enable_underwater_anim_sfx;
        bool mute_out_of_focus;

        MUSIC_LOAD_CONDITION music_load_condition;
    } audio;

    struct {
        bool disable_healing_between_levels;
        bool disable_medpacks;
        bool disable_extra_guns;
        bool enable_pickup_aids;
        SAVE_CRYSTAL_MODE save_crystal_mode;
        bool enable_enhanced_saves;

        bool enable_cheats;
        bool enable_console;
        GAME_MODES_POLICY game_modes_policy;
        bool enable_play_previous_levels;
        bool enable_fmv;
        bool enable_legal;
        bool enable_credits;
        INTRO_FMV_MODE intro_fmv_mode;
        bool enable_cinematics;
        bool enable_cinematic_skips;
        bool enable_cutscenes;
        bool enable_demo;
        LOADING_SCREENS_MODE loading_screens;
        bool enable_compass_stats;
        bool enable_total_stats;
        bool pause_on_focus_lost;
        bool enable_binoculars;

        bool enable_jump_twists;
        bool enable_underwater_auto_draw;
        bool enable_uw_roll;
        bool enable_crouch_roll;
        bool enable_tr2_swimming;
        bool enable_wading;
        bool enable_swamp_sidestepping;
        bool enable_tr2_swim_cancel;
        bool enable_tr2_jumping;
        bool enable_swing_cancel;
        bool enable_smooth_wall_deflect;
        bool enable_lean_jumping;
        bool enable_step_roll_boost;
        bool enable_slide_to_run;
        bool enable_back_slope_stumble;
        bool enable_alternative_turns;
        bool enable_controlled_drops;
        bool enable_ledge_jumps;
        bool enable_corner_shimmying;
        bool enable_fast_shimmying;
        bool enable_fast_pull_up;
        bool enable_crawling;
        bool enable_responsive_crawl;
        bool enable_crawl_jump;
        bool enable_crawl_tilt;
        bool enable_sprint;
        bool enable_responsive_sprint;
        bool enable_toggle_crouch;
        bool enable_toggle_sprint;
        bool enable_slow_ledge_swing;
        int32_t idle_pose_timeout;
        bool enable_idle_pose_camera;
        bool enable_soft_statics;
        bool enable_bouncy_grenades;

        bool enable_auto_item_selection;
        bool enable_manual_camera;
        bool enable_item_examining;
        TARGET_CHANGE_MODE target_change_mode;
        M16_AIM_MODE m16_aim_mode;
        bool enable_walk_to_items;
        bool enable_snap_interactions;
        bool enable_fast_pickups;
        bool enable_multiple_pickups;
        bool enable_continuous_pushblocks;
        bool restore_ps1_enemies;
        bool enable_ally_targeting;
        bool enable_ally_kill_count;
        bool enable_environment_kill_count;
        bool enable_enemy_rotation;
        bool enable_killer_pushblocks;
        bool enable_boulder_shake;
        bool enable_body_bags;
        ALLY_HOSTILITY_POLICY ally_hostility_policy;
        CREATURE_DROWN_POLICY creature_drown_policy;

        bool enable_timer_in_inventory;
        LOOK_MODE look_mode;
        bool enable_inverted_look;
        bool remember_gun_status;
        RING_MEMORY_MODE ring_memory_mode;

        int32_t turbo_speed;
        int32_t camera_speed;
        int32_t start_lara_hitpoints;
        int32_t maximum_save_slots;
        int32_t maximum_quick_save_slots;
        int32_t harpoon_recoil;

        JUMP_LOCK_MODE jump_lock_mode;
        TARGET_LOCK_MODE target_mode;
        bool fix_qwop_glitch;
        bool fix_step_glitch;
        bool fix_item_duplication_glitch;
        bool fix_descending_glitch;
        bool fix_lara_pickup_embed;
        bool fix_water_exit;
        WALL_GLITCH_MODE wall_glitch_mode;
        bool fix_wall_geometry;
        bool fix_alligator_ai;
        bool disable_trex_collision;
        bool change_pierre_spawn;
        bool fix_m16_accuracy;
        bool fix_flare_throw_priority;
        bool fix_free_flare_glitch;
        bool fix_walk_run_jump;
        bool fix_wade_wall_hit;
        bool fix_underwater_crawl;
        bool fix_lift_collision;
        bool fix_breakable_wall_clip;

        bool fix_floor_data_issues;
        bool fix_bridge_collision;
        bool fix_bear_ai;
        bool fix_monkey_pickup_priority;
        bool fix_pipeman_aim;
        bool fix_animated_spikes;

        PROJECTILE_AREA_DAMAGE projectile_area_damage;
    } gameplay;

    struct {
        ASPECT_MODE aspect_mode;
        int32_t fps;
        bool enable_trapezoid_filter;
        bool enable_affine_mapping;
        bool enable_lighting;
        bool enable_ps1_fog;
        bool enable_textures;
        TEXTURE_FILTER ui_filter;
        TEXTURE_FILTER texture_filter;
        TEXTURE_FILTER upscaling_filter;
        TEXTURE_FILTER fmv_filter;
        VERTEX_SNAP_MODE vertex_snap_mode;
        DITHER_MODE dither_mode;
        bool enable_wireframe;
        float wireframe_width;
        bool enable_vsync;
        float anisotropy_filter;
        SCREENSHOT_FORMAT screenshot_format;
        LIGHTING_CONTRAST lighting_contrast;
        LIGHTING_CURVE lighting_curve;
        BILLBOARD_LOCK_MODE sprite_lock_mode;
        int32_t upscaling_factor;
        int32_t supersampling_factor;
        int32_t multisampling_factor;
        float borders;
    } rendering;

    struct {
        bool enable_debug_triggers;
        bool enable_debug_portals;
        bool enable_debug_room_clip;
        bool enable_debug_spheres;
        bool enable_debug_bounding_boxes;
        bool enable_debug_pos;
        bool enable_debug_anim;
        bool enable_debug_camera;
        bool enable_debug_status;
        bool enable_review_markers;
        bool enable_invulnerability;
        bool enable_endless_sprint;
        bool enable_endless_flare_time;
    } debug;

    struct {
        bool lockout_option_ring;
        bool load_save_disabled;
        bool play_any_level;
        float demo_delay;
        bool cheat_keys;
    } flow;
} CONFIG;
