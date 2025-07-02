#include "game/clock.h"
#include "game/console/common.h"

#include <libtrx/config.h>
#include <libtrx/gfx/context.h>

static void M_ToggleFullscreen(void)
{
    g_Config.window.is_fullscreen = !g_Config.window.is_fullscreen;
    Config_Write();
}

void Shell_ProcessInput(void)
{
    if (g_InputDB.screenshot) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }

    if (g_InputDB.toggle_bilinear_filter) {
        g_Config.rendering.texture_filter =
            (g_Config.rendering.texture_filter + 1) % GFX_TF_NUMBER_OF;

        switch (g_Config.rendering.texture_filter) {
        case GFX_TF_NN:
            Console_Log(GS(OSD_TEXTURE_FILTER_SET), GS(OSD_TEXTURE_FILTER_NN));
            break;
        case GFX_TF_BILINEAR:
            Console_Log(
                GS(OSD_TEXTURE_FILTER_SET), GS(OSD_TEXTURE_FILTER_BILINEAR));
            break;
        case GFX_TF_NUMBER_OF:
            break;
        }

        Config_Write();
    }

    if (g_InputDB.toggle_trapezoid_filter) {
        g_Config.rendering.enable_trapezoid_filter ^= true;
        Console_Log(
            g_Config.rendering.enable_trapezoid_filter
                ? GS(OSD_TRAPEZOID_FILTER_ON)
                : GS(OSD_TRAPEZOID_FILTER_OFF));
        Config_Write();
    }

    if (g_InputDB.toggle_fps_counter) {
        g_Config.ui.enable_fps_counter ^= true;
        Console_Log(
            g_Config.ui.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                           : GS(OSD_FPS_COUNTER_OFF));
        Config_Write();
    }

    if (g_InputDB.toggle_fullscreen) {
        M_ToggleFullscreen();
    }

    if (g_InputDB.turbo_cheat) {
        Clock_CycleTurboSpeed(!g_Input.slow);
    }
}
