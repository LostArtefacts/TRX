#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>

#include <math.h>

bool Output_MakeScreenshot(const char *const path)
{
    GFX_Context_ScheduleScreenshot(path);
    return true;
}

int32_t Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    // TODO: remove
    return 1;
}

int32_t Output_CalcFogShade(const int32_t depth)
{
    // TODO: done in the shader
    return 0;
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    // TODO: remove
    ASSERT_FAIL();
    return 0;
}

void Output_LightRoomVertices(const ROOM *const room)
{
    // TODO: remove
    ASSERT_FAIL();
}

void Output_ApplyFOV(void)
{
    int32_t fov = Viewport_GetEffectiveFOV();

    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.visuals.fov_vertical) {
        double aspect_ratio = Viewport_GetWidth(VIEWPORT_GAME)
            / (double)Viewport_GetHeight(VIEWPORT_GAME);
        double fov_rad_h = fov * M_PI / (180 * DEG_1);
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * (180 * DEG_1));
    }

    const int16_t c = Math_Cos(fov / 2);
    const int16_t s = Math_Sin(fov / 2);
    g_PhdPersp = Viewport_GetWidth(VIEWPORT_GAME) / 2;
    if (s != 0) {
        g_PhdPersp *= c;
        g_PhdPersp /= s;
    }
}
