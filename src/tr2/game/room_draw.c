#include "game/room_draw.h"

#include "decomp/decomp.h"
#include "game/effects.h"
#include "game/lara/draw.h"
#include "game/output.h"
#include "global/vars.h"

#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

static int32_t m_Outside;
static int32_t m_OutsideRight;
static int32_t m_OutsideLeft;
static int32_t m_OutsideTop;
static int32_t m_OutsideBottom;

static int32_t m_BoundStart;
static int32_t m_BoundEnd;
static int32_t m_BoundRooms[MAX_BOUND_ROOMS] = {};

static int32_t m_BoxLines[12][2] = {
    { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
    { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
};

void Room_GetBounds(void)
{
    while (m_BoundStart != m_BoundEnd) {
        const int16_t room_num = m_BoundRooms[m_BoundStart++ % MAX_BOUND_ROOMS];
        ROOM *const room = Room_Get(room_num);
        room->bound_active &= ~2;
        g_MidSort = (room->bound_active >> 8) + 1;

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
            xs = xv / zp + g_PhdWinCenterX;
            ys = yv / zp + g_PhdWinCenterY;
        } else {
            xs = xv < 0 ? g_PhdWinLeft : g_PhdWinRight;
            ys = yv < 0 ? g_PhdWinTop : g_PhdWinBottom;
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
                left = 0;
            } else if (dest->xv > 0 && last->xv > 0) {
                right = g_PhdWinMaxX;
            } else {
                left = 0;
                right = g_PhdWinMaxX;
            }

            if (dest->yv < 0 && last->yv < 0) {
                top = 0;
            } else if (dest->yv > 0 && last->yv > 0) {
                bottom = g_PhdWinMaxY;
            } else {
                top = 0;
                bottom = g_PhdWinMaxY;
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
        m_BoundRooms[m_BoundEnd++ % MAX_BOUND_ROOMS] = room_num;
        room->bound_active |= 2;
        room->bound_active += (int16_t)(g_MidSort << 8);
        room->test_left = left;
        room->test_right = right;
        room->test_top = top;
        room->test_bottom = bottom;
    }
}

void Room_Clip(const ROOM *const room)
{
    int32_t xv[8];
    int32_t yv[8];
    int32_t zv[8];

    xv[0] = WALL_L;
    yv[0] = room->max_ceiling - room->pos.y;
    zv[0] = WALL_L;

    xv[1] = (room->size.x - 1) * WALL_L;
    yv[1] = room->max_ceiling - room->pos.y;
    zv[1] = WALL_L;

    xv[2] = (room->size.x - 1) * WALL_L;
    yv[2] = room->max_ceiling - room->pos.y;
    zv[2] = (room->size.z - 1) * WALL_L;

    xv[3] = WALL_L;
    yv[3] = room->max_ceiling - room->pos.y;
    zv[3] = (room->size.z - 1) * WALL_L;

    xv[4] = WALL_L;
    yv[4] = room->min_floor - room->pos.y;
    zv[4] = WALL_L;

    xv[5] = (room->size.x - 1) * WALL_L;
    yv[5] = room->min_floor - room->pos.y;
    zv[5] = WALL_L;

    xv[6] = (room->size.x - 1) * WALL_L;
    yv[6] = room->min_floor - room->pos.y;
    zv[6] = (room->size.z - 1) * WALL_L;

    xv[7] = WALL_L;
    yv[7] = room->min_floor - room->pos.y;
    zv[7] = (room->size.z - 1) * WALL_L;

    bool clip_room = false;
    bool clip[8];

    const MATRIX *const m = g_MatrixPtr;
    for (int32_t i = 0; i < 8; i++) {
        const int32_t x = xv[i];
        const int32_t y = yv[i];
        const int32_t z = zv[i];
        xv[i] = x * m->_00 + y * m->_01 + z * m->_02 + m->_03;
        yv[i] = x * m->_10 + y * m->_11 + z * m->_12 + m->_13;
        zv[i] = x * m->_20 + y * m->_21 + z * m->_22 + m->_23;
        if (zv[i] > g_PhdFarZ) {
            clip_room = true;
            clip[i] = true;
        } else {
            clip[i] = false;
        }
    }

    if (!clip_room) {
        return;
    }

    int32_t min_x = 0x10000000;
    int32_t min_y = 0x10000000;
    int32_t max_x = -0x10000000;
    int32_t max_y = -0x10000000;
    for (int32_t i = 0; i < 12; i++) {
        const int32_t p1 = m_BoxLines[i][0];
        const int32_t p2 = m_BoxLines[i][1];

        if (clip[p1] == clip[p2]) {
            continue;
        }

        const int32_t zdiv = (zv[p2] - zv[p1]) >> W2V_SHIFT;
        if (zdiv) {
            const int32_t znom = (g_PhdFarZ - zv[p1]) >> W2V_SHIFT;
            const int32_t x = xv[p1]
                + ((((xv[p2] - xv[p1]) >> W2V_SHIFT) * znom / zdiv)
                   << W2V_SHIFT);
            const int32_t y = yv[p1]
                + ((((yv[p2] - yv[p1]) >> W2V_SHIFT) * znom / zdiv)
                   << W2V_SHIFT);

            CLAMPG(min_x, x);
            CLAMPL(max_x, x);
            CLAMPG(min_y, y);
            CLAMPL(max_y, y);
        } else {
            CLAMPG(min_x, xv[p1]);
            CLAMPG(min_x, xv[p2]);
            CLAMPL(max_x, xv[p1]);
            CLAMPL(max_x, xv[p2]);
            CLAMPG(min_y, yv[p1]);
            CLAMPG(min_y, yv[p2]);
            CLAMPL(max_y, yv[p1]);
            CLAMPL(max_y, yv[p2]);
        }
    }

    const int32_t zp = g_PhdFarZ / g_PhdPersp;
    min_x = g_PhdWinCenterX + min_x / zp;
    min_y = g_PhdWinCenterY + min_y / zp;
    max_x = g_PhdWinCenterX + max_x / zp;
    max_y = g_PhdWinCenterY + max_y / zp;

    // clang-format off
    if (min_x > g_PhdWinRight ||
        min_y > g_PhdWinBottom ||
        max_x < g_PhdWinLeft ||
        max_y < g_PhdWinTop
    ) {
        return;
    }
    // clang-format on

    CLAMPL(min_x, g_PhdWinLeft);
    CLAMPL(min_y, g_PhdWinTop);
    CLAMPG(max_x, g_PhdWinRight);
    CLAMPG(max_y, g_PhdWinBottom);
    Output_InsertBackPolygon(min_x, min_y, max_x, max_y);
}

void Room_DrawSingleRoomGeometry(const int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);

    if (room->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(g_CameraUnderwater);
    } else {
        Output_SetupAboveWater(g_CameraUnderwater);
    }

    Matrix_TranslateAbs32(room->pos);
    g_PhdWinLeft = room->bound_left;
    g_PhdWinRight = room->bound_right;
    g_PhdWinTop = room->bound_top;
    g_PhdWinBottom = room->bound_bottom;

    Output_LightRoom(room);
    if (m_Outside > 0 && !(room->flags & RF_INSIDE)) {
        Output_DrawRoom(&room->mesh, true);
    } else {
        if (m_Outside >= 0) {
            Room_Clip(room);
        }
        Output_DrawRoom(&room->mesh, false);
    }
}

void Room_DrawSingleRoomObjects(const int16_t room_num)
{
    ROOM *const room = Room_Get(room_num);

    if (room->flags & RF_UNDERWATER) {
        Output_SetupBelowWater(g_CameraUnderwater);
    } else {
        Output_SetupAboveWater(g_CameraUnderwater);
    }

    room->bound_active = 0;

    Matrix_Push();
    Matrix_TranslateAbs32(room->pos);

    g_PhdWinLeft = room->bound_left;
    g_PhdWinTop = room->bound_top;
    g_PhdWinRight = room->bound_right;
    g_PhdWinBottom = room->bound_bottom;

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
        const int16_t clip = Output_GetObjectBounds(&obj->draw_bounds);
        if (clip != 0) {
            Output_CalculateStaticMeshLight(mesh->pos, mesh->shade, room);
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }
        Matrix_Pop();
    }

    g_PhdWinLeft = 0;
    g_PhdWinTop = 0;
    g_PhdWinRight = g_PhdWinMaxX + 1;
    g_PhdWinBottom = g_PhdWinMaxY + 1;

    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        if (item->status != IS_INVISIBLE) {
            const OBJECT *const obj = Object_Get(item->object_id);
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

    room->bound_left = g_PhdWinMaxX;
    room->bound_top = g_PhdWinMaxY;
    room->bound_right = 0;
    room->bound_bottom = 0;
}

void Room_DrawAllRooms(const int16_t current_room)
{
    ROOM *const room = Room_Get(current_room);
    room->test_left = 0;
    room->test_top = 0;
    room->test_right = g_PhdWinMaxX;
    room->test_bottom = g_PhdWinMaxY;
    room->bound_active = 2;

    g_PhdWinLeft = room->test_left;
    g_PhdWinTop = room->test_top;
    g_PhdWinRight = room->test_right;
    g_PhdWinBottom = room->test_bottom;

    m_BoundRooms[0] = current_room;
    m_BoundStart = 0;
    m_BoundEnd = 1;

    Room_DrawReset();
    m_Outside = room->flags & RF_OUTSIDE;

    if (m_Outside) {
        m_OutsideTop = 0;
        m_OutsideLeft = 0;
        m_OutsideRight = g_PhdWinMaxX;
        m_OutsideBottom = g_PhdWinMaxY;
    } else {
        m_OutsideLeft = g_PhdWinMaxX;
        m_OutsideTop = g_PhdWinMaxY;
        m_OutsideBottom = 0;
        m_OutsideRight = 0;
    }

    g_CameraUnderwater = room->flags & RF_UNDERWATER;
    Room_GetBounds();

    g_MidSort = 0;
    if (m_Outside) {
        g_PhdWinLeft = m_OutsideLeft;
        g_PhdWinRight = m_OutsideRight;
        g_PhdWinBottom = m_OutsideBottom;
        g_PhdWinTop = m_OutsideTop;

        const OBJECT *const skybox = Object_Get(O_SKYBOX);
        if (skybox->loaded) {
            Output_SetupAboveWater(g_CameraUnderwater);
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

    if (Object_Get(O_LARA)->loaded && !(g_LaraItem->flags & IF_ONE_SHOT)) {
        const ROOM *const lara_room = Room_Get(g_LaraItem->room_num);
        if (lara_room->flags & RF_UNDERWATER) {
            Output_SetupBelowWater(g_CameraUnderwater);
        } else {
            Output_SetupAboveWater(g_CameraUnderwater);
        }
        g_MidSort = lara_room->bound_active >> 8;
        if (g_MidSort) {
            g_MidSort--;
        }
        Lara_Draw(g_LaraItem);
    }

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const int16_t room_num = Room_DrawGetRoom(i);
        Room_DrawSingleRoomGeometry(room_num);
    }

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const int16_t room_num = Room_DrawGetRoom(i);
        Room_DrawSingleRoomObjects(room_num);
    }

    Output_SetupAboveWater(false);
}
