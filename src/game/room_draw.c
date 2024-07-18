#include "game/room_draw.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/lara_draw.h"
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

static void Room_PrintDrawStack(void);
static bool Room_SetBounds(const DOOR_INFO *door, const ROOM_INFO *parent);
static void Room_GetBounds(int16_t room_num);
static void Room_PrepareToDraw(int16_t room_num);
static void Room_DrawSkybox(void);

static void Room_PrintDrawStack(void)
{
    for (int i = 0; i < m_RoomNumStackIdx; i++) {
        LOG_ERROR("Room Number %d", m_RoomNumStack[i]);
    }
}

static bool Room_SetBounds(const DOOR_INFO *door, const ROOM_INFO *parent)
{
    const int32_t x =
        door->normal.x * (parent->x + door->vertex[0].x - g_W2VMatrix._03);
    const int32_t y =
        door->normal.y * (parent->y + door->vertex[0].y - g_W2VMatrix._13);
    const int32_t z =
        door->normal.z * (parent->z + door->vertex[0].z - g_W2VMatrix._23);
    if (x + y + z >= 0) {
        return false;
    }

    DOOR_VBUF door_vbuf[4];
    int32_t left = parent->right;
    int32_t right = parent->left;
    int32_t top = parent->bottom;
    int32_t bottom = parent->top;

    int32_t z_toofar = 0;
    int32_t z_behind = 0;

    const MATRIX *mptr = g_MatrixPtr;
    for (int i = 0; i < 4; i++) {
        int32_t xv = mptr->_00 * door->vertex[i].x
            + mptr->_01 * door->vertex[i].y + mptr->_02 * door->vertex[i].z
            + mptr->_03;
        int32_t yv = mptr->_10 * door->vertex[i].x
            + mptr->_11 * door->vertex[i].y + mptr->_12 * door->vertex[i].z
            + mptr->_13;
        int32_t zv = mptr->_20 * door->vertex[i].x
            + mptr->_21 * door->vertex[i].y + mptr->_22 * door->vertex[i].z
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

    ROOM_INFO *r = &g_RoomInfo[door->room_num];
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
            g_RoomsToDraw[g_RoomsToDrawCount++] = door->room_num;
        }
        r->bound_active = 1;
    }
    return true;
}

static void Room_GetBounds(int16_t room_num)
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
            const DOOR_INFO *door = &r->doors->door[i];
            if (Room_SetBounds(door, r)) {
                Room_GetBounds(door->room_num);
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

    Room_PrepareToDraw(base_room);
    Room_PrepareToDraw(target_room);
    Room_DrawSkybox();

    for (int i = 0; i < g_RoomsToDrawCount; i++) {
        Room_DrawSingleRoom(g_RoomsToDraw[i]);
    }

    if (g_Objects[O_LARA].loaded) {
        if (g_RoomInfo[g_LaraItem->room_number].flags & RF_UNDERWATER) {
            Output_SetupBelowWater(g_Camera.underwater);
        } else {
            Output_SetupAboveWater(g_Camera.underwater);
        }
        Lara_Draw(g_LaraItem);
    }
}

static void Room_PrepareToDraw(int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];
    if (r->bound_active) {
        return;
    }

    r->left = g_PhdLeft;
    r->top = g_PhdTop;
    r->right = g_PhdRight;
    r->bottom = g_PhdBottom;
    r->bound_active = 1;

    if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
        g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
    }

    Matrix_Push();
    Matrix_TranslateAbs(r->x, r->y, r->z);
    if (r->doors) {
        for (int i = 0; i < r->doors->count; i++) {
            const DOOR_INFO *door = &r->doors->door[i];
            if (Room_SetBounds(door, r)) {
                Room_GetBounds(door->room_num);
            }
        }
    }
    Matrix_Pop();
}

static void Room_DrawSkybox(void)
{
    if (!Output_IsSkyboxEnabled()) {
        return;
    }

    Output_SetupAboveWater(g_Camera.underwater);
    Matrix_Push();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;

    const OBJECT_INFO skybox = g_Objects[O_SKYBOX];
    const FRAME_INFO *const frame = g_Anims[skybox.anim_index].frame_ptr;
    Matrix_RotYXZpack(frame->mesh_rots[0]);
    Output_DrawSkybox(g_Meshes[skybox.mesh_index]);

    Matrix_Pop();
}

void Room_DrawSingleRoom(int16_t room_num)
{
    bool camera_underwater =
        g_RoomInfo[g_Camera.pos.room_number].flags & RF_UNDERWATER;

    ROOM_INFO *r = &g_RoomInfo[room_num];
    if (r->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(camera_underwater);
    } else {
        Output_SetupAboveWater(camera_underwater);
    }

    r->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs(r->x, r->y, r->z);

    g_PhdLeft = r->left;
    g_PhdRight = r->right;
    g_PhdTop = r->top;
    g_PhdBottom = r->bottom;

    Output_DrawRoom(r->data);

    for (int i = r->item_number; i != NO_ITEM; i = g_Items[i].next_item) {
        ITEM_INFO *item = &g_Items[i];
        if (item->status != IS_INVISIBLE) {
            g_Objects[item->object_number].draw_routine(item);
        }
    }

    for (int i = 0; i < r->num_meshes; i++) {
        MESH_INFO *mesh = &r->mesh[i];
        if (g_StaticObjects[mesh->static_number].flags & 2) {
            Matrix_Push();
            Matrix_TranslateAbs(mesh->pos.x, mesh->pos.y, mesh->pos.z);
            Matrix_RotY(mesh->rot.y);
            int clip =
                Output_GetObjectBounds(&g_StaticObjects[mesh->static_number].p);
            if (clip) {
                Output_CalculateStaticLight(mesh->shade);
                Output_DrawPolygons(
                    g_Meshes[g_StaticObjects[mesh->static_number].mesh_number],
                    clip);
            }
            Matrix_Pop();
        }
    }

    for (int i = r->fx_number; i != NO_ITEM; i = g_Effects[i].next_draw) {
        Effect_Draw(i);
    }

    Matrix_Pop();

    r->left = Viewport_GetMaxX();
    r->bottom = 0;
    r->right = 0;
    r->top = Viewport_GetMaxY();
}
