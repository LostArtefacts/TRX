#include "game/lara/const.h"
#if TR_VERSION == 1
    #include "priv_tr1.c"
#elif TR_VERSION == 2
    #include "priv_tr2.c"
#endif

void Config_SanitizeCommon(void)
{
    CLAMP(g_Config.audio.master_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.sound_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_music_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.inventory_ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.underwater_ambient_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.cutscene_volume, 0.0f, 1.0f);
    CLAMP(g_Config.audio.fmv_volume, 0.0f, 1.0f);
    CLAMP(g_Config.input.keyboard_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(g_Config.input.controller_layout, 0, INPUT_LAYOUT_NUMBER_OF - 1);
    CLAMP(
        g_Config.gameplay.turbo_speed, CLOCK_TURBO_SPEED_MIN,
        CLOCK_TURBO_SPEED_MAX);
    CLAMP(g_Config.gameplay.start_lara_hitpoints, 1, LARA_MAX_HITPOINTS);
    CLAMP(g_Config.gameplay.camera_speed, 1, 10);
    CLAMP(g_Config.gameplay.idle_pose_timeout, 0, 1200);
    CLAMP(g_Config.rendering.wireframe_width, 1.0, 100.0);
    CLAMP(g_Config.rendering.upscaling_factor, 1, 8);
    CLAMP(g_Config.rendering.borders, 0.0, 0.45);
    CLAMP(g_Config.ui.bar_scale, 0.5, 2.0);
    CLAMP(g_Config.ui.text_scale, 0.5, 2.0);
    CLAMP(g_Config.ui.pickup_scale, 0.5, 2.0);
    CLAMP(g_Config.visuals.fog_start, 1, 100);
    CLAMP(g_Config.visuals.fog_end, 1, 100);

    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        g_Config.rendering.fps = 30;
    }

    CLAMP(
        g_Config.visuals.brightness, CONFIG_MIN_BRIGHTNESS,
        CONFIG_MAX_BRIGHTNESS);
    CLAMPL(g_Config.rendering.anisotropy_filter, 1.0);
}
