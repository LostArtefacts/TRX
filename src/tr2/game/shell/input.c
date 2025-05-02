#include "game/shell/input.h"

#include "decomp/decomp.h"
#include "game/camera.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/overlay.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/enum_map.h>
#include <libtrx/screenshot.h>
#include <libtrx/utils.h>

static void M_ToggleFPSCounter(void);
static void M_ToggleBilinearFiltering(void);
static void M_TogglePerspectiveCorrection(void);
static void M_ToggleTrapezoidFilter(void);
static void M_ToggleZBuffer(void);
static void M_CycleLightingContrast(void);
static void M_ToggleFullscreen(void);
static void M_ToggleRenderingMode(void);
static void M_DecreaseResolutionOrBPP(void);
static void M_IncreaseResolutionOrBPP(void);
static void M_DecreaseInternalScreenSize(void);
static void M_IncreaseInternalScreenSize(void);

static void M_ToggleFPSCounter(void)
{
    g_Config.ui.enable_fps_counter = !g_Config.ui.enable_fps_counter;
    Config_Write();
    Overlay_DisplayModeInfo(
        g_Config.ui.enable_fps_counter ? GS(OSD_FPS_COUNTER_ON)
                                       : GS(OSD_FPS_COUNTER_OFF));
}

static void M_ToggleBilinearFiltering(void)
{
    g_Config.rendering.texture_filter =
        g_Config.rendering.texture_filter == GFX_TF_BILINEAR ? GFX_TF_NN
                                                             : GFX_TF_BILINEAR;
    Config_Write();
    Overlay_DisplayModeInfo(
        g_Config.rendering.texture_filter == GFX_TF_BILINEAR
            ? GS(OSD_BILINEAR_FILTER_ON)
            : GS(OSD_BILINEAR_FILTER_OFF));
}

static void M_TogglePerspectiveCorrection(void)
{
    g_Config.rendering.enable_perspective_filter =
        !g_Config.rendering.enable_perspective_filter;
    g_PerspectiveDistance = g_Config.rendering.enable_perspective_filter
        ? SW_DETAIL_HIGH
        : SW_DETAIL_MEDIUM;
    Config_Write();
    Overlay_DisplayModeInfo(
        g_Config.rendering.enable_perspective_filter
            ? GS(OSD_PERSPECTIVE_FILTER_ON)
            : GS(OSD_PERSPECTIVE_FILTER_OFF));
}

static void M_ToggleTrapezoidFilter(void)
{
    g_Config.rendering.enable_trapezoid_filter =
        !g_Config.rendering.enable_trapezoid_filter;
    Config_Write();
    Overlay_DisplayModeInfo(
        g_Config.rendering.enable_trapezoid_filter
            ? GS(OSD_TRAPEZOID_FILTER_ON)
            : GS(OSD_TRAPEZOID_FILTER_OFF));
}

static void M_ToggleZBuffer(void)
{
    if (g_Input.slow) {
        g_Config.rendering.enable_wireframe =
            !g_Config.rendering.enable_wireframe;
        Config_Write();
        Overlay_DisplayModeInfo(
            g_Config.rendering.enable_wireframe ? GS(OSD_WIREFRAME_MODE_ON)
                                                : GS(OSD_WIREFRAME_MODE_OFF));
    } else {
        g_Config.rendering.enable_zbuffer = !g_Config.rendering.enable_zbuffer;
        Config_Write();
        Overlay_DisplayModeInfo(
            g_Config.rendering.enable_zbuffer ? GS(OSD_DEPTH_BUFFER_ON)
                                              : GS(OSD_DEPTH_BUFFER_OFF));
    }
}

static void M_CycleLightingContrast(void)
{
    const int32_t direction = g_Input.slow ? -1 : 1;
    LIGHTING_CONTRAST value = g_Config.rendering.lighting_contrast;
    value += direction;
    value += LIGHTING_CONTRAST_NUMBER_OF;
    value %= LIGHTING_CONTRAST_NUMBER_OF;
    g_Config.rendering.lighting_contrast = value;
    Config_Write();
    char tmp[100];
    sprintf(
        tmp, GS(OSD_LIGHTNING_CONTRAST_FMT),
        ENUM_MAP_TO_STRING(LIGHTING_CONTRAST, value));
    Overlay_DisplayModeInfo(tmp);
}

static void M_ToggleFullscreen(void)
{
    g_Config.window.is_fullscreen = !g_Config.window.is_fullscreen;
    Config_Write();
}

static void M_ToggleRenderingMode(void)
{
    g_Config.rendering.render_mode =
        g_Config.rendering.render_mode == RM_HARDWARE ? RM_SOFTWARE
                                                      : RM_HARDWARE;
    Config_Write();
    Overlay_DisplayModeInfo(
        g_Config.rendering.render_mode == RM_HARDWARE
            ? GS(OSD_HARDWARE_RENDERING)
            : GS(OSD_SOFTWARE_RENDERING));
}

static void M_DecreaseResolutionOrBPP(void)
{
    if (g_Camera.type == CAM_CINEMATIC) {
        return;
    }

    g_Config.rendering.scaler--;
    Config_Write();
    char mode_string[64] = {};
    sprintf(mode_string, GS(OSD_SCALER_FMT), g_Config.rendering.scaler);
    Overlay_DisplayModeInfo(mode_string);
}

static void M_IncreaseResolutionOrBPP(void)
{
    if (g_Camera.type == CAM_CINEMATIC) {
        return;
    }

    g_Config.rendering.scaler++;
    Config_Write();
    char mode_string[64] = {};
    sprintf(mode_string, GS(OSD_SCALER_FMT), g_Config.rendering.scaler);
    Overlay_DisplayModeInfo(mode_string);
}

static void M_DecreaseInternalScreenSize(void)
{
    if (g_Camera.type == CAM_CINEMATIC) {
        return;
    }

    DecreaseScreenSize();
}

static void M_IncreaseInternalScreenSize(void)
{
    if (g_Camera.type == CAM_CINEMATIC) {
        return;
    }

    IncreaseScreenSize();
}

void Shell_ProcessInput(void)
{
    if (g_InputDB.screenshot) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }

    if (g_InputDB.switch_resolution) {
        if (g_Input.slow) {
            M_DecreaseResolutionOrBPP();
        } else {
            M_IncreaseResolutionOrBPP();
        }
    }

    if (g_InputDB.switch_internal_screen_size) {
        if (g_Input.slow) {
            M_IncreaseInternalScreenSize();
        } else {
            M_DecreaseInternalScreenSize();
        }
    }

    if (g_InputDB.toggle_fps_counter) {
        M_ToggleFPSCounter();
    }

    if (g_InputDB.toggle_bilinear_filter) {
        M_ToggleBilinearFiltering();
    }

    // XXX: this is a poor hack
    if (g_InputDB.toggle_perspective_filter
        || g_InputDB.toggle_trapezoid_filter) {
        if (g_Config.rendering.render_mode == RM_HARDWARE) {
            M_ToggleTrapezoidFilter();
        } else {
            M_TogglePerspectiveCorrection();
        }
    }

    if (g_InputDB.toggle_z_buffer) {
        M_ToggleZBuffer();
    }

    if (g_InputDB.cycle_lighting_contrast) {
        M_CycleLightingContrast();
    }

    if (g_InputDB.toggle_fullscreen) {
        M_ToggleFullscreen();
    }

    if (g_InputDB.toggle_rendering_mode) {
        M_ToggleRenderingMode();
    }

    if (g_InputDB.turbo_cheat) {
        Clock_CycleTurboSpeed(!g_Input.slow);
    }
}
