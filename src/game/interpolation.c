#include "game/interpolation.h"

#include "game/clock.h"
#include "game/items.h"
#include "game/lara/lara_hair.h"
#include "game/phase/phase.h"
#include "game/room.h"
#include "global/vars.h"
#include "math/math_misc.h"
#include "util.h"

#include <assert.h>

#define REMEMBER(target, member) (target)->interp.prev.member = (target)->member

#define COMMIT(target, member) (target)->interp.result.member = (target)->member

#define INTERPOLATE_F(target, member, ratio)                                   \
    (target)->interp.result.member = ((target)->interp.prev.member)            \
        + (((target)->member - ((target)->interp.prev.member)) * (ratio));

#define INTERPOLATE(target, member, ratio, max_diff)                           \
    if (ABS(((target)->member) - ((target)->interp.prev.member))               \
        >= (max_diff)) {                                                       \
        COMMIT((target), member);                                              \
    } else {                                                                   \
        INTERPOLATE_F(target, member, ratio);                                  \
    }

#define INTERPOLATE_ROT(target, member, ratio, max_diff)                       \
    if (!Math_AngleInCone(                                                     \
            (target)->member, (target)->interp.prev.member, (max_diff))) {     \
        COMMIT((target), member);                                              \
    } else {                                                                   \
        INTERPOLATE_ROT_F(target, member, ratio);                              \
    }

#define INTERPOLATE_ROT_F(target, member, ratio)                               \
    (target)->interp.result.member = Math_AngleMean(                           \
        (target)->interp.prev.member, (target)->member, (ratio))

static bool m_IsEnabled = false;

bool Interpolation_IsEnabled(void)
{
    return m_IsEnabled
        && (Phase_Get() != PHASE_INVENTORY && g_Config.rendering.fps == 60);
}

void Interpolation_Disable(void)
{
    m_IsEnabled = false;
}

void Interpolation_Enable(void)
{
    m_IsEnabled = true;
}

void Interpolation_Commit(void)
{
    const double ratio = Clock_GetTickProgress();

    if (g_Camera.pos.room_number != NO_ROOM) {
        INTERPOLATE(&g_Camera, shift, ratio, 128);
        INTERPOLATE(&g_Camera, pos.x, ratio, 512);
        INTERPOLATE(&g_Camera, pos.y, ratio, 512);
        INTERPOLATE(&g_Camera, pos.z, ratio, 512);
        INTERPOLATE(&g_Camera, target.x, ratio, 512);
        INTERPOLATE(&g_Camera, target.y, ratio, 512);
        INTERPOLATE(&g_Camera, target.z, ratio, 512);

        g_Camera.interp.result.room_num = g_Camera.interp.prev.room_num;
        Room_GetFloor(
            g_Camera.interp.result.pos.x,
            g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
            g_Camera.interp.result.pos.z, &g_Camera.interp.result.room_num);
    }

    INTERPOLATE_ROT(&g_Lara.left_arm, rot.x, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara.left_arm, rot.y, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara.left_arm, rot.z, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara.right_arm, rot.x, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara.right_arm, rot.y, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara.right_arm, rot.z, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, torso_rot.x, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, torso_rot.y, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, torso_rot.z, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, head_rot.x, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, head_rot.y, ratio, PHD_45);
    INTERPOLATE_ROT(&g_Lara, head_rot.z, ratio, PHD_45);

    for (int i = 0; i < Item_GetTotalCount(); i++) {
        ITEM_INFO *item = &g_Items[i];
        if ((item->flags & IF_KILLED_ITEM) || item->status == IS_NOT_ACTIVE
            || item->object_number == O_BAT) {
            COMMIT(item, pos.x);
            COMMIT(item, pos.y);
            COMMIT(item, pos.z);
            COMMIT(item, rot.x);
            COMMIT(item, rot.y);
            COMMIT(item, rot.z);
            continue;
        }

        const int32_t max_xz = item->object_number == O_DARTS ? 200 : 128;
        const int32_t max_y = MAX(128, item->fall_speed * 2);
        INTERPOLATE(item, pos.x, ratio, max_xz);
        INTERPOLATE(item, pos.y, ratio, max_y);
        INTERPOLATE(item, pos.z, ratio, max_xz);
        INTERPOLATE_ROT(item, rot.x, ratio, PHD_45);
        INTERPOLATE_ROT(item, rot.y, ratio, PHD_45);
        INTERPOLATE_ROT(item, rot.z, ratio, PHD_45);
    }

    if (g_LaraItem) {
        INTERPOLATE(g_LaraItem, pos.x, ratio, 128);
        INTERPOLATE(
            g_LaraItem, pos.y, ratio, MAX(128, g_LaraItem->fall_speed * 2));
        INTERPOLATE(g_LaraItem, pos.z, ratio, 128);
        INTERPOLATE_ROT(g_LaraItem, rot.x, ratio, PHD_45);
        INTERPOLATE_ROT(g_LaraItem, rot.y, ratio, PHD_45);
        INTERPOLATE_ROT(g_LaraItem, rot.z, ratio, PHD_45);
    }

    if (Lara_Hair_IsActive()) {
        for (int i = 0; i < Lara_Hair_GetSegmentCount(); i++) {
            HAIR_SEGMENT *hair = Lara_Hair_GetSegment(i);
            INTERPOLATE(hair, pos.x, ratio, 128);
            INTERPOLATE(hair, pos.y, ratio, 128);
            INTERPOLATE(hair, pos.z, ratio, 128);
            INTERPOLATE_ROT(hair, rot.x, ratio, PHD_45);
            INTERPOLATE_ROT(hair, rot.y, ratio, PHD_45);
            INTERPOLATE_ROT(hair, rot.z, ratio, PHD_45);
        }
    }
}

void Interpolation_Remember(void)
{
    if (g_Camera.pos.room_number != NO_ROOM) {
        REMEMBER(&g_Camera, shift);
        REMEMBER(&g_Camera, pos.x);
        REMEMBER(&g_Camera, pos.y);
        REMEMBER(&g_Camera, pos.z);
        REMEMBER(&g_Camera, target.x);
        REMEMBER(&g_Camera, target.y);
        REMEMBER(&g_Camera, target.z);
        g_Camera.interp.prev.room_num = g_Camera.pos.room_number;
    }

    REMEMBER(&g_Lara.left_arm, rot.x);
    REMEMBER(&g_Lara.left_arm, rot.y);
    REMEMBER(&g_Lara.left_arm, rot.z);
    REMEMBER(&g_Lara.right_arm, rot.x);
    REMEMBER(&g_Lara.right_arm, rot.y);
    REMEMBER(&g_Lara.right_arm, rot.z);
    REMEMBER(&g_Lara, torso_rot.x);
    REMEMBER(&g_Lara, torso_rot.y);
    REMEMBER(&g_Lara, torso_rot.z);
    REMEMBER(&g_Lara, head_rot.x);
    REMEMBER(&g_Lara, head_rot.y);
    REMEMBER(&g_Lara, head_rot.z);

    for (int i = 0; i < Item_GetTotalCount(); i++) {
        ITEM_INFO *item = &g_Items[i];
        REMEMBER(item, pos.x);
        REMEMBER(item, pos.y);
        REMEMBER(item, pos.z);
        REMEMBER(item, rot.x);
        REMEMBER(item, rot.y);
        REMEMBER(item, rot.z);
    }

    if (g_LaraItem) {
        REMEMBER(g_LaraItem, pos.x);
        REMEMBER(g_LaraItem, pos.y);
        REMEMBER(g_LaraItem, pos.z);
        REMEMBER(g_LaraItem, rot.x);
        REMEMBER(g_LaraItem, rot.y);
        REMEMBER(g_LaraItem, rot.z);
    }

    if (Lara_Hair_IsActive()) {
        for (int i = 0; i < Lara_Hair_GetSegmentCount(); i++) {
            HAIR_SEGMENT *hair = Lara_Hair_GetSegment(i);
            REMEMBER(hair, pos.x);
            REMEMBER(hair, pos.y);
            REMEMBER(hair, pos.z);
            REMEMBER(hair, rot.x);
            REMEMBER(hair, rot.y);
            REMEMBER(hair, rot.z);
        }
    }
}

void Interpolation_RememberItem(ITEM_INFO *item)
{
    item->interp.prev.pos = item->pos;
    item->interp.prev.rot = item->rot;
}
