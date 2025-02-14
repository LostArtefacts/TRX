#pragma once

#include "../game/input.h"
#include "../game/sound/enum.h"
#include "../gfx/common.h"
#include "../screenshot.h"

#include <stdint.h>

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

    struct {
        int32_t keyboard_layout;
        int32_t controller_layout;
        bool enable_tr3_sidesteps;
        bool enable_responsive_passport;
    } input;

    struct {
        bool enable_3d_pickups;
        bool enable_gun_lighting;
        bool enable_fade_effects;
        bool enable_exit_fade_effects;
        bool fix_item_rots;
        int32_t fov;
        bool use_pcx_fov;
    } visuals;

    struct {
        bool enable_photo_mode_ui;
        double text_scale;
        double bar_scale;
    } ui;

    struct {
        int32_t sound_volume;
        int32_t music_volume;
        bool enable_lara_mic;
        UNDERWATER_MUSIC_MODE underwater_music_mode;
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
        bool enable_cheats;
        bool enable_console;
        bool enable_fmv;
        bool enable_auto_item_selection;
        int32_t turbo_speed;
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
        RENDER_MODE render_mode;
        ASPECT_MODE aspect_mode;
        bool enable_zbuffer;
        bool enable_perspective_filter;
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
} CONFIG;
