#include "game/room_draw.h"

#include "game/effects.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>
#include <libtrx/utils.h>

#define M_MAX_BOUND_ROOMS 128

static int32_t m_Outside;
static int32_t m_OutsideRight;
static int32_t m_OutsideLeft;
static int32_t m_OutsideTop;
static int32_t m_OutsideBottom;

static int32_t m_MidSort = 0;
static int32_t m_BoundStart;
static int32_t m_BoundEnd;
static int32_t m_BoundRooms[M_MAX_BOUND_ROOMS] = {};

void Room_GetBounds(void)
{
    while (m_BoundStart != m_BoundEnd) {
        const int16_t room_num =
            m_BoundRooms[m_BoundStart++ % M_MAX_BOUND_ROOMS];
        ROOM *const room = Room_Get(room_num);
        room->bound_active &= ~2;
        m_MidSort = (room->bound_active >> 8) + 1;

        if (room->test_left < room->bound_left) {
            room->bound_left = room->test_left;
        }
        if (room->test_top < room->bound_top) {
            room->bound_top = room->test_top;
        }
        if (room->test_right > room->bound_right) {
            room->bound_right = room->test_right;
        }
        if (room->test_bottom > room->bound_bottom) {
            room->bound_bottom = room->test_bottom;
        }

        if (!(room->bound_active & 1)) {
            Room_MarkToBeDrawn(room_num);
            room->bound_active |= 1;
            if (room->flags & RF_OUTSIDE) {
                m_Outside = RF_OUTSIDE;
            }
        }

        if (!(room->flags & RF_INSIDE) || (room->flags & RF_OUTSIDE)) {
            if (room->bound_left < m_OutsideLeft) {
                m_OutsideLeft = room->bound_left;
            }
            if (room->bound_right > m_OutsideRight) {
                m_OutsideRight = room->bound_right;
            }
            if (room->bound_top < m_OutsideTop) {
                m_OutsideTop = room->bound_top;
            }
            if (room->bound_bottom > m_OutsideBottom) {
                m_OutsideBottom = room->bound_bottom;
            }
        }

        if (room->portals == nullptr) {
            continue;
        }

        Matrix_Push();
        Matrix_TranslateAbs32(room->pos);
        for (int32_t i = 0; i < room->portals->count; i++) {
            const PORTAL *const portal = &room->portals->portal[i];

            // clang-format off
            const XYZ_32 offset = {
                .x = portal->normal.x * (room->pos.x + portal->vertex[0].x - g_W2VMatrix._03),
                .y = portal->normal.y * (room->pos.y + portal->vertex[0].y - g_W2VMatrix._13),
                .z = portal->normal.z * (room->pos.z + portal->vertex[0].z - g_W2VMatrix._23),
            };
            // clang-format on

            if (offset.x + offset.y + offset.z >= 0) {
                continue;
            }

            Room_SetBounds(&portal->normal.x, portal->room_num, room);
        }
        Matrix_Pop();
    }
}

void Room_SetBounds(
    const int16_t *obj_ptr, int32_t room_num, const ROOM *parent)
{
    ROOM *const room = Room_Get(room_num);
    const PORTAL *const portal = (const PORTAL *)(obj_ptr - 1);

    // clang-format off
    if (room->bound_left <= parent->test_left &&
        room->bound_right >= parent->test_right &&
        room->bound_top <= parent->test_top &&
        room->bound_bottom >= parent->test_bottom
   ) {
        return;
    }
    // clang-format on

    const MATRIX *const m = g_MatrixPtr;
    int32_t left = parent->test_right;
    int32_t right = parent->test_left;
    int32_t bottom = parent->test_top;
    int32_t top = parent->test_bottom;

    PORTAL_VBUF portal_vbuf[4];
    int32_t z_behind = 0;

    for (int32_t i = 0; i < 4; i++) {
        PORTAL_VBUF *const dvbuf = &portal_vbuf[i];
        const XYZ_16 *const dvtx = &portal->vertex[i];
        const int32_t xv =
            dvtx->x * m->_00 + dvtx->y * m->_01 + dvtx->z * m->_02 + m->_03;
        const int32_t yv =
            dvtx->x * m->_10 + dvtx->y * m->_11 + dvtx->z * m->_12 + m->_13;
        const int32_t zv =
            dvtx->x * m->_20 + dvtx->y * m->_21 + dvtx->z * m->_22 + m->_23;
        dvbuf->xv = xv;
        dvbuf->yv = yv;
        dvbuf->zv = zv;

        if (zv <= 0) {
            z_behind++;
            continue;
        }

        int32_t xs;
        int32_t ys;
        const int32_t zp = zv / g_PhdPersp;
        if (zp) {
            xs = Viewport_GetCenterX(VIEWPORT_GAME) + xv / zp;
            ys = Viewport_GetCenterY(VIEWPORT_GAME) + yv / zp;
        } else {
            xs = xv < 0 ? g_PhdLeft : g_PhdRight;
            ys = yv < 0 ? g_PhdTop : g_PhdBottom;
        }

        if (xs - 1 < left) {
            left = xs - 1;
        }
        if (xs + 1 > right) {
            right = xs + 1;
        }
        if (ys - 1 < top) {
            top = ys - 1;
        }
        if (ys + 1 > bottom) {
            bottom = ys + 1;
        }
    }

    if (z_behind == 4) {
        return;
    }

    if (z_behind > 0) {
        const PORTAL_VBUF *dest = &portal_vbuf[0];
        const PORTAL_VBUF *last = &portal_vbuf[3];

        for (int32_t i = 0; i < 4; i++, last = dest++) {
            if ((dest->zv < 0) == (last->zv < 0)) {
                continue;
            }

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
    }

    if (left < parent->test_left) {
        left = parent->test_left;
    }
    if (right > parent->test_right) {
        right = parent->test_right;
    }
    if (top < parent->test_top) {
        top = parent->test_top;
    }
    if (bottom > parent->test_bottom) {
        bottom = parent->test_bottom;
    }

    if (left >= right || top >= bottom) {
        return;
    }

    if (room->bound_active & 2) {
        if (left < room->test_left) {
            room->test_left = left;
        }
        if (top < room->test_top) {
            room->test_top = top;
        }
        if (right > room->test_right) {
            room->test_right = right;
        }
        if (bottom > room->test_bottom) {
            room->test_bottom = bottom;
        }
    } else {
        m_BoundRooms[m_BoundEnd++ % M_MAX_BOUND_ROOMS] = room_num;
        room->bound_active |= 2;
        room->bound_active += (int16_t)(m_MidSort << 8);
        room->test_left = left;
        room->test_right = right;
        room->test_top = top;
        room->test_bottom = bottom;
    }
}

void Room_DrawSingleRoomGeometry(const int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);
    if ((room->flags & RF_UNDERWATER) != 0) {
        Output_SetupBelowWater(g_Camera.underwater);
    } else {
        Output_SetupAboveWater(g_Camera.underwater);
    }

    g_PhdLeft = room->bound_left;
    g_PhdRight = room->bound_right;
    g_PhdTop = room->bound_top;
    g_PhdBottom = room->bound_bottom;

    if (g_Config.debug.enable_debug_room_clip) {
        Output_DrawScreenFrame(
            g_PhdLeft, g_PhdTop, g_PhdRight - g_PhdLeft, g_PhdBottom - g_PhdTop,
            (RGBA_8888) { 0, 255, 0, 128 }, (RGBA_8888) { 0, 255, 0, 128 }, 1);
    }

    Matrix_TranslateAbs32(room->pos);
    Output_LightRoom(room);
    Output_DrawRoom(room, false);
}

void Room_DrawSingleRoomObjects(const int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);

    if ((room->flags & RF_UNDERWATER) != 0) {
        Output_SetupBelowWater(g_Camera.underwater);
    } else {
        Output_SetupAboveWater(g_Camera.underwater);
    }

    room->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs32(room->pos);

    g_PhdLeft = room->bound_left;
    g_PhdTop = room->bound_top;
    g_PhdRight = room->bound_right;
    g_PhdBottom = room->bound_bottom;

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

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);

    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (item->status != IS_INVISIBLE && obj->draw_func != nullptr) {
            obj->draw_func(item);
        }
        item_num = item->next_item;
    }

    int16_t effect_num = room->effect_num;
    while (effect_num != NO_EFFECT) {
        const EFFECT *const effect = Effect_Get(effect_num);
        Effect_Draw(effect_num);
        effect_num = effect->next_free;
    }

    Matrix_Pop();

    room->bound_left = Viewport_GetMaxX(VIEWPORT_GAME);
    room->bound_bottom = Viewport_GetMinX(VIEWPORT_GAME);
    room->bound_right = Viewport_GetMinY(VIEWPORT_GAME);
    room->bound_top = Viewport_GetMaxY(VIEWPORT_GAME);
}

static void M_DrawSkybox(void)
{
    if (!Output_IsSkyboxEnabled()) {
        return;
    }

    g_PhdLeft = m_OutsideLeft;
    g_PhdRight = m_OutsideRight;
    g_PhdBottom = m_OutsideBottom;
    g_PhdTop = m_OutsideTop;

    const OBJECT *const skybox = Object_Get(O_SKYBOX);
    if (skybox->loaded) {
        Output_SetupAboveWater(g_Camera.underwater);
        Matrix_Push();
        g_MatrixPtr->_03 = 0;
        g_MatrixPtr->_13 = 0;
        g_MatrixPtr->_23 = 0;
        Matrix_Rot16(skybox->frame_base->mesh_rots[0]);
        Output_DrawSkybox(Object_GetMesh(skybox->mesh_idx));
        Matrix_Pop();
    } else {
        m_Outside = -1;
    }
}

void Room_DrawAllRooms(const int16_t current_room)
{
    ROOM *const room = Room_Get(current_room);
    room->test_left = Viewport_GetMinX(VIEWPORT_GAME);
    room->test_top = Viewport_GetMinY(VIEWPORT_GAME);
    room->test_right = Viewport_GetMaxX(VIEWPORT_GAME);
    room->test_bottom = Viewport_GetMaxY(VIEWPORT_GAME);
    room->bound_active = 2;

    g_PhdLeft = room->test_left;
    g_PhdTop = room->test_top;
    g_PhdRight = room->test_right;
    g_PhdBottom = room->test_bottom;

    m_BoundRooms[0] = current_room;
    m_BoundStart = 0;
    m_BoundEnd = 1;

    Room_DrawReset();
    m_Outside = room->flags & RF_OUTSIDE;

    if (m_Outside) {
        m_OutsideLeft = Viewport_GetMinX(VIEWPORT_GAME);
        m_OutsideTop = Viewport_GetMinY(VIEWPORT_GAME);
        m_OutsideRight = Viewport_GetMaxX(VIEWPORT_GAME);
        m_OutsideBottom = Viewport_GetMaxY(VIEWPORT_GAME);
    } else {
        m_OutsideLeft = Viewport_GetMaxX(VIEWPORT_GAME);
        m_OutsideTop = Viewport_GetMaxY(VIEWPORT_GAME);
        m_OutsideBottom = Viewport_GetMinY(VIEWPORT_GAME);
        m_OutsideRight = Viewport_GetMinX(VIEWPORT_GAME);
    }

    Room_GetBounds();

    if (m_Outside) {
        M_DrawSkybox();
    }

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const int16_t room_num = Room_DrawGetRoom(i);
        Room_DrawSingleRoomGeometry(room_num);
        Room_DrawSingleRoomObjects(room_num);
    }

    m_MidSort = 0;
    const ITEM *const lara_item = Lara_GetItem();
    if (Object_Get(O_LARA)->loaded && !(lara_item->flags & IF_ONE_SHOT)) {
        const ROOM *const lara_room = Room_Get(lara_item->room_num);
        if ((lara_room->flags & RF_UNDERWATER) != 0) {
            Output_SetupBelowWater(g_Camera.underwater);
        } else {
            Output_SetupAboveWater(g_Camera.underwater);
        }
        m_MidSort = lara_room->bound_active >> 8;
        if (m_MidSort) {
            m_MidSort--;
        }
        Lara_Draw(lara_item);
    }

    Output_SetupAboveWater(false);
}
