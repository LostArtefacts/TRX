#include "game/console/common.h"

#include <libtrx/config.h>
#include <libtrx/game/shell/input.h>
#include <libtrx/gfx/context.h>

void Shell_ProcessInput(void)
{
    Shell_ProcessCommonInput();

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
}
