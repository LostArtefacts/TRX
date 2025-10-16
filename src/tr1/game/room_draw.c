#include "game/room_draw.h"

#include "game/effects.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara/draw.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>
#include <libtrx/game/viewport.h>
#include <libtrx/log.h>

static int32_t m_RoomNumStack[MAX_ROOMS_TO_DRAW] = {};
static int32_t m_RoomNumStackIdx = 0;

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
                xs = Viewport_GetCenterX(VIEWPORT_GAME) + xv / zv;
                ys = Viewport_GetCenterY(VIEWPORT_GAME) + yv / zv;
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
                    left = Viewport_GetMinX(VIEWPORT_GAME);
                } else if (dest->xv > 0 && last->xv > 0) {
                    right = Viewport_GetMaxX(VIEWPORT_GAME);
                } else {
                    left = Viewport_GetMinX(VIEWPORT_GAME);
                    right = Viewport_GetMaxX(VIEWPORT_GAME);
                }

                if (dest->yv < 0 && last->yv < 0) {
                    top = Viewport_GetMinY(VIEWPORT_GAME);
                } else if (dest->yv > 0 && last->yv > 0) {
                    bottom = Viewport_GetMaxY(VIEWPORT_GAME);
                } else {
                    top = Viewport_GetMinY(VIEWPORT_GAME);
                    bottom = Viewport_GetMaxY(VIEWPORT_GAME);
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

void Room_DrawAllRooms(int16_t base_room, int16_t target_room)
{
    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);

    Room_DrawReset();

    M_PrepareToDraw(base_room);
    if (!Room_CheckOverlap(base_room, target_room)) {
        M_PrepareToDraw(target_room);
    }
    M_DrawSkybox();

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        Room_DrawSingleRoom(Room_DrawGetRoom(i));
    }
    Output_SetupAboveWater(false);
}

void Room_DrawSingleRoom(const int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);
    if (room->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(g_Camera.underwater);
    } else {
        Output_SetupAboveWater(g_Camera.underwater);
    }

    room->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs32(room->pos);

    g_PhdLeft = room->bound_left;
    g_PhdRight = room->bound_right;
    g_PhdTop = room->bound_top;
    g_PhdBottom = room->bound_bottom;

    if (g_Config.debug.enable_debug_room_clip) {
        Output_DrawScreenFrame(
            g_PhdLeft, g_PhdTop, g_PhdRight - g_PhdLeft, g_PhdBottom - g_PhdTop,
            (RGBA_8888) { 0, 255, 0, 128 }, (RGBA_8888) { 0, 255, 0, 128 }, 1);
    }

    Output_LightRoom(room);
    Output_DrawRoom(room, false);

    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (item->status != IS_INVISIBLE && obj->draw_func != nullptr) {
            obj->draw_func(item);
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
        const CLIP clip = Output_CheckBoundsClip(&obj->draw_bounds);
        if (clip != CLIP_NOT_VISIBLE) {
            Output_CalculateStaticMeshLight(mesh->pos, mesh->shade, room);
            Object_DrawMesh(obj->mesh_idx, clip, false);
            if (g_Config.debug.enable_debug_cuboids) {
                Output_DrawCuboid(&obj->draw_bounds);
            }
        }
        Matrix_Pop();
    }

    for (int32_t i = room->effect_num; i != NO_EFFECT;
         i = Effect_Get(i)->next_draw) {
        Effect_Draw(i);
    }

    Matrix_Pop();

    room->bound_left = Viewport_GetMaxX(VIEWPORT_GAME);
    room->bound_bottom = Viewport_GetMinX(VIEWPORT_GAME);
    room->bound_right = Viewport_GetMinY(VIEWPORT_GAME);
    room->bound_top = Viewport_GetMaxY(VIEWPORT_GAME);
}
