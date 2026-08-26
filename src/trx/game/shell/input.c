#include <trx/game/shell/input.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/enum_map.h>
#include <trx/core/utils.h>
#include <trx/game/clock.h>
#include <trx/game/console.h>
#include <trx/game/console/common.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/screenshot.h>
#include <trx/game/viewport.h>
#include <trx/gl/context.h>

static void M_ToggleFullscreen(void)
{
    CONFIG_TOGGLE(g_Config.window.is_fullscreen);
    Config_Update();
}

static void M_ToggleFPSCounter(void)
{
    CONFIG_TOGGLE(g_Config.ui.enable_fps_counter);
    Config_Update();
    Console_Info(
        "%s",
        g_Config.ui.enable_fps_counter ? GS("general/osd/fps_counter_on")
                                       : GS("general/osd/fps_counter_off"));
}

static void M_ToggleBilinearFilter(void)
{
    CONFIG_CYCLE(
        g_Config.rendering.texture_filter, 1, TEXTURE_FILTER_NUMBER_OF);
    Config_Update();
    Console_Info(
        "%s",
        g_Config.rendering.texture_filter == TEXTURE_FILTER_BILINEAR
            ? GS("general/osd/bilinear_filter_on")
            : GS("general/osd/bilinear_filter_off"));
}

static void M_ToggleTrapezoidFilter(void)
{
    CONFIG_TOGGLE(g_Config.rendering.enable_trapezoid_filter);
    Config_Update();
    Console_Info(
        "%s",
        g_Config.rendering.enable_trapezoid_filter
            ? GS("general/osd/trapezoid_filter_on")
            : GS("general/osd/trapezoid_filter_off"));
}

static void M_ToggleWireframe(void)
{
    CONFIG_TOGGLE(g_Config.rendering.enable_wireframe);
    Config_Update();
    Console_Info(
        "%s",
        g_Config.rendering.enable_wireframe
            ? GS("general/osd/wireframe_mode_on")
            : GS("general/osd/wireframe_mode_off"));
}

static void M_ToggleTextures(void)
{
    CONFIG_TOGGLE(g_Config.rendering.enable_textures);
    Config_Update();
    Console_Info(
        "%s",
        g_Config.rendering.enable_textures ? GS("general/osd/textures_on")
                                           : GS("general/osd/textures_off"));
}

static void M_CycleLightingModel(void)
{
    const int32_t dir = g_Input.slow ? -1 : 1;
    if (Config_FindOptionByMirror(&g_Config.rendering.lighting_curve)
        != nullptr) {
        CONFIG_CYCLE(
            g_Config.rendering.lighting_curve, dir, LIGHTING_CURVE_NUMBER_OF);
        Config_Update();
        Console_Info(
            GS("general/osd/lighting_curve_fmt"),
            ENUM_MAP_TO_STRING(
                LIGHTING_CURVE, g_Config.rendering.lighting_curve));
    } else if (
        Config_FindOptionByMirror(&g_Config.rendering.lighting_contrast)
        != nullptr) {
        CONFIG_CYCLE(
            g_Config.rendering.lighting_contrast, dir,
            LIGHTING_CONTRAST_NUMBER_OF);
        Config_Update();
        Console_Info(
            GS("general/osd/lighting_contrast_fmt"),
            ENUM_MAP_TO_STRING(
                LIGHTING_CONTRAST, g_Config.rendering.lighting_contrast));
    }
}

static void M_CycleUpscalingFactor(void)
{
    CONFIG_SET(
        g_Config.rendering.upscaling_factor,
        g_Config.rendering.upscaling_factor + (g_Input.slow ? -1 : 1));
    Config_Update();
    Console_Info(
        GS("general/osd/upscaling_factor"),
        g_Config.rendering.upscaling_factor);
}

static void M_CycleBorders(void)
{
    if (g_Input.slow) {
        if (g_Config.rendering.borders > 0.0) {
            CONFIG_SET(
                g_Config.rendering.borders,
                MAX(g_Config.rendering.borders - 0.05, 0.0));
            Viewport_Reset();
        }
    } else {
        if (g_Config.rendering.borders < 0.45) {
            CONFIG_SET(
                g_Config.rendering.borders,
                MIN(g_Config.rendering.borders + 0.05, 0.45));
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
    if (g_InputDB.toggle_textures) {
        M_ToggleTextures();
    }
    if (g_InputDB.cycle_lighting_model) {
        M_CycleLightingModel();
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

    if (g_Config.gameplay.enable_cheats
        && (g_Input.fast_forward_cheat || g_Input.slow_motion_cheat)) {
        Clock_HoldTurboSpeed(
            g_Input.fast_forward_cheat ? CLOCK_TURBO_SPEED_MAX
                                       : CLOCK_TURBO_SPEED_MIN);
    } else {
        Clock_ReleaseTurboSpeed();
    }

    if (g_InputDB.change_outfit) {
        Lara_Skin_CycleOutfit(g_Input.slow ? -1 : 1);
    }
}
