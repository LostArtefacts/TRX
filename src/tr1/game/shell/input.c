#include "game/console/common.h"

#include <libtrx/config.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/shell/input.h>
#include <libtrx/gfx/context.h>

static void M_CycleLightingContrast(void)
{
    const int32_t direction = g_Input.slow ? -1 : 1;
    LIGHTING_CONTRAST value = g_Config.rendering.lighting_contrast;
    value += direction;
    value += LIGHTING_CONTRAST_NUMBER_OF;
    value %= LIGHTING_CONTRAST_NUMBER_OF;
    g_Config.rendering.lighting_contrast = value;
    Config_Update();
    Console_Log(
        GS(OSD_LIGHTING_CONTRAST_FMT),
        ENUM_MAP_TO_STRING(LIGHTING_CONTRAST, value));
}

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

        Config_Update();
    }

    if (g_InputDB.toggle_trapezoid_filter) {
        g_Config.rendering.enable_trapezoid_filter ^= true;
        Console_Log(
            g_Config.rendering.enable_trapezoid_filter
                ? GS(OSD_TRAPEZOID_FILTER_ON)
                : GS(OSD_TRAPEZOID_FILTER_OFF));
        Config_Update();
    }

    if (g_InputDB.toggle_fps_counter) {
        g_Config.ui.enable_fps_counter ^= true;
        Console_Log(
            g_Config.ui.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                           : GS(OSD_FPS_COUNTER_OFF));
        Config_Update();
    }

    if (g_InputDB.cycle_lighting_contrast) {
        M_CycleLightingContrast();
    }
}
