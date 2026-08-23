#include <trx/config.h>
#include <trx/core/strings.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/gun.h>
#include <trx/game/input/backends/touch.h>
#include <trx/game/music.h>
#include <trx/game/objects/common.h>
#include <trx/game/shell/common.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/game/ui/dialogs/settings_editor.h>
#include <trx/game/ui/dialogs/settings_handlers.h>
#include <trx/game/ui/settings.h>
#include <trx/gl/enum.h>
#include <trx/version.h>

#include <string.h>

// The few enums the menu shows in an order other than the one they were
// defined in - because the values read better sorted, or because the order the
// rest of the engine wants is not the order a player wants.
static const int32_t m_AspectModeOrder[] = {
    ASPECT_MODE_4_3, ASPECT_MODE_16_9, ASPECT_MODE_16_10, ASPECT_MODE_ANY, -1,
};

static const int32_t m_AllyHostilityPolicyOrder[] = {
    ALLY_HOSTILITY_POLICY_INDIVIDUAL,
    ALLY_HOSTILITY_POLICY_SHARED,
    -1,
};

static const int32_t m_BarShowModeOrder[] = {
    BAR_SHOW_MODE_NEVER,
    BAR_SHOW_MODE_BOSS_ONLY,
    BAR_SHOW_MODE_ALWAYS,
    -1,
};

static const int32_t m_TextureFilterOrder[] = {
    TEXTURE_FILTER_POINT,
    TEXTURE_FILTER_BILINEAR,
    -1,
};

// BK_IMAGE sits here but is never offered - see
// M_BackgroundStyle_IsEnumValueAvailable. It is listed so that a
// config file naming it still has a row to show.
static const int32_t m_BackgroundStyleOrder[] = {
    // clang-format off
    BK_NONE,
    BK_TRANSPARENT_MEDIUM,
    BK_TRANSPARENT_DARK,
    BK_BLACK,
    BK_PATTERN_STATIC,
    BK_PATTERN_WAVE,
    BK_IMAGE,
    BK_MONOCHROME,
    BK_MONOCHROME_COOL,
    BK_MONOCHROME_WARM,
    -1,
    // clang-format on
};

static bool M_EnablePS1Crystals_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.save_crystal_mode != SAVE_CRYSTAL_OFF;
}

static bool M_ShowCrystals_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    if (!Stats_GameHasCrystals()) {
        return false;
    }
    // Pickup mode counts crystals and nothing else, so it shows the row
    // regardless of this setting.
    const SAVE_CRYSTAL_MODE mode = g_Config.gameplay.save_crystal_mode;
    if (mode == SAVE_CRYSTAL_OFF || mode == SAVE_CRYSTAL_PICKUP) {
        return false;
    }
    return true;
}

static bool M_EnableFadeEffects_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.visuals.enable_fade_effects;
}

static bool M_FogColor_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return !g_Config.visuals.fog_transparency;
}

static bool M_FogBulbs_IsVisible(
    const CONFIG_OPTION *const option, void *const user_data)
{
    // Fog bulbs are a TR4 feature; older games have no such config option.
    return g_TRVersion == 4;
}

static bool M_GunGlow_IsVisible(
    const CONFIG_OPTION *const option, void *const user_data)
{
    // TR3/4 always draw the gun glow; only TR1/2 have it as optional.
    return g_TRVersion == 1 || g_TRVersion == 2;
}

// The two ways of anti-aliasing the scene are alternatives: one rasterizes
// more pixels, the other more samples of the same pixel, and asking for both
// only costs what the one that wins would have.
static bool M_Supersampling_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.rendering.multisampling_factor <= 1;
}

static bool M_Multisampling_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.rendering.supersampling_factor <= 1;
}

static bool M_EnableBreeze_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.visuals.enable_braid || g_Config.visuals.enable_weather
        || g_Config.visuals.enable_droplets;
}

static bool M_ResponsiveJumping_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.enable_tr2_jumping;
}

static bool M_Crawl_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.enable_crawling;
}

static bool M_Sprint_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.enable_sprint;
}

static bool M_Bar_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.ui.show_bars;
}

static bool M_Sprintbar_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return M_Sprint_IsAvailable(option, user_data)
        && M_Bar_IsAvailable(option, user_data);
}

static bool M_EnemyHealthbar_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return M_Bar_IsAvailable(option, user_data)
        && g_Config.ui.enemy_health_bar.show_mode != BAR_SHOW_MODE_NEVER;
}

static bool M_AllyHealthbar_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return M_Bar_IsAvailable(option, user_data)
        && g_Config.ui.enemy_health_bar.show_mode == BAR_SHOW_MODE_ALWAYS
        && g_Config.gameplay.enable_ally_targeting;
}

static bool M_BarColorPC_IsVisible(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return !UI_Settings_IsCurrentBarLookPS1();
}

static bool M_BarColorPS1_IsVisible(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return UI_Settings_IsCurrentBarLookPS1();
}

static bool M_IdlePose_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.idle_pose_timeout > 0;
}

static const char *M_ColorEditor_FormatValue(
    const CONFIG_OPTION *const option, void *const user_data)
{
    const RGB_888 color = option->value.as_rgb;
    return String_FormatStatic("#%02X%02X%02X", color.r, color.g, color.b);
}

static bool M_ColorEditor_CanChangeValue(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    return false;
}

static bool M_FixStepGlitch_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.enable_smooth_wall_deflect;
}

static bool M_FixWadeWallHit_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.gameplay.enable_wading;
}

static bool M_PauseMusicInInventory_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return g_Config.audio.enable_music_in_inventory;
}

static bool M_BackgroundStyle_IsEnumValueAvailable(
    const CONFIG_OPTION *const option, const int32_t value,
    void *const user_data)
{
    if (value == BK_PATTERN_STATIC || value == BK_PATTERN_WAVE) {
        return Object_Get(O_INV_BACKGROUND)->loaded;
    }
    // A level's loading image is not a backdrop the menu offers.
    return value != BK_IMAGE;
}

static bool M_ShadowType_IsEnumValueAvailable(
    const CONFIG_OPTION *const option, const int32_t value,
    void *const user_data)
{
    if (value == SHADOW_TYPE_SPRITE) {
        return Object_Get(O_SHADOW)->loaded;
    }
    return true;
}

static bool M_Volume_RequestChange(
    CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    UI_SettingsEditor_RequestChange(option, dir);
    if (strcmp(option->name, "audio.music_volume") == 0) {
        Music_SetVolume(g_Config.audio.music_volume);
    } else if (strcmp(option->name, "audio.sound_volume") == 0) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    return true;
}

static bool M_Flare_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Gun_Registry_Get(LGT_FLARE)->is_available;
}

static bool M_Grenade_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Gun_Registry_Get(LGT_GRENADE)->is_available;
}

static bool M_Harpoon_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Gun_Registry_Get(LGT_HARPOON)->is_available;
}

static bool M_M16_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Gun_Registry_Get(LGT_M16)->is_available
        || Gun_Registry_Get(LGT_MP5)->is_available;
}

static bool M_ProjectileAreaDamage_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Gun_Registry_Get(LGT_ROCKET)->is_available
        || Gun_Registry_Get(LGT_GRENADE)->is_available;
}

static bool M_TouchControls_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Touch_HasHardwareSupport();
}

static bool M_TouchControls_CanChange(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    return Touch_HasHardwareSupport();
}

static bool M_TouchOption_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return Touch_HasHardwareSupport() && g_Config.input.enable_touch_controls;
}

static bool M_TouchOption_CanChange(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    return Touch_HasHardwareSupport() && g_Config.input.enable_touch_controls;
}

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.text_scale", .delta_slow = 1, .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.pickup_scale", .delta_slow = 1, .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.fog_start", .delta_slow = 1, .delta_fast = 10)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.fog_end", .delta_slow = 1, .delta_fast = 10)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.fov", .delta_slow = 1, .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.start_lara_hitpoints", .delta_slow = 10,
        .delta_fast = 100)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.maximum_save_slots", .delta_slow = 1, .delta_fast = 10)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.maximum_quick_save_slots", .delta_slow = 1,
        .delta_fast = 10)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.fps", .delta_slow = 30, .delta_fast = 30)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.anisotropy_filter", .delta_slow = 10,
        .delta_fast = 100)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.game_brightness", .delta_slow = 1, .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.ui_brightness", .delta_slow = 1, .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.background_brightness", .delta_slow = 1,
        .delta_fast = 5)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.gamma", .delta_slow = 10, .delta_fast = 50)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.upscaling_factor", .delta_slow = 1, .delta_fast = 1)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.supersampling_factor", .delta_slow = 1,
        .delta_fast = 1, .is_available = M_Supersampling_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.multisampling_factor", .delta_slow = 1,
        .delta_fast = 1, .is_available = M_Multisampling_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.borders", .delta_slow = 1, .delta_fast = 5)
REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.aspect_mode", .enum_order = m_AspectModeOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.texture_filter", .enum_order = m_TextureFilterOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.ui_filter", .enum_order = m_TextureFilterOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.upscaling_filter", .enum_order = m_TextureFilterOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "rendering.fmv_filter", .enum_order = m_TextureFilterOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.ally_hostility_policy",
        .enum_order = m_AllyHostilityPolicyOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_idle_pose_camera",
        .is_available = M_IdlePose_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.jump_lock_mode",
        .is_available = M_ResponsiveJumping_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_responsive_crawl",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_crawl_jump",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_crawl_tilt",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_crouch_roll",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_toggle_crouch",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_responsive_sprint",
        .is_available = M_Sprint_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_toggle_sprint",
        .is_available = M_Sprint_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "input.enable_touch_controls",
        .can_change_value = M_TouchControls_CanChange,
        .is_available = M_TouchControls_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "input.touch_opacity",
        .can_change_value = M_TouchOption_CanChange,
        .is_available = M_TouchOption_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "input.touch_button_scale",
        .can_change_value = M_TouchOption_CanChange,
        .is_available = M_TouchOption_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "input.touch_dpad_scale",
        .can_change_value = M_TouchOption_CanChange,
        .is_available = M_TouchOption_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "input.touch_dpad_deadzone",
        .can_change_value = M_TouchOption_CanChange,
        .is_available = M_TouchOption_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_flare_throw_priority",
        .is_available = M_Flare_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_m16_accuracy", .is_available = M_M16_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.m16_aim_mode", .is_available = M_M16_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_wade_wall_hit",
        .is_available = M_FixWadeWallHit_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_underwater_crawl",
        .is_available = M_Crawl_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_step_glitch",
        .is_available = M_FixStepGlitch_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.fix_free_flare_glitch",
        .is_available = M_Flare_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.enable_bouncy_grenades",
        .is_available = M_Grenade_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.harpoon_recoil", .delta_slow = 1, .delta_fast = 1,
        .is_available = M_Harpoon_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "debug.enable_endless_flare_time",
        .is_available = M_Flare_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "gameplay.projectile_area_damage",
        .is_available = M_ProjectileAreaDamage_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.inventory_background_style",
        .is_enum_value_available = M_BackgroundStyle_IsEnumValueAvailable,
        .enum_order = m_BackgroundStyleOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.inventory_fade_effects",
        .is_available = M_EnableFadeEffects_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.pause_background_style",
        .is_enum_value_available = M_BackgroundStyle_IsEnumValueAvailable,
        .enum_order = m_BackgroundStyleOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.pause_fade_effects",
        .is_available = M_EnableFadeEffects_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.stats_background_style",
        .is_enum_value_available = M_BackgroundStyle_IsEnumValueAvailable,
        .enum_order = m_BackgroundStyleOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.stats_fade_effects",
        .is_available = M_EnableFadeEffects_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.enable_exit_fade_effects",
        .is_available = M_EnableFadeEffects_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.bar_scale", .delta_slow = 1, .delta_fast = 5,
        .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.bar_look", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enable_smooth_bars", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enable_bar_flashing", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.healthbar_color", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.healthbar_color_ps1", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.healthbar_poison_color", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.healthbar_poison_color_ps1",
        .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.airbar_color", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.airbar_color_ps1", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.sprintbar_color", .is_available = M_Sprintbar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.sprintbar_color_ps1",
        .is_available = M_Sprintbar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.exposurebar_color", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.exposurebar_color_ps1", .is_available = M_Bar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_color",
        .is_available = M_EnemyHealthbar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_color_ps1",
        .is_available = M_EnemyHealthbar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_color_allies",
        .is_available = M_AllyHealthbar_IsAvailable,
        .is_visible = M_BarColorPC_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_color_allies_ps1",
        .is_available = M_AllyHealthbar_IsAvailable,
        .is_visible = M_BarColorPS1_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_show_mode",
        .is_available = M_Bar_IsAvailable, .enum_order = m_BarShowModeOrder)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.healthbar_location", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.airbar_location", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.sprintbar_location", .is_available = M_Sprintbar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.exposurebar_location", .is_available = M_Bar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.enemy_healthbar_location",
        .is_available = M_EnemyHealthbar_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "ui.stats.show_crystals",
        .is_available = M_ShowCrystals_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.fog_color", .format_value = M_ColorEditor_FormatValue,
        .can_change_value = M_ColorEditor_CanChangeValue,
        .is_available = M_FogColor_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.enable_fog_bulbs", .is_visible = M_FogBulbs_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.water_color", .format_value = M_ColorEditor_FormatValue,
        .can_change_value = M_ColorEditor_CanChangeValue)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.breeze_mode",
        .is_available = M_EnableBreeze_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.enable_ps1_crystals",
        .is_available = M_EnablePS1Crystals_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.shadow_type",
        .is_enum_value_available = M_ShadowType_IsEnumValueAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "visuals.enable_gun_glow", .is_visible = M_GunGlow_IsVisible)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.master_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.sound_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.music_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.inventory_music_volume", .delta_slow = 1,
        .delta_fast = 10, .request_change_value = M_Volume_RequestChange,
        .is_available = M_PauseMusicInInventory_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.underwater_music_volume", .delta_slow = 1,
        .delta_fast = 10, .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.ambient_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.inventory_ambient_volume", .delta_slow = 1,
        .delta_fast = 10, .request_change_value = M_Volume_RequestChange,
        .is_available = M_PauseMusicInInventory_IsAvailable)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.underwater_ambient_volume", .delta_slow = 1,
        .delta_fast = 10, .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.cutscene_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.fmv_volume", .delta_slow = 1, .delta_fast = 10,
        .request_change_value = M_Volume_RequestChange)
