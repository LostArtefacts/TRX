#include "decomp/decomp.h"
#include "game/console/common.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/shell/input.h>
#include <libtrx/screenshot.h>
#include <libtrx/utils.h>

static void M_ToggleFPSCounter(void);
static void M_ToggleBilinearFiltering(void);
static void M_ToggleTrapezoidFilter(void);
static void M_ToggleWireframe(void);
static void M_CycleLightingContrast(void);

static void M_ToggleFPSCounter(void)
{
    g_Config.ui.enable_fps_counter = !g_Config.ui.enable_fps_counter;
    Config_Update();
    Console_Log(
        "%s",
        g_Config.ui.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                       : GS(OSD_FPS_COUNTER_OFF));
}

static void M_ToggleBilinearFiltering(void)
{
    g_Config.rendering.texture_filter =
        g_Config.rendering.texture_filter == GFX_TF_BILINEAR ? GFX_TF_NN
                                                             : GFX_TF_BILINEAR;
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.texture_filter == GFX_TF_BILINEAR
            ? GS(OSD_BILINEAR_FILTER_ON)
            : GS(OSD_BILINEAR_FILTER_OFF));
}

static void M_ToggleTrapezoidFilter(void)
{
    g_Config.rendering.enable_trapezoid_filter =
        !g_Config.rendering.enable_trapezoid_filter;
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.enable_trapezoid_filter
            ? GS(OSD_TRAPEZOID_FILTER_ON)
            : GS(OSD_TRAPEZOID_FILTER_OFF));
}

static void M_ToggleWireframe(void)
{
    g_Config.rendering.enable_wireframe = !g_Config.rendering.enable_wireframe;
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.enable_wireframe ? GS(OSD_WIREFRAME_MODE_ON)
                                            : GS(OSD_WIREFRAME_MODE_OFF));
}

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

    if (g_InputDB.toggle_fps_counter) {
        M_ToggleFPSCounter();
    }

    if (g_InputDB.toggle_bilinear_filter) {
        M_ToggleBilinearFiltering();
    }

    if (g_InputDB.toggle_trapezoid_filter) {
        M_ToggleTrapezoidFilter();
    }

    if (g_InputDB.toggle_wireframe) {
        M_ToggleWireframe();
    }

    if (g_InputDB.cycle_lighting_contrast) {
        M_CycleLightingContrast();
    }
}
