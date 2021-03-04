#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/vars.h"
#include "config.h"

#ifdef T1M_FEAT_UI
    #include "specific/output.h"
    #include <math.h>
#endif

#define TRIGMULT2(A, B) (((A) * (B)) >> W2V_SHIFT)
#define TRIGMULT3(A, B, C) (TRIGMULT2((TRIGMULT2(A, B)), C))

void phd_GenerateW2V(PHD_3DPOS* viewpos)
{
    PhdMatrixPtr = &MatrixStack;
    int32_t sx = phd_sin(viewpos->x_rot);
    int32_t cx = phd_cos(viewpos->x_rot);
    int32_t sy = phd_sin(viewpos->y_rot);
    int32_t cy = phd_cos(viewpos->y_rot);
    int32_t sz = phd_sin(viewpos->z_rot);
    int32_t cz = phd_cos(viewpos->z_rot);

    MatrixStack._00 = TRIGMULT3(sx, sy, sz) + TRIGMULT2(cy, cz);
    MatrixStack._01 = TRIGMULT2(cx, sz);
    MatrixStack._02 = TRIGMULT3(sx, cy, sz) - TRIGMULT2(sy, cz);
    MatrixStack._10 = TRIGMULT3(sx, sy, cz) - TRIGMULT2(cy, sz);
    MatrixStack._11 = TRIGMULT2(cx, cz);
    MatrixStack._12 = TRIGMULT3(sx, cy, cz) + TRIGMULT2(sy, sz);
    MatrixStack._20 = TRIGMULT2(cx, sy);
    MatrixStack._21 = -sx;
    MatrixStack._22 = TRIGMULT2(cx, cy);
    MatrixStack._03 = viewpos->x;
    MatrixStack._13 = viewpos->y;
    MatrixStack._23 = viewpos->z;
    W2VMatrix = MatrixStack;
}

void phd_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll)
{
    PHD_ANGLE angles[2];
    phd_GetVectorAngles(xtar - xsrc, ytar - ysrc, ztar - zsrc, angles);

    PHD_3DPOS viewer;
    viewer.x = xsrc;
    viewer.y = ysrc;
    viewer.z = zsrc;
    viewer.x_rot = angles[1];
    viewer.y_rot = angles[0];
    viewer.z_rot = roll;
    phd_GenerateW2V(&viewer);
}

void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t* dest)
{
    dest[0] = phd_atan(z, x);

    while ((int16_t)x != x || (int16_t)y != y || (int16_t)z != z) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
    }

    PHD_ANGLE pitch = phd_atan(phd_sqrt(SQUARE(x) + SQUARE(z)), y);
    if ((y > 0 && pitch > 0) || (y < 0 && pitch < 0)) {
        pitch = -pitch;
    }

    dest[1] = pitch;
}

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
    INJECT(0x00401000, phd_GenerateW2V);
    INJECT(0x004011A0, phd_LookAt);
    INJECT(0x00401270, phd_GetVectorAngles);
    INJECT(0x004025D0, phd_InitWindow);
    INJECT(0x004026D0, AlterFOV);
}
