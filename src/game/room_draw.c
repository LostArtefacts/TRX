#include "game/room_draw.h"

#include "game/output.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "math/matrix.h"

static int32_t m_RoomNumStack[MAX_ROOMS_TO_DRAW] = { 0 };
static int32_t m_RoomNumStackIdx = 0;

static void Room_PrintDrawStack(void);

static void Room_PrintDrawStack(void)
{
    for (int i = 0; i < m_RoomNumStackIdx; i++) {
        LOG_ERROR("Room Number %d", m_RoomNumStack[i]);
    }
}

bool Room_SetBounds(int16_t *objptr, int16_t room_num, ROOM_INFO *parent)
{
    // XXX: the way the game passes the objptr is dangerous and relies on
    // layout of DOOR_INFO

    if ((objptr[0] * (parent->x + objptr[3] - g_W2VMatrix._03))
            + (objptr[1] * (parent->y + objptr[4] - g_W2VMatrix._13))
            + (objptr[2] * (parent->z + objptr[5] - g_W2VMatrix._23))
        >= 0) {
        return false;
    }

    DOOR_VBUF door_vbuf[4];
    int32_t left = parent->right;
    int32_t right = parent->left;
    int32_t top = parent->bottom;
    int32_t bottom = parent->top;

    objptr += 3;
    int32_t z_toofar = 0;
    int32_t z_behind = 0;

    const MATRIX *mptr = g_MatrixPtr;
    for (int i = 0; i < 4; i++) {
        int32_t xv = mptr->_00 * objptr[0] + mptr->_01 * objptr[1]
            + mptr->_02 * objptr[2] + mptr->_03;
        int32_t yv = mptr->_10 * objptr[0] + mptr->_11 * objptr[1]
            + mptr->_12 * objptr[2] + mptr->_13;
        int32_t zv = mptr->_20 * objptr[0] + mptr->_21 * objptr[1]
            + mptr->_22 * objptr[2] + mptr->_23;
        door_vbuf[i].xv = xv;
        door_vbuf[i].yv = yv;
        door_vbuf[i].zv = zv;
        objptr += 3;

        if (zv > 0) {
            if (zv > Output_GetFarZ()) {
                z_toofar++;
            }

            zv /= g_PhdPersp;
            int32_t xs, ys;
            if (zv) {
                xs = Viewport_GetCenterX() + xv / zv;
                ys = Viewport_GetCenterY() + yv / zv;
            } else {
                xs = xv >= 0 ? g_PhdRight : g_PhdLeft;
                ys = yv >= 0 ? g_PhdBottom : g_PhdTop;
            }

            if (xs < left) {
                left = xs;
            }
            if (xs > right) {
                right = xs;
            }
            if (ys < top) {
                top = ys;
            }
            if (ys > bottom) {
                bottom = ys;
            }
        } else {
            z_behind++;
        }
    }

    if (z_behind == 4 || z_toofar == 4) {
        return false;
    }

    if (z_behind > 0) {
        DOOR_VBUF *dest = &door_vbuf[0];
        DOOR_VBUF *last = &door_vbuf[3];
        for (int i = 0; i < 4; i++) {
            if ((dest->zv < 0) ^ (last->zv < 0)) {
                if (dest->xv < 0 && last->xv < 0) {
                    left = 0;
                } else if (dest->xv > 0 && last->xv > 0) {
                    right = Viewport_GetMaxX();
                } else {
                    left = 0;
                    right = Viewport_GetMaxX();
                }

                if (dest->yv < 0 && last->yv < 0) {
                    top = 0;
                } else if (dest->yv > 0 && last->yv > 0) {
                    bottom = Viewport_GetMaxY();
                } else {
                    top = 0;
                    bottom = Viewport_GetMaxY();
                }
            }

            last = dest;
            dest++;
        }
    }

    if (left < parent->left) {
        left = parent->left;
    }
    if (right > parent->right) {
        right = parent->right;
    }
    if (top < parent->top) {
        top = parent->top;
    }
    if (bottom > parent->bottom) {
        bottom = parent->bottom;
    }

    if (left >= right || top >= bottom) {
        return false;
    }

    ROOM_INFO *r = &g_RoomInfo[room_num];
    if (left < r->left) {
        r->left = left;
    }
    if (top < r->top) {
        r->top = top;
    }
    if (right > r->right) {
        r->right = right;
    }
    if (bottom > r->bottom) {
        r->bottom = bottom;
    }

    if (!r->bound_active) {
        if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
            g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
        }
        r->bound_active = 1;
    }
    return true;
}

void Room_GetBounds(int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];
    if (!Matrix_Push()) {
        Room_PrintDrawStack();
        Shell_ExitSystem("Matrix stack overflow.");
    }
    m_RoomNumStack[m_RoomNumStackIdx++] = room_num;
    Matrix_TranslateAbs(r->x, r->y, r->z);
    if (r->doors) {
        for (int i = 0; i < r->doors->count; i++) {
            DOOR_INFO *door = &r->doors->door[i];
            if (Room_SetBounds(&door->x, door->room_num, r)) {
                Room_GetBounds(door->room_num);
            }
        }
    }
    Matrix_Pop();
    m_RoomNumStackIdx--;
}
