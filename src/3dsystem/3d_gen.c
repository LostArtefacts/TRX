#include "3dsystem/3d_gen.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/output.h"
#include "util.h"

#include <math.h>

#define TRIGMULT2(A, B) (((A) * (B)) >> W2V_SHIFT)
#define TRIGMULT3(A, B, C) (TRIGMULT2((TRIGMULT2(A, B)), C))
#define EXTRACT_ROT_Y(rots) (((rots >> 10) & 0x3FF) << 6)
#define EXTRACT_ROT_X(rots) (((rots >> 20) & 0x3FF) << 6)
#define EXTRACT_ROT_Z(rots) ((rots & 0x3FF) << 6)

void phd_GenerateW2V(PHD_3DPOS *viewpos)
{
    PhdMatrixPtr = &MatrixStack[0];
    int32_t sx = phd_sin(viewpos->x_rot);
    int32_t cx = phd_cos(viewpos->x_rot);
    int32_t sy = phd_sin(viewpos->y_rot);
    int32_t cy = phd_cos(viewpos->y_rot);
    int32_t sz = phd_sin(viewpos->z_rot);
    int32_t cz = phd_cos(viewpos->z_rot);

    MatrixStack[0]._00 = TRIGMULT3(sx, sy, sz) + TRIGMULT2(cy, cz);
    MatrixStack[0]._01 = TRIGMULT2(cx, sz);
    MatrixStack[0]._02 = TRIGMULT3(sx, cy, sz) - TRIGMULT2(sy, cz);
    MatrixStack[0]._10 = TRIGMULT3(sx, sy, cz) - TRIGMULT2(cy, sz);
    MatrixStack[0]._11 = TRIGMULT2(cx, cz);
    MatrixStack[0]._12 = TRIGMULT3(sx, cy, cz) + TRIGMULT2(sy, sz);
    MatrixStack[0]._20 = TRIGMULT2(cx, sy);
    MatrixStack[0]._21 = -sx;
    MatrixStack[0]._22 = TRIGMULT2(cx, cy);
    MatrixStack[0]._03 = viewpos->x;
    MatrixStack[0]._13 = viewpos->y;
    MatrixStack[0]._23 = viewpos->z;
    W2VMatrix = MatrixStack[0];
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

void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest)
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

void phd_RotX(PHD_ANGLE rx)
{
    if (!rx) {
        return;
    }

    PHD_MATRIX *mptr = PhdMatrixPtr;
    int32_t sx = phd_sin(rx);
    int32_t cx = phd_cos(rx);

    int32_t r0, r1;
    r0 = mptr->_01 * cx + mptr->_02 * sx;
    r1 = mptr->_02 * cx - mptr->_01 * sx;
    mptr->_01 = r0 >> W2V_SHIFT;
    mptr->_02 = r1 >> W2V_SHIFT;

    r0 = mptr->_11 * cx + mptr->_12 * sx;
    r1 = mptr->_12 * cx - mptr->_11 * sx;
    mptr->_11 = r0 >> W2V_SHIFT;
    mptr->_12 = r1 >> W2V_SHIFT;

    r0 = mptr->_21 * cx + mptr->_22 * sx;
    r1 = mptr->_22 * cx - mptr->_21 * sx;
    mptr->_21 = r0 >> W2V_SHIFT;
    mptr->_22 = r1 >> W2V_SHIFT;
}

void phd_RotY(PHD_ANGLE ry)
{
    if (!ry) {
        return;
    }

    PHD_MATRIX *mptr = PhdMatrixPtr;
    int32_t sy = phd_sin(ry);
    int32_t cy = phd_cos(ry);

    int32_t r0, r1;
    r0 = mptr->_00 * cy - mptr->_02 * sy;
    r1 = mptr->_02 * cy + mptr->_00 * sy;
    mptr->_00 = r0 >> W2V_SHIFT;
    mptr->_02 = r1 >> W2V_SHIFT;

    r0 = mptr->_10 * cy - mptr->_12 * sy;
    r1 = mptr->_12 * cy + mptr->_10 * sy;
    mptr->_10 = r0 >> W2V_SHIFT;
    mptr->_12 = r1 >> W2V_SHIFT;

    r0 = mptr->_20 * cy - mptr->_22 * sy;
    r1 = mptr->_22 * cy + mptr->_20 * sy;
    mptr->_20 = r0 >> W2V_SHIFT;
    mptr->_22 = r1 >> W2V_SHIFT;
}

void phd_RotZ(PHD_ANGLE rz)
{
    if (!rz) {
        return;
    }

    PHD_MATRIX *mptr = PhdMatrixPtr;
    int32_t sz = phd_sin(rz);
    int32_t cz = phd_cos(rz);

    int32_t r0, r1;
    r0 = mptr->_00 * cz + mptr->_01 * sz;
    r1 = mptr->_01 * cz - mptr->_00 * sz;
    mptr->_00 = r0 >> W2V_SHIFT;
    mptr->_01 = r1 >> W2V_SHIFT;

    r0 = mptr->_10 * cz + mptr->_11 * sz;
    r1 = mptr->_11 * cz - mptr->_10 * sz;
    mptr->_10 = r0 >> W2V_SHIFT;
    mptr->_11 = r1 >> W2V_SHIFT;

    r0 = mptr->_20 * cz + mptr->_21 * sz;
    r1 = mptr->_21 * cz - mptr->_20 * sz;
    mptr->_20 = r0 >> W2V_SHIFT;
    mptr->_21 = r1 >> W2V_SHIFT;
}

void phd_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz)
{
    PHD_MATRIX *mptr = PhdMatrixPtr;
    int32_t r0, r1;

    if (ry) {
        int32_t sy = phd_sin(ry);
        int32_t cy = phd_cos(ry);

        r0 = mptr->_00 * cy - mptr->_02 * sy;
        r1 = mptr->_02 * cy + mptr->_00 * sy;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cy - mptr->_12 * sy;
        r1 = mptr->_12 * cy + mptr->_10 * sy;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cy - mptr->_22 * sy;
        r1 = mptr->_22 * cy + mptr->_20 * sy;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    if (rx) {
        int32_t sx = phd_sin(rx);
        int32_t cx = phd_cos(rx);

        r0 = mptr->_01 * cx + mptr->_02 * sx;
        r1 = mptr->_02 * cx - mptr->_01 * sx;
        mptr->_01 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_11 * cx + mptr->_12 * sx;
        r1 = mptr->_12 * cx - mptr->_11 * sx;
        mptr->_11 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_21 * cx + mptr->_22 * sx;
        r1 = mptr->_22 * cx - mptr->_21 * sx;
        mptr->_21 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    if (rz) {
        int32_t sz = phd_sin(rz);
        int32_t cz = phd_cos(rz);

        r0 = mptr->_00 * cz + mptr->_01 * sz;
        r1 = mptr->_01 * cz - mptr->_00 * sz;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_01 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cz + mptr->_11 * sz;
        r1 = mptr->_11 * cz - mptr->_10 * sz;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_11 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cz + mptr->_21 * sz;
        r1 = mptr->_21 * cz - mptr->_20 * sz;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_21 = r1 >> W2V_SHIFT;
    }
}

void phd_RotYXZpack(int32_t rots)
{
    PHD_MATRIX *mptr = PhdMatrixPtr;
    int32_t r0, r1;

    PHD_ANGLE ry = EXTRACT_ROT_Y(rots);
    if (ry) {
        int32_t sy = phd_sin(ry);
        int32_t cy = phd_cos(ry);

        r0 = mptr->_00 * cy - mptr->_02 * sy;
        r1 = mptr->_02 * cy + mptr->_00 * sy;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cy - mptr->_12 * sy;
        r1 = mptr->_12 * cy + mptr->_10 * sy;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cy - mptr->_22 * sy;
        r1 = mptr->_22 * cy + mptr->_20 * sy;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    PHD_ANGLE rx = EXTRACT_ROT_X(rots);
    if (rx) {
        int32_t sx = phd_sin(rx);
        int32_t cx = phd_cos(rx);

        r0 = mptr->_01 * cx + mptr->_02 * sx;
        r1 = mptr->_02 * cx - mptr->_01 * sx;
        mptr->_01 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_11 * cx + mptr->_12 * sx;
        r1 = mptr->_12 * cx - mptr->_11 * sx;
        mptr->_11 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_21 * cx + mptr->_22 * sx;
        r1 = mptr->_22 * cx - mptr->_21 * sx;
        mptr->_21 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    PHD_ANGLE rz = EXTRACT_ROT_Z(rots);
    if (rz) {
        int32_t sz = phd_sin(rz);
        int32_t cz = phd_cos(rz);

        r0 = mptr->_00 * cz + mptr->_01 * sz;
        r1 = mptr->_01 * cz - mptr->_00 * sz;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_01 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cz + mptr->_11 * sz;
        r1 = mptr->_11 * cz - mptr->_10 * sz;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_11 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cz + mptr->_21 * sz;
        r1 = mptr->_21 * cz - mptr->_20 * sz;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_21 = r1 >> W2V_SHIFT;
    }
}

int32_t phd_TranslateRel(int32_t x, int32_t y, int32_t z)
{
    PHD_MATRIX *mptr = PhdMatrixPtr;
    mptr->_03 += mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 += mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 += mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
    return ABS(mptr->_03) <= PhdFarZ && ABS(mptr->_13) <= PhdFarZ
        && ABS(mptr->_23) <= PhdFarZ;
}

void phd_TranslateAbs(int32_t x, int32_t y, int32_t z)
{
    PHD_MATRIX *mptr = PhdMatrixPtr;
    x -= W2VMatrix._03;
    y -= W2VMatrix._13;
    z -= W2VMatrix._23;
    mptr->_03 = mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 = mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 = mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
}

int32_t visible_zclip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3)
{
    double v1x = vn1->xv;
    double v1y = vn1->yv;
    double v1z = vn1->zv;
    double v2x = vn2->xv;
    double v2y = vn2->yv;
    double v2z = vn2->zv;
    double v3x = vn3->xv;
    double v3y = vn3->yv;
    double v3z = vn3->zv;
    double a = v3y * v1x - v1y * v3x;
    double b = v3x * v1z - v1x * v3z;
    double c = v3z * v1y - v1z * v3y;
    return a * v2z + b * v2y + c * v2x < 0.0;
}

void phd_RotateLight(int16_t pitch, int16_t yaw)
{
    LsYaw = yaw;
    LsPitch = pitch;
    int32_t cp = phd_cos(pitch);
    int32_t sp = phd_sin(pitch);
    int32_t cy = phd_cos(yaw);
    int32_t sy = phd_sin(yaw);
    int32_t ls_x = TRIGMULT2(cp, sy);
    int32_t ls_y = -sp;
    int32_t ls_z = TRIGMULT2(cp, cy);
    LsVectorView.x =
        (W2VMatrix._00 * ls_x + W2VMatrix._01 * ls_y + W2VMatrix._02 * ls_z)
        >> W2V_SHIFT;
    LsVectorView.y =
        (W2VMatrix._10 * ls_x + W2VMatrix._11 * ls_y + W2VMatrix._12 * ls_z)
        >> W2V_SHIFT;
    LsVectorView.z =
        (W2VMatrix._20 * ls_x + W2VMatrix._21 * ls_y + W2VMatrix._22 * ls_z)
        >> W2V_SHIFT;
}

void phd_InitWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t nearz,
    int32_t farz, int32_t view_angle, int32_t scrwidth, int32_t scrheight,
    uint8_t *scrptr)
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

    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);

    PhdLeft = 0;
    PhdTop = 0;
    PhdRight = PhdWinMaxX;
    PhdBottom = PhdWinMaxY;

    PhdMatrixPtr = &MatrixStack[0];
}

void AlterFOV(PHD_ANGLE fov)
{
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (T1MConfig.fov_vertical) {
        double aspect_ratio = GetRenderWidth() / (double)GetRenderHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }

    int16_t c = phd_cos(fov / 2);
    int16_t s = phd_sin(fov / 2);
    PhdPersp = ((PhdWinWidth / 2) * c) / s;
}

// originally in shell.c
void phd_PushMatrix()
{
    PhdMatrixPtr++;
    PhdMatrixPtr[0] = PhdMatrixPtr[-1];
}

// originally in shell.c
void phd_PushUnitMatrix()
{
    PHD_MATRIX *mptr = ++PhdMatrixPtr;
    mptr->_00 = W2V_SCALE;
    mptr->_01 = 0;
    mptr->_02 = 0;
    mptr->_10 = 0;
    mptr->_11 = W2V_SCALE;
    mptr->_12 = 0;
    mptr->_20 = 0;
    mptr->_21 = 0;
    mptr->_22 = W2V_SCALE;
}

void phd_PopMatrix()
{
    PhdMatrixPtr--;
}

int16_t *calc_object_vertices(int16_t *obj_ptr)
{
    int16_t total_clip = -1;

    obj_ptr++;
    int vertex_count = *obj_ptr++;
    for (int i = 0; i < vertex_count; i++) {
        int32_t xv = PhdMatrixPtr->_00 * obj_ptr[0]
            + PhdMatrixPtr->_01 * obj_ptr[1] + PhdMatrixPtr->_02 * obj_ptr[2]
            + PhdMatrixPtr->_03;
        int32_t yv = PhdMatrixPtr->_10 * obj_ptr[0]
            + PhdMatrixPtr->_11 * obj_ptr[1] + PhdMatrixPtr->_12 * obj_ptr[2]
            + PhdMatrixPtr->_13;
        int32_t zv = PhdMatrixPtr->_20 * obj_ptr[0]
            + PhdMatrixPtr->_21 * obj_ptr[1] + PhdMatrixPtr->_22 * obj_ptr[2]
            + PhdMatrixPtr->_23;
        PhdVBuf[i].xv = xv;
        PhdVBuf[i].yv = yv;
        PhdVBuf[i].zv = zv;

        int32_t clip_flags;
        if (zv < PhdNearZ) {
            clip_flags = -32768;
        } else {
            clip_flags = 0;

            int32_t xs = PhdWinCenterX + xv / (zv / PhdPersp);
            int32_t ys = PhdWinCenterY + yv / (zv / PhdPersp);

            if (xs < PhdLeft) {
                if (xs < -32760) {
                    xs = -32760;
                }
                clip_flags |= 1;
            } else if (xs > PhdRight) {
                if (xs > 32760) {
                    xs = 32760;
                }
                clip_flags |= 2;
            }

            if (ys < PhdTop) {
                if (ys < -32760) {
                    ys = -32760;
                }
                clip_flags |= 4;
            } else if (ys > PhdBottom) {
                if (ys > 32760) {
                    ys = 32760;
                }
                clip_flags |= 8;
            }

            PhdVBuf[i].xs = xs;
            PhdVBuf[i].ys = ys;
        }

        PhdVBuf[i].clip = clip_flags;
        total_clip &= clip_flags;
        obj_ptr += 3;
    }

    return total_clip == 0 ? obj_ptr : NULL;
}

void T1MInject3DSystem3DGen()
{
    INJECT(0x00401000, phd_GenerateW2V);
    INJECT(0x004011A0, phd_LookAt);
    INJECT(0x00401270, phd_GetVectorAngles);
    INJECT(0x004012F0, phd_RotX);
    INJECT(0x004013A0, phd_RotY);
    INJECT(0x00401450, phd_RotZ);
    INJECT(0x00401500, phd_RotYXZ);
    INJECT(0x004016F0, phd_RotYXZpack);
    INJECT(0x004018F0, phd_TranslateRel);
    INJECT(0x004019A0, phd_TranslateAbs);
    INJECT(0x00401A20, visible_zclip);
    INJECT(0x00401C40, calc_object_vertices);
    INJECT(0x004023A0, phd_RotateLight);
    INJECT(0x004025D0, phd_InitWindow);
    INJECT(0x004026D0, AlterFOV);
    INJECT(0x0043EA01, phd_PushMatrix);
    INJECT(0x0043EA21, phd_PushUnitMatrix);
}
