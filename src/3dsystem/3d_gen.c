#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/vars.h"
#include "config.h"

#ifdef T1M_FEAT_UI
    #include "specific/output.h"
    #include <math.h>
#endif

void phd_InitWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t nearz,
    int32_t farz, int32_t view_angle, int32_t scrwidth, int32_t scrheight,
    uint8_t* scrptr)
{
    PhdWinPtr = &scrptr[x + y * scrwidth];
    PhdWinMaxX = width - 1;
    PhdWinMaxY = height - 1;
    PhdWinWidth = width;
    PhdWinHeight = height;
    PhdCenterX = width / 2;
    PhdCenterY = height / 2;
    PhdNearZ = nearz << W2V_SHIFT;
    PhdFarZ = farz << W2V_SHIFT;
    PhdViewDist = farz;
    PhdScrHeight = scrheight;
    PhdScrWidth = scrwidth;

#ifdef T1M_FEAT_UI
    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);
#else
    AlterFOV(view_angle * PHD_DEGREE);
#endif

    PhdLeft = 0;
    PhdTop = 0;
    PhdRight = PhdWinMaxX;
    PhdBottom = PhdWinMaxY;

    PhdMatrixPtr = &MatrixStack;
}

void AlterFOV(PHD_ANGLE fov)
{
#ifdef T1M_FEAT_UI
    // NOTE: in places that use GAME_FOV, it can be safely changed to user's
    // choice. But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (T1MConfig.fov_vertical) {
        double aspect_ratio = GetRenderWidth() / (double)GetRenderHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }
#endif

    int16_t c = phd_cos(fov / 2);
    int16_t s = phd_sin(fov / 2);
    PhdPersp = ((PhdWinWidth / 2) * c) / s;
}

void phd_PopMatrix()
{
    PhdMatrixPtr--;
}

void T1MInject3DSystem3DGen()
{
    INJECT(0x004025D0, phd_InitWindow);
    INJECT(0x004026D0, AlterFOV);
}
