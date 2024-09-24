#include "game/room_draw.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/draw.h"
#include "game/output.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <libtrx/log.h>

#include <stdbool.h>

static int32_t m_RoomNumStack[MAX_ROOMS_TO_DRAW] = { 0 };
static int32_t m_RoomNumStackIdx = 0;

static void M_PrintDrawStack(void);
static bool M_SetBounds(const PORTAL *portal, const ROOM *parent);
static void M_GetBounds(int16_t room_num);
static void M_PrepareToDraw(int16_t room_num);
static void M_DrawSkybox(void);

static void M_PrintDrawStack(void)
{
    for (int i = 0; i < m_RoomNumStackIdx; i++) {
        LOG_ERROR("Room Number %d", m_RoomNumStack[i]);
    }
}

static bool M_SetBounds(const PORTAL *portal, const ROOM *parent)
{
    const int32_t x = portal->normal.x
        * (parent->pos.x + portal->vertex[0].x - g_W2VMatrix._03);
    const int32_t y = portal->normal.y
        * (parent->pos.y + portal->vertex[0].y - g_W2VMatrix._13);
    const int32_t z = portal->normal.z
        * (parent->pos.z + portal->vertex[0].z - g_W2VMatrix._23);
    if (x + y + z >= 0) {
        return false;
    }

    DOOR_VBUF door_vbuf[4];
    int32_t left = parent->bound_right;
    int32_t right = parent->bound_left;
    int32_t top = parent->bound_bottom;
    int32_t bottom = parent->bound_top;

    int32_t z_toofar = 0;
    int32_t z_behind = 0;

    const MATRIX *mptr = g_MatrixPtr;
    for (int i = 0; i < 4; i++) {
        int32_t xv = mptr->_00 * portal->vertex[i].x
            + mptr->_01 * portal->vertex[i].y + mptr->_02 * portal->vertex[i].z
            + mptr->_03;
        int32_t yv = mptr->_10 * portal->vertex[i].x
            + mptr->_11 * portal->vertex[i].y + mptr->_12 * portal->vertex[i].z
            + mptr->_13;
        int32_t zv = mptr->_20 * portal->vertex[i].x
            + mptr->_21 * portal->vertex[i].y + mptr->_22 * portal->vertex[i].z
            + mptr->_23;
        door_vbuf[i].xv = xv;
        door_vbuf[i].yv = yv;
        door_vbuf[i].zv = zv;

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

    if (left < parent->bound_left) {
        left = parent->bound_left;
    }
    if (right > parent->bound_right) {
        right = parent->bound_right;
    }
    if (top < parent->bound_top) {
        top = parent->bound_top;
    }
    if (bottom > parent->bound_bottom) {
        bottom = parent->bound_bottom;
    }

    if (left >= right || top >= bottom) {
        return false;
    }

    ROOM *r = &g_RoomInfo[portal->room_num];
    if (left < r->bound_left) {
        r->bound_left = left;
    }
    if (top < r->bound_top) {
        r->bound_top = top;
    }
    if (right > r->bound_right) {
        r->bound_right = right;
    }
    if (bottom > r->bound_bottom) {
        r->bound_bottom = bottom;
    }

    if (!r->bound_active) {
        if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
            g_RoomsToDraw[g_RoomsToDrawCount++] = portal->room_num;
        }
        r->bound_active = 1;
    }
    return true;
}

static void M_GetBounds(int16_t room_num)
{
    ROOM *r = &g_RoomInfo[room_num];
    if (!Matrix_Push()) {
        M_PrintDrawStack();
        Shell_ExitSystem("Matrix stack overflow.");
    }
    m_RoomNumStack[m_RoomNumStackIdx++] = room_num;
    Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);
    if (r->portals != NULL) {
        for (int i = 0; i < r->portals->count; i++) {
            const PORTAL *portal = &r->portals->portal[i];
            if (M_SetBounds(portal, r)) {
                M_GetBounds(portal->room_num);
            }
        }
    }
    Matrix_Pop();
    m_RoomNumStackIdx--;
}

void Room_DrawAllRooms(int16_t base_room, int16_t target_room)
{
    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    g_RoomsToDrawCount = 0;

    M_PrepareToDraw(base_room);
    M_PrepareToDraw(target_room);
    M_DrawSkybox();

    for (int i = 0; i < g_RoomsToDrawCount; i++) {
        Room_DrawSingleRoom(g_RoomsToDraw[i]);
    }
}

static void M_PrepareToDraw(int16_t room_num)
{
    ROOM *r = &g_RoomInfo[room_num];
    if (r->bound_active) {
        return;
    }

    r->bound_left = g_PhdLeft;
    r->bound_top = g_PhdTop;
    r->bound_right = g_PhdRight;
    r->bound_bottom = g_PhdBottom;
    r->bound_active = 1;

    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }

    Matrix_Push();
    Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);
    if (r->portals != NULL) {
        for (int i = 0; i < r->portals->count; i++) {
            const PORTAL *portal = &r->portals->portal[i];
            if (M_SetBounds(portal, r)) {
                M_GetBounds(portal->room_num);
            }
        }
    }
    Matrix_Pop();
}

static void M_DrawSkybox(void)
{
    if (!Output_IsSkyboxEnabled()) {
        return;
    }

    Output_SetupAboveWater(g_Camera.underwater);
    Matrix_Push();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;

    const OBJECT skybox = g_Objects[O_SKYBOX];
    const FRAME_INFO *const frame = g_Anims[skybox.anim_idx].frame_ptr;
    Matrix_RotYXZpack(frame->mesh_rots[0]);
    Output_DrawSkybox(g_Meshes[skybox.mesh_idx]);

    Matrix_Pop();
}

void Room_DrawSingleRoom(int16_t room_num)
{
    bool camera_underwater =
        g_RoomInfo[g_Camera.pos.room_num].flags & RF_UNDERWATER;

    ROOM *r = &g_RoomInfo[room_num];
    if (r->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(camera_underwater);
    } else {
        Output_SetupAboveWater(camera_underwater);
    }

    r->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs(r->pos.x, r->pos.y, r->pos.z);

    g_PhdLeft = r->bound_left;
    g_PhdRight = r->bound_right;
    g_PhdTop = r->bound_top;
    g_PhdBottom = r->bound_bottom;

    Output_DrawRoom(r->data);

    for (int i = r->item_num; i != NO_ITEM; i = g_Items[i].next_item) {
        ITEM *item = &g_Items[i];
        if (item->status != IS_INVISIBLE) {
            g_Objects[item->object_id].draw_routine(item);
        }
    }

    for (int i = 0; i < r->num_meshes; i++) {
        MESH *mesh = &r->meshes[i];
        if (g_StaticObjects[mesh->static_num].flags & 2) {
            Matrix_Push();
            Matrix_TranslateAbs(mesh->pos.x, mesh->pos.y, mesh->pos.z);
            Matrix_RotY(mesh->rot.y);
            int clip =
                Output_GetObjectBounds(&g_StaticObjects[mesh->static_num].p);
            if (clip) {
                Output_CalculateStaticLight(mesh->shade);
                Output_DrawPolygons(
                    g_Meshes[g_StaticObjects[mesh->static_num].mesh_num], clip);
            }
            Matrix_Pop();
        }
    }

    for (int i = r->fx_num; i != NO_ITEM; i = g_Effects[i].next_draw) {
        Effect_Draw(i);
    }

    Matrix_Pop();

    r->bound_left = Viewport_GetMaxX();
    r->bound_bottom = 0;
    r->bound_right = 0;
    r->bound_top = Viewport_GetMaxY();
}
