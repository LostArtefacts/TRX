#include "game/ui/dialogs/setting_helpers/handlers.h"

#include "config.h"
#include "game/music.h"
#include "game/sound.h"

#if TR_VERSION == 1
bool UI_Settings_EnablePS1Crystals_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.enable_save_crystals;
}
#endif

bool UI_Settings_EnableExitFadeEffects_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_fade_effects;
}

bool UI_Settings_FogColor_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return !g_Config.visuals.fog_transparency;
}

bool UI_Settings_EnableBreeze_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_braid;
}

bool UI_Settings_Sprint_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.enable_sprint;
}

bool UI_Settings_Healthbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.lara_health_bar.show_mode != BSM_NEVER;
}

bool UI_Settings_Airbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.lara_air_bar.show_mode != BSM_NEVER;
}

bool UI_Settings_Sprintbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_Sprint_IsAvailable(option)
        && g_Config.ui.lara_sprint_bar.show_mode != BSM_NEVER;
}

bool UI_Settings_Exposurebar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.lara_exposure_bar.show_mode != BSM_NEVER;
}

bool UI_Settings_EnemyHealthbar_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.enemy_health_bar.show_mode != BSM_NEVER;
}

bool UI_Settings_AllyHealthbar_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
#if TR_VERSION == 1
    return g_Config.ui.enemy_health_bar.show_mode == BSM_ALWAYS;
#else
    return g_Config.ui.enemy_health_bar.show_mode == BSM_ALWAYS
        && g_Config.gameplay.enable_ally_targeting;
#endif
}

bool UI_Settings_HealthbarColor_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_Healthbar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_AirbarColor_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_Airbar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_SprintbarColor_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_Sprintbar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_ExposurebarColor_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_Exposurebar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_EnemyHealthbarColor_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_EnemyHealthbar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_AllyHealthbarColor_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return UI_Settings_AllyHealthbar_IsAvailable(option)
        && g_Config.ui.bar_look != BAR_LOOK_TR23_PS1;
}

bool UI_Settings_IdlePose_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.idle_pose_timeout > 0;
}

bool UI_Settings_FixItemRots_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_3d_pickups;
}

bool UI_Settings_FixStepGlitch_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.enable_smooth_wall_deflect;
}

bool UI_Settings_FixWadeWallHit_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
#if TR_VERSION == 1
    return g_Config.gameplay.enable_wading;
#else
    return true;
#endif
}

bool UI_Settings_PauseMusicInInventory_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.audio.enable_music_in_inventory;
}

bool UI_Settings_Volume_RequestChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    UI_Settings_RequestChange(option, dir);
    if (option->target == &g_Config.audio.music_volume) {
        Music_SetVolume(g_Config.audio.music_volume);
    } else if (option->target == &g_Config.audio.sound_volume) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    return true;
}
