#include "game/shell/input.h"

#include "config.h"
#include "game/clock.h"
#include "game/console.h"
#include "game/viewport.h"
#include "utils.h"

static void M_IncreaseBorders(void);
static void M_DecreaseBorders(void);
static void M_ToggleFullscreen(void);
static void M_DecreaseUpscaling(void);
static void M_IncreaseUpscaling(void);

static void M_ToggleFullscreen(void)
{
    g_Config.window.is_fullscreen = !g_Config.window.is_fullscreen;
    Config_Update();
}

static void M_IncreaseBorders(void)
{
    if (g_Config.rendering.borders < 0.45) {
        g_Config.rendering.borders += 0.05;
        CLAMPG(g_Config.rendering.borders, 0.45);
        Viewport_Reset();
    }
}

static void M_DecreaseBorders(void)
{
    if (g_Config.rendering.borders > 0.0) {
        g_Config.rendering.borders -= 0.05;
        CLAMPL(g_Config.rendering.borders, 0.0);
        Viewport_Reset();
    }
}

static void M_DecreaseUpscaling(void)
{
    g_Config.rendering.upscaling_factor--;
    Config_Update();
    Console_Log(GS(OSD_UPSCALING_FACTOR), g_Config.rendering.upscaling_factor);
}

static void M_IncreaseUpscaling(void)
{
    g_Config.rendering.upscaling_factor++;
    Config_Update();
    Console_Log(GS(OSD_UPSCALING_FACTOR), g_Config.rendering.upscaling_factor);
}

void Shell_ProcessCommonInput(void)
{
    if (g_InputDB.screenshot) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }

    if (g_InputDB.switch_upscaling) {
        if (g_Input.slow) {
            M_DecreaseUpscaling();
        } else {
            M_IncreaseUpscaling();
        }
    }

    if (g_InputDB.switch_borders) {
        if (g_Input.slow) {
            M_DecreaseBorders();
        } else {
            M_IncreaseBorders();
        }
    }

    if (g_InputDB.toggle_fullscreen) {
        M_ToggleFullscreen();
    }

    if (g_InputDB.turbo_cheat && g_Config.gameplay.enable_cheats) {
        Clock_CycleTurboSpeed(!g_Input.slow);
    }
}
