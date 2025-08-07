#include "game/output/func.h"

#include "config.h"
#include "game/math.h"
#include "game/output/const.h"
#include "game/output/state.h"
#include "gfx/context.h"
#include "log.h"

extern int32_t g_PhdPersp;

void Output_MakeScreenshot(const char *const path)
{
    LOG_INFO("Taking screenshot");
    GFX_Context_ScheduleScreenshot(path);
}

int32_t Output_CalcFogShade(const int32_t depth)
{
#if TR_VERSION == 1
    return 0;
#endif
    const int32_t fog_start = Output_GetFogStart();
    const int32_t fog_end = Output_GetFogEnd();
    if (depth < fog_start) {
        return 0;
    }
    if (depth >= fog_end) {
        return SHADE_MAX;
    }
    return (depth - fog_start) * SHADE_MAX / (fog_end - fog_start);
}

void Output_ApplyFOV(void)
{
    int32_t fov = Viewport_GetEffectiveFOV();
    const int32_t sw = Viewport_GetWidth(VIEWPORT_GAME);
    const int32_t sh = Viewport_GetHeight(VIEWPORT_GAME);

#if TR_VERSION == 1
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.visuals.fov_vertical) {
        double aspect_ratio = sw / (float)sh;
        double fov_rad_h = fov * M_PI / (180 * DEG_1);
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * (180 * DEG_1));
    }

    const int32_t fov_width = sw;
#else
    const int32_t adjust = g_Config.visuals.use_psx_fov ? 200 : 240;
    const int32_t fov_width = sw * 240 / 320 * 240 / adjust;
#endif

    const int16_t c = Math_Cos(fov / 2);
    const int16_t s = Math_Sin(fov / 2);
    g_PhdPersp = fov_width / 2;
    if (s != 0) {
        g_PhdPersp *= c;
        g_PhdPersp /= s;
    }
}
