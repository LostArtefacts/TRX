#include "specific/s_misc.h"

#include "game/gameflow.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_output.h"

#include <string.h>

void S_NoFade()
{
    // not implemented in TombATI
}

void S_FadeInInventory(int32_t fade)
{
    if (g_CurrentLevel != g_GameFlow.title_level_num) {
        S_Output_CopyFromPicture();
    }
}

void S_FadeOutInventory(int32_t fade)
{
    // not implemented in TombATI
}

void S_FadeToBlack()
{
    memset(g_GamePalette, 0, sizeof(g_GamePalette));
    S_Output_FadeToPal(20, g_GamePalette);
    S_Output_FadeWait();
}

void S_FinishInventory()
{
    if (g_InvMode != INV_TITLE_MODE) {
        Screen_ApplyResolution();
    }
    g_ModeLock = false;
}

RGB888 S_ColourFromPalette(int8_t idx)
{
    RGB888 ret;
    ret.r = 4 * g_GamePalette[idx].r;
    ret.g = 4 * g_GamePalette[idx].g;
    ret.b = 4 * g_GamePalette[idx].b;
    return ret;
}

int S_GetObjectBounds(int16_t *bptr)
{
    if (g_PhdMatrixPtr->_23 >= Output_GetFarZ()) {
        return 0;
    }

    int32_t x_min = bptr[0];
    int32_t x_max = bptr[1];
    int32_t y_min = bptr[2];
    int32_t y_max = bptr[3];
    int32_t z_min = bptr[4];
    int32_t z_max = bptr[5];

    PHD_VECTOR vtx[8];
    vtx[0].x = x_min;
    vtx[0].y = y_min;
    vtx[0].z = z_min;
    vtx[1].x = x_max;
    vtx[1].y = y_min;
    vtx[1].z = z_min;
    vtx[2].x = x_max;
    vtx[2].y = y_max;
    vtx[2].z = z_min;
    vtx[3].x = x_min;
    vtx[3].y = y_max;
    vtx[3].z = z_min;
    vtx[4].x = x_min;
    vtx[4].y = y_min;
    vtx[4].z = z_max;
    vtx[5].x = x_max;
    vtx[5].y = y_min;
    vtx[5].z = z_max;
    vtx[6].x = x_max;
    vtx[6].y = y_max;
    vtx[6].z = z_max;
    vtx[7].x = x_min;
    vtx[7].y = y_max;
    vtx[7].z = z_max;

    int num_z = 0;
    x_min = 0x3FFFFFFF;
    y_min = 0x3FFFFFFF;
    x_max = -0x3FFFFFFF;
    y_max = -0x3FFFFFFF;

    for (int i = 0; i < 8; i++) {
        int32_t zv = g_PhdMatrixPtr->_20 * vtx[i].x
            + g_PhdMatrixPtr->_21 * vtx[i].y + g_PhdMatrixPtr->_22 * vtx[i].z
            + g_PhdMatrixPtr->_23;

        if (zv > Output_GetNearZ() && zv < Output_GetFarZ()) {
            ++num_z;
            int32_t zp = zv / g_PhdPersp;
            int32_t xv =
                (g_PhdMatrixPtr->_00 * vtx[i].x + g_PhdMatrixPtr->_01 * vtx[i].y
                 + g_PhdMatrixPtr->_02 * vtx[i].z + g_PhdMatrixPtr->_03)
                / zp;
            int32_t yv =
                (g_PhdMatrixPtr->_10 * vtx[i].x + g_PhdMatrixPtr->_11 * vtx[i].y
                 + g_PhdMatrixPtr->_12 * vtx[i].z + g_PhdMatrixPtr->_13)
                / zp;

            if (x_min > xv) {
                x_min = xv;
            } else if (x_max < xv) {
                x_max = xv;
            }

            if (y_min > yv) {
                y_min = yv;
            } else if (y_max < yv) {
                y_max = yv;
            }
        }
    }

    x_min += ViewPort_GetCenterX();
    x_max += ViewPort_GetCenterX();
    y_min += ViewPort_GetCenterY();
    y_max += ViewPort_GetCenterY();

    if (!num_z || x_min > g_PhdRight || y_min > g_PhdBottom || x_max < g_PhdLeft
        || y_max < g_PhdTop) {
        return 0; // out of screen
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > ViewPort_GetMaxX()
        || y_max > ViewPort_GetMaxY()) {
        return -1; // clipped
    }

    return 1; // fully on screen
}
