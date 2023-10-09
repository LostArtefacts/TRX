#include "config_map.h"

#include "config.h"
#include "gfx/common.h"
#include "global/const.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const CONFIG_OPTION_ENUM_MAP m_UIStyles[] = {
    { "ps1", UI_STYLE_PS1 },
    { "pc", UI_STYLE_PC },
    { NULL, -1 },
};

static const CONFIG_OPTION_ENUM_MAP m_BarShowingModes[] = {
    { "default", BSM_DEFAULT },
    { "flashing-or-default", BSM_FLASHING_OR_DEFAULT },
    { "flashing-only", BSM_FLASHING_ONLY },
    { "always", BSM_ALWAYS },
    { "never", BSM_NEVER },
    { "ps1", BSM_PS1 },
    { NULL, -1 },
};

static const CONFIG_OPTION_ENUM_MAP m_BarLocations[] = {
    { "top-left", BL_TOP_LEFT },
    { "top-center", BL_TOP_CENTER },
    { "top-right", BL_TOP_RIGHT },
    { "bottom-left", BL_BOTTOM_LEFT },
    { "bottom-center", BL_BOTTOM_CENTER },
    { "bottom-right", BL_BOTTOM_RIGHT },
    { NULL, -1 },
};

static const CONFIG_OPTION_ENUM_MAP m_BarColors[] = {
    { "gold", BC_GOLD },   { "blue", BC_BLUE },     { "grey", BC_GREY },
    { "red", BC_RED },     { "silver", BC_SILVER }, { "green", BC_GREEN },
    { "gold2", BC_GOLD2 }, { "blue2", BC_BLUE2 },   { "pink", BC_PINK },
    { NULL, -1 },
};

static const CONFIG_OPTION_ENUM_MAP m_ScreenshotFormats[] = {
    { "jpg", SCREENSHOT_FORMAT_JPEG },
    { "jpeg", SCREENSHOT_FORMAT_JPEG },
    { "png", SCREENSHOT_FORMAT_PNG },
    { NULL, -1 },
};

const CONFIG_OPTION g_ConfigOptionMap[] = {
    // clang-format off
    { .name = "disable_healing_between_levels",      .type = COT_BOOL,  .target = &g_Config.disable_healing_between_levels,      .default_value = &(bool){false}},
    { .name = "disable_medpacks",                    .type = COT_BOOL,  .target = &g_Config.disable_medpacks,                    .default_value = &(bool){false}},
    { .name = "disable_magnums",                     .type = COT_BOOL,  .target = &g_Config.disable_magnums,                     .default_value = &(bool){false}},
    { .name = "disable_uzis",                        .type = COT_BOOL,  .target = &g_Config.disable_uzis,                        .default_value = &(bool){false}},
    { .name = "disable_shotgun",                     .type = COT_BOOL,  .target = &g_Config.disable_shotgun,                     .default_value = &(bool){false}},
    { .name = "enable_detailed_stats",               .type = COT_BOOL,  .target = &g_Config.enable_detailed_stats,               .default_value = &(bool){true}},
    { .name = "enable_deaths_counter",               .type = COT_BOOL,  .target = &g_Config.enable_deaths_counter,               .default_value = &(bool){true}},
    { .name = "enable_enemy_healthbar",              .type = COT_BOOL,  .target = &g_Config.enable_enemy_healthbar,              .default_value = &(bool){true}},
    { .name = "enable_enhanced_look",                .type = COT_BOOL,  .target = &g_Config.enable_enhanced_look,                .default_value = &(bool){true}},
    { .name = "enable_shotgun_flash",                .type = COT_BOOL,  .target = &g_Config.enable_shotgun_flash,                .default_value = &(bool){true}},
    { .name = "fix_shotgun_targeting",               .type = COT_BOOL,  .target = &g_Config.fix_shotgun_targeting,               .default_value = &(bool){true}},
    { .name = "enable_cheats",                       .type = COT_BOOL,  .target = &g_Config.enable_cheats,                       .default_value = &(bool){false}},
    { .name = "enable_numeric_keys",                 .type = COT_BOOL,  .target = &g_Config.enable_numeric_keys,                 .default_value = &(bool){true}},
    { .name = "enable_tr3_sidesteps",                .type = COT_BOOL,  .target = &g_Config.enable_tr3_sidesteps,                .default_value = &(bool){true}},
    { .name = "enable_braid",                        .type = COT_BOOL,  .target = &g_Config.enable_braid,                        .default_value = &(bool){false}},
    { .name = "enable_compass_stats",                .type = COT_BOOL,  .target = &g_Config.enable_compass_stats,                .default_value = &(bool){true}},
    { .name = "enable_total_stats",                  .type = COT_BOOL,  .target = &g_Config.enable_total_stats,                  .default_value = &(bool){true}},
    { .name = "enable_timer_in_inventory",           .type = COT_BOOL,  .target = &g_Config.enable_timer_in_inventory,           .default_value = &(bool){true}},
    { .name = "enable_smooth_bars",                  .type = COT_BOOL,  .target = &g_Config.enable_smooth_bars,                  .default_value = &(bool){true}},
    { .name = "enable_fade_effects",                 .type = COT_BOOL,  .target = &g_Config.enable_fade_effects,                 .default_value = &(bool){true}},
    { .name = "fix_tihocan_secret_sound",            .type = COT_BOOL,  .target = &g_Config.fix_tihocan_secret_sound,            .default_value = &(bool){true}},
    { .name = "fix_floor_data_issues",               .type = COT_BOOL,  .target = &g_Config.fix_floor_data_issues,               .default_value = &(bool){true}},
    { .name = "fix_secrets_killing_music",           .type = COT_BOOL,  .target = &g_Config.fix_secrets_killing_music,           .default_value = &(bool){true}},
    { .name = "fix_speeches_killing_music",          .type = COT_BOOL,  .target = &g_Config.fix_speeches_killing_music,          .default_value = &(bool){true}},
    { .name = "fix_descending_glitch",               .type = COT_BOOL,  .target = &g_Config.fix_descending_glitch,               .default_value = &(bool){false}},
    { .name = "fix_wall_jump_glitch",                .type = COT_BOOL,  .target = &g_Config.fix_wall_jump_glitch,                .default_value = &(bool){false}},
    { .name = "fix_bridge_collision",                .type = COT_BOOL,  .target = &g_Config.fix_bridge_collision,                .default_value = &(bool){true}},
    { .name = "fix_qwop_glitch",                     .type = COT_BOOL,  .target = &g_Config.fix_qwop_glitch,                     .default_value = &(bool){false}},
    { .name = "fix_alligator_ai",                    .type = COT_BOOL,  .target = &g_Config.fix_alligator_ai,                    .default_value = &(bool){true}},
    { .name = "change_pierre_spawn",                 .type = COT_BOOL,  .target = &g_Config.change_pierre_spawn,                 .default_value = &(bool){true}},
    { .name = "fix_bear_ai",                         .type = COT_BOOL,  .target = &g_Config.fix_bear_ai,                         .default_value = &(bool){true}},
    { .name = "fov_value",                           .type = COT_INT32, .target = &g_Config.fov_value,                           .default_value = &(int32_t){65}},
    { .name = "resolution_width",                    .type = COT_INT32, .target = &g_Config.resolution_width,                    .default_value = &(int32_t){-1}},
    { .name = "resolution_height",                   .type = COT_INT32, .target = &g_Config.resolution_height,                   .default_value = &(int32_t){-1}},
    { .name = "fov_vertical",                        .type = COT_BOOL,  .target = &g_Config.fov_vertical,                        .default_value = &(bool){true}},
    { .name = "enable_demo",                         .type = COT_BOOL,  .target = &g_Config.enable_demo,                         .default_value = &(bool){true}},
    { .name = "enable_fmv",                          .type = COT_BOOL,  .target = &g_Config.enable_fmv,                          .default_value = &(bool){true}},
    { .name = "enable_cine",                         .type = COT_BOOL,  .target = &g_Config.enable_cine,                         .default_value = &(bool){true}},
    { .name = "enable_music_in_menu",                .type = COT_BOOL,  .target = &g_Config.enable_music_in_menu,                .default_value = &(bool){true}},
    { .name = "enable_music_in_inventory",           .type = COT_BOOL,  .target = &g_Config.enable_music_in_inventory,           .default_value = &(bool){true}},
    { .name = "enable_round_shadow",                 .type = COT_BOOL,  .target = &g_Config.enable_round_shadow,                 .default_value = &(bool){true}},
    { .name = "enable_3d_pickups",                   .type = COT_BOOL,  .target = &g_Config.enable_3d_pickups,                   .default_value = &(bool){true}},
    { .name = "rendering.anisotropy_filter",         .type = COT_FLOAT, .target = &g_Config.rendering.anisotropy_filter,         .default_value = &(float){16.0f}},
    { .name = "walk_to_items",                       .type = COT_BOOL,  .target = &g_Config.walk_to_items,                       .default_value = &(bool){false}},
    { .name = "disable_trex_collision",              .type = COT_BOOL,  .target = &g_Config.disable_trex_collision,              .default_value = &(bool){false}},
    { .name = "start_lara_hitpoints",                .type = COT_INT32, .target = &g_Config.start_lara_hitpoints,                .default_value = &(int32_t){LARA_HITPOINTS}},
    { .name = "healthbar_showing_mode",              .type = COT_ENUM,  .target = &g_Config.healthbar_showing_mode,              .default_value = &(int32_t){BSM_FLASHING_OR_DEFAULT}, .param = m_BarShowingModes},
    { .name = "airbar_showing_mode",                 .type = COT_ENUM,  .target = &g_Config.airbar_showing_mode,                 .default_value = &(int32_t){BSM_DEFAULT},             .param = m_BarShowingModes},
    { .name = "healthbar_location",                  .type = COT_ENUM,  .target = &g_Config.healthbar_location,                  .default_value = &(int32_t){BL_TOP_LEFT},             .param = m_BarLocations},
    { .name = "airbar_location",                     .type = COT_ENUM,  .target = &g_Config.airbar_location,                     .default_value = &(int32_t){BL_TOP_RIGHT},            .param = m_BarLocations},
    { .name = "enemy_healthbar_location",            .type = COT_ENUM,  .target = &g_Config.enemy_healthbar_location,            .default_value = &(int32_t){BL_BOTTOM_LEFT},          .param = m_BarLocations},
    { .name = "healthbar_color",                     .type = COT_ENUM,  .target = &g_Config.healthbar_color,                     .default_value = &(int32_t){BC_RED},                  .param = m_BarColors},
    { .name = "airbar_color",                        .type = COT_ENUM,  .target = &g_Config.airbar_color,                        .default_value = &(int32_t){BC_BLUE},                 .param = m_BarColors},
    { .name = "enemy_healthbar_color",               .type = COT_ENUM,  .target = &g_Config.enemy_healthbar_color,               .default_value = &(int32_t){BC_GREY},                 .param = m_BarColors},
    { .name = "screenshot_format",                   .type = COT_ENUM,  .target = &g_Config.screenshot_format,                   .default_value = &(int32_t){SCREENSHOT_FORMAT_JPEG},  .param = m_ScreenshotFormats},
    { .name = "ui.menu_style",                       .type = COT_ENUM,  .target = &g_Config.ui.menu_style,                       .default_value = &(int32_t){UI_STYLE_PC},             .param = m_UIStyles},
    { .name = "maximum_save_slots",                  .type = COT_INT32, .target = &g_Config.maximum_save_slots,                  .default_value = &(int32_t){25}},
    { .name = "revert_to_pistols",                   .type = COT_BOOL,  .target = &g_Config.revert_to_pistols,                   .default_value = &(bool){false}},
    { .name = "enable_enhanced_saves",               .type = COT_BOOL,  .target = &g_Config.enable_enhanced_saves,               .default_value = &(bool){true}},
    { .name = "enable_pitched_sounds",               .type = COT_BOOL,  .target = &g_Config.enable_pitched_sounds,               .default_value = &(bool){true}},
    { .name = "enable_ps_uzi_sfx",                   .type = COT_BOOL,  .target = &g_Config.enable_ps_uzi_sfx,                   .default_value = &(bool){false}},
    { .name = "enable_jump_twists",                  .type = COT_BOOL,  .target = &g_Config.enable_jump_twists,                  .default_value = &(bool){true}},
    { .name = "enabled_inverted_look",               .type = COT_BOOL,  .target = &g_Config.enabled_inverted_look,               .default_value = &(bool){false}},
    { .name = "camera_speed",                        .type = COT_INT32, .target = &g_Config.camera_speed,                        .default_value = &(int32_t){5}},
    { .name = "fix_texture_issues",                  .type = COT_BOOL,  .target = &g_Config.fix_texture_issues,                  .default_value = &(bool){true}},
    { .name = "enable_swing_cancel",                 .type = COT_BOOL,  .target = &g_Config.enable_swing_cancel,                 .default_value = &(bool){true}},
    { .name = "enable_tr2_jumping",                  .type = COT_BOOL,  .target = &g_Config.enable_tr2_jumping,                  .default_value = &(bool){false}},
    { .name = "load_current_music",                  .type = COT_BOOL,  .target = &g_Config.load_current_music,                  .default_value = &(bool){true}},
    { .name = "load_music_triggers",                 .type = COT_BOOL,  .target = &g_Config.load_music_triggers,                 .default_value = &(bool){true}},
    { .name = "fix_item_rots",                       .type = COT_BOOL,  .target = &g_Config.fix_item_rots,                       .default_value = &(bool){true}},
    { .name = "restore_ps1_enemies",                 .type = COT_BOOL,  .target = &g_Config.restore_ps1_enemies,                 .default_value = &(bool){false}},
    { .name = "enable_game_modes",                   .type = COT_BOOL,  .target = &g_Config.enable_game_modes,                   .default_value = &(bool){true}},
    { .name = "enable_save_crystals",                .type = COT_BOOL,  .target = &g_Config.enable_save_crystals,                .default_value = &(bool){false}},
    { .name = "enable_uw_roll",                      .type = COT_BOOL,  .target = &g_Config.enable_uw_roll,                      .default_value = &(bool){true}},
    { .name = "enable_buffering",                    .type = COT_BOOL,  .target = &g_Config.enable_buffering,                    .default_value = &(bool){false}},
    { .name = "enable_lean_jumping",                 .type = COT_BOOL,  .target = &g_Config.enable_lean_jumping,                 .default_value = &(bool){false}},
    { .name = "rendering.render_mode",               .type = COT_INT32, .target = &g_Config.rendering.render_mode,               .default_value = &(int32_t){GFX_RM_LEGACY}},
    { .name = "rendering.enable_fullscreen",         .type = COT_BOOL,  .target = &g_Config.rendering.enable_fullscreen,         .default_value = &(bool){true}},
    { .name = "rendering.enable_maximized",          .type = COT_BOOL,  .target = &g_Config.rendering.enable_maximized,          .default_value = &(bool){true}},
    { .name = "rendering.window_x",                  .type = COT_INT32, .target = &g_Config.rendering.window_x,                  .default_value = &(int32_t){-1}},
    { .name = "rendering.window_y",                  .type = COT_INT32, .target = &g_Config.rendering.window_y,                  .default_value = &(int32_t){-1}},
    { .name = "rendering.window_width",              .type = COT_INT32, .target = &g_Config.rendering.window_width,              .default_value = &(int32_t){-1}},
    { .name = "rendering.window_height",             .type = COT_INT32, .target = &g_Config.rendering.window_height,             .default_value = &(int32_t){-1}},
    { .name = "rendering.texture_filter",            .type = COT_INT32, .target = &g_Config.rendering.texture_filter,            .default_value = &(int32_t){GFX_TF_BILINEAR}},
    { .name = "rendering.fbo_filter",                .type = COT_INT32, .target = &g_Config.rendering.fbo_filter,                .default_value = &(int32_t){GFX_TF_NN}},
    { .name = "rendering.enable_perspective_filter", .type = COT_BOOL,  .target = &g_Config.rendering.enable_perspective_filter, .default_value = &(bool){true}},
    { .name = "rendering.enable_vsync",              .type = COT_BOOL,  .target = &g_Config.rendering.enable_vsync,              .default_value = &(bool){true}},
    { .name = "music_volume",                        .type = COT_INT32, .target = &g_Config.music_volume,                        .default_value = &(int32_t){8}},
    { .name = "sound_volume",                        .type = COT_INT32, .target = &g_Config.sound_volume,                        .default_value = &(int32_t){8}},
    { .name = "input.layout",                        .type = COT_INT32, .target = &g_Config.input.layout,                        .default_value = &(int32_t){0}},
    { .name = "input.cntlr_layout",                  .type = COT_INT32, .target = &g_Config.input.cntlr_layout,                  .default_value = &(int32_t){0}},
    { .name = "brightness",                          .type = COT_FLOAT, .target = &g_Config.brightness,                          .default_value = &(float){DEFAULT_BRIGHTNESS}},
    { .name = "ui.text_scale",                       .type = COT_DOUBLE, .target = &g_Config.ui.text_scale,                       .default_value = &(double){DEFAULT_UI_SCALE}},
    { .name = "ui.bar_scale",                        .type = COT_DOUBLE, .target = &g_Config.ui.bar_scale,                        .default_value = &(double){DEFAULT_UI_SCALE}},
    { .name = "profile.new_game_plus_unlock",        .type = COT_BOOL,  .target = &g_Config.profile.new_game_plus_unlock,        .default_value = &(bool){false}},
    // clang-format on

    // guard
    {
        .name = NULL,
        .target = NULL,
    },
};
