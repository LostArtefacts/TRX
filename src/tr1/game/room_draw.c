#include "game/room_draw.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lara/draw.h"
#include "game/output.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/matrix.h>
#include <libtrx/log.h>

static int32_t m_RoomNumStack[MAX_ROOMS_TO_DRAW] = {};
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

    ROOM *const room = Room_Get(portal->room_num);
    if (left < room->bound_left) {
        room->bound_left = left;
    }
    if (top < room->bound_top) {
        room->bound_top = top;
    }
    if (right > room->bound_right) {
        room->bound_right = right;
    }
    if (bottom > room->bound_bottom) {
        room->bound_bottom = bottom;
    }

    if (!room->bound_active) {
        Room_MarkToBeDrawn(portal->room_num);
        room->bound_active = 1;
    }
    return true;
}

static void M_GetBounds(int16_t room_num)
{
    const ROOM *const room = Room_Get(room_num);
    if (!Matrix_Push()) {
        M_PrintDrawStack();
        Shell_ExitSystem("Matrix stack overflow.");
    }
    m_RoomNumStack[m_RoomNumStackIdx++] = room_num;
    Matrix_TranslateAbs32(room->pos);
    if (room->portals != nullptr) {
        for (int32_t i = 0; i < room->portals->count; i++) {
            const PORTAL *portal = &room->portals->portal[i];
            if (M_SetBounds(portal, room)) {
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

    Room_DrawReset();

    M_PrepareToDraw(base_room);
    M_PrepareToDraw(target_room);
    M_DrawSkybox();

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        Room_DrawSingleRoom(Room_DrawGetRoom(i));
    }
    Output_SetupAboveWater(false);
}

static void M_PrepareToDraw(int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);
    if (room->bound_active) {
        return;
    }

    room->bound_left = g_PhdLeft;
    room->bound_top = g_PhdTop;
    room->bound_right = g_PhdRight;
    room->bound_bottom = g_PhdBottom;
    room->bound_active = 1;

    Room_MarkToBeDrawn(room_num);

    Matrix_Push();
    Matrix_TranslateAbs32(room->pos);
    if (room->portals != nullptr) {
        for (int32_t i = 0; i < room->portals->count; i++) {
            const PORTAL *portal = &room->portals->portal[i];
            if (M_SetBounds(portal, room)) {
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

    const OBJECT *const skybox = Object_Get(O_SKYBOX);
    Matrix_Rot16(skybox->frame_base->mesh_rots[0]);
    Output_DrawSkybox(Object_GetMesh(skybox->mesh_idx));

    Matrix_Pop();
}

void Room_DrawSingleRoom(int16_t room_num)
{
    bool camera_underwater =
        Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER;

    ROOM *const room = Room_Get(room_num);
    if (room->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(camera_underwater);
    } else {
        Output_SetupAboveWater(camera_underwater);
    }

    room->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs32(room->pos);

    g_PhdLeft = room->bound_left;
    g_PhdRight = room->bound_right;
    g_PhdTop = room->bound_top;
    g_PhdBottom = room->bound_bottom;

    Output_LightRoom(room);
    Output_DrawRoom(&room->mesh);

    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        if (item->status != IS_INVISIBLE) {
            Object_Get(item->object_id)->draw_routine(item);
        }
        item_num = item->next_item;
    }

    for (int32_t i = 0; i < room->num_static_meshes; i++) {
        const STATIC_MESH *const mesh = &room->static_meshes[i];
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(mesh->static_num);
        if (!obj->visible) {
            continue;
        }

        Matrix_Push();
        Matrix_TranslateAbs32(mesh->pos);
        Matrix_RotY(mesh->rot.y);
        int32_t clip = Output_GetObjectBounds(&obj->draw_bounds);
        if (clip != 0) {
            Output_CalculateStaticMeshLight(mesh->pos, mesh->shade, room);
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }
        Matrix_Pop();
    }

    for (int32_t i = room->effect_num; i != NO_EFFECT;
         i = Effect_Get(i)->next_draw) {
        Effect_Draw(i);
    }

    if (g_Config.rendering.enable_debug_triggers) {
        Output_DrawRoomTriggers(room);
    }
    if (g_Config.rendering.enable_debug_portals) {
        Output_DrawRoomPortals(room);
    }
    Matrix_Pop();

    room->bound_left = Viewport_GetMaxX();
    room->bound_bottom = 0;
    room->bound_right = 0;
    room->bound_top = Viewport_GetMaxY();
}
