#include "game/shell/input.h"

#include "config.h"
#include "enum_map.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/console/common.h"
#include "game/viewport.h"
#include "gfx/context.h"
#include "utils.h"

static void M_ToggleFullscreen(void)
{
    TOGGLE(g_Config.window.is_fullscreen);
    Config_Update();
}

static void M_ToggleFPSCounter(void)
{
    TOGGLE(g_Config.ui.enable_fps_counter);
    Config_Update();
    Console_Log(
        "%s",
        g_Config.ui.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                       : GS(OSD_FPS_COUNTER_OFF));
}

static void M_ToggleBilinearFilter(void)
{
    CYCLE(g_Config.rendering.texture_filter, 1, GFX_TF_NUMBER_OF);
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.texture_filter == GFX_TF_BILINEAR
            ? GS(OSD_BILINEAR_FILTER_ON)
            : GS(OSD_BILINEAR_FILTER_OFF));
}

static void M_ToggleTrapezoidFilter(void)
{
    TOGGLE(g_Config.rendering.enable_trapezoid_filter);
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.enable_trapezoid_filter
            ? GS(OSD_TRAPEZOID_FILTER_ON)
            : GS(OSD_TRAPEZOID_FILTER_OFF));
}

static void M_ToggleWireframe(void)
{
    TOGGLE(g_Config.rendering.enable_wireframe);
    Config_Update();
    Console_Log(
        "%s",
        g_Config.rendering.enable_wireframe ? GS(OSD_WIREFRAME_MODE_ON)
                                            : GS(OSD_WIREFRAME_MODE_OFF));
}

static void M_CycleLightingContrast(void)
{
    CYCLE(
        g_Config.rendering.lighting_contrast, g_Input.slow ? -1 : 1,
        LIGHTING_CONTRAST_NUMBER_OF);
    Config_Update();
    Console_Log(
        GS(OSD_LIGHTING_CONTRAST_FMT),
        ENUM_MAP_TO_STRING(
            LIGHTING_CONTRAST, g_Config.rendering.lighting_contrast));
}

static void M_CycleUpscalingFactor(void)
{
    g_Config.rendering.upscaling_factor += g_Input.slow ? -1 : 1;
    Config_Update();
    Console_Log(GS(OSD_UPSCALING_FACTOR), g_Config.rendering.upscaling_factor);
}

static void M_CycleBorders(void)
{
    if (g_Input.slow) {
        if (g_Config.rendering.borders > 0.0) {
            g_Config.rendering.borders -= 0.05;
            CLAMPL(g_Config.rendering.borders, 0.0);
            Viewport_Reset();
        }
    } else {
        if (g_Config.rendering.borders < 0.45) {
            g_Config.rendering.borders += 0.05;
            CLAMPG(g_Config.rendering.borders, 0.45);
            Viewport_Reset();
        }
    }
}

void Shell_ProcessInput(void)
{
    if (g_InputDB.screenshot) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }

    if (g_InputDB.toggle_fullscreen) {
        M_ToggleFullscreen();
    }
    if (g_InputDB.toggle_fps_counter) {
        M_ToggleFPSCounter();
    }
    if (g_InputDB.toggle_bilinear_filter) {
        M_ToggleBilinearFilter();
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
    if (g_InputDB.switch_upscaling) {
        M_CycleUpscalingFactor();
    }
    if (g_InputDB.switch_borders) {
        M_CycleBorders();
    }

    if (g_InputDB.turbo_cheat && g_Config.gameplay.enable_cheats) {
        Clock_CycleTurboSpeed(!g_Input.slow);
    }
}
