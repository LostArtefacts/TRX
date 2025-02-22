#include "game/interpolation.h"

#include "config.h"
#include "game/camera/vars.h"
#include "game/effects.h"
#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/rooms.h"
#include "utils.h"

#include <stdint.h>

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

static bool m_IsEnabled = true;
static double m_Rate = 0.0;

static int32_t M_GetFPS(void)
{
    return g_Config.rendering.fps;
}

static XYZ_32 M_GetItemMaxDelta(const ITEM *const item)
{
    int32_t max_xz = 128;
    int32_t max_y = MAX(128, ABS(item->fall_speed) * 2);
    switch (item->object_id) {
#if TR_VERSION == 1
    case O_BAT:
        max_xz = 0;
        max_y = 0;
        break;
#endif

    case O_DART:
#if TR_VERSION == 2
    case O_BOAT:
    case O_SKIDOO_ARMED:
    case O_SKIDOO_TRACK:
    case O_SKIDOO_FAST:
    case O_SKIDOO_DRIVER:
#endif
        max_xz = 200;
        break;

#if TR_VERSION == 2
    case O_LARA:
    case O_LARA_EXTRA: {
        const int16_t vehicle_item = Lara_GetLaraInfo()->skidoo;
        if (vehicle_item != NO_ITEM) {
            return M_GetItemMaxDelta(Item_Get(vehicle_item));
        }
        break;
    }
#endif

    default:
        break;
    }
    return (XYZ_32) { .x = max_xz, .y = max_y, .z = max_xz };
}

bool Interpolation_IsEnabled(void)
{
    return m_IsEnabled && M_GetFPS() == 60;
}

void Interpolation_Disable(void)
{
    m_IsEnabled = false;
}

void Interpolation_Enable(void)
{
    m_IsEnabled = true;
}

double Interpolation_GetRate(void)
{
    if (!Interpolation_IsEnabled()) {
        return 1.0;
    }
    return m_Rate;
}

void Interpolation_SetRate(double rate)
{
    m_Rate = rate;
}

void Interpolation_Commit(void)
{
    const double ratio = Interpolation_GetRate();

    if (g_Camera.pos.room_num != NO_ROOM) {
        INTERPOLATE(&g_Camera, shift, ratio, 128);
        INTERPOLATE(&g_Camera, pos.x, ratio, 512);
        INTERPOLATE(&g_Camera, pos.y, ratio, 512);
        INTERPOLATE(&g_Camera, pos.z, ratio, 512);
        INTERPOLATE(&g_Camera, target.x, ratio, 512);
        INTERPOLATE(&g_Camera, target.y, ratio, 512);
        INTERPOLATE(&g_Camera, target.z, ratio, 512);

        g_Camera.interp.room_num = g_Camera.pos.room_num;
        Room_GetSector(
            g_Camera.interp.result.pos.x,
            g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
            g_Camera.interp.result.pos.z, &g_Camera.interp.room_num);
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    INTERPOLATE_ROT(&lara->left_arm, rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(&lara->left_arm, rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(&lara->left_arm, rot.z, ratio, DEG_45);
    INTERPOLATE_ROT(&lara->right_arm, rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(&lara->right_arm, rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(&lara->right_arm, rot.z, ratio, DEG_45);
    INTERPOLATE_ROT(lara, torso_rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(lara, torso_rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(lara, torso_rot.z, ratio, DEG_45);
    INTERPOLATE_ROT(lara, head_rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(lara, head_rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(lara, head_rot.z, ratio, DEG_45);

    for (int i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        const XYZ_32 max_delta = M_GetItemMaxDelta(item);
#if TR_VERSION == 1
        const bool is_mounted = false;
#elif TR_VERSION == 2
        const bool is_mounted = i == lara->skidoo;
#endif
        if (((item->flags & IF_KILLED) || item->status == IS_INACTIVE)
            && !is_mounted) {
            COMMIT(item, pos.x);
            COMMIT(item, pos.y);
            COMMIT(item, pos.z);
            COMMIT(item, rot.x);
            COMMIT(item, rot.y);
            COMMIT(item, rot.z);
            continue;
        }
        INTERPOLATE(item, pos.x, ratio, max_delta.x);
        INTERPOLATE(item, pos.y, ratio, max_delta.y);
        INTERPOLATE(item, pos.z, ratio, max_delta.z);
        INTERPOLATE_ROT(item, rot.x, ratio, DEG_45);
        INTERPOLATE_ROT(item, rot.y, ratio, DEG_45);
        INTERPOLATE_ROT(item, rot.z, ratio, DEG_45);
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item != nullptr) {
        const XYZ_32 max_delta = M_GetItemMaxDelta(lara_item);
        INTERPOLATE(lara_item, pos.x, ratio, max_delta.x);
        INTERPOLATE(lara_item, pos.y, ratio, max_delta.y);
        INTERPOLATE(lara_item, pos.z, ratio, max_delta.z);
        INTERPOLATE_ROT(lara_item, rot.x, ratio, DEG_45);
        INTERPOLATE_ROT(lara_item, rot.y, ratio, DEG_45);
        INTERPOLATE_ROT(lara_item, rot.z, ratio, DEG_45);
    }

    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        INTERPOLATE(effect, pos.x, ratio, 128);
        INTERPOLATE(effect, pos.y, ratio, MAX(128, effect->fall_speed * 2));
        INTERPOLATE(effect, pos.z, ratio, 128);
        INTERPOLATE_ROT(effect, rot.x, ratio, DEG_45);
        INTERPOLATE_ROT(effect, rot.y, ratio, DEG_45);
        INTERPOLATE_ROT(effect, rot.z, ratio, DEG_45);
        effect_num = effect->next_active;
    }

    if (Lara_Hair_IsActive()) {
        const XYZ_32 max_delta = M_GetItemMaxDelta(Lara_GetItem());
        for (int i = 0; i < Lara_Hair_GetSegmentCount(); i++) {
            HAIR_SEGMENT *const hair = Lara_Hair_GetSegment(i);
            INTERPOLATE(hair, pos.x, ratio, max_delta.x);
            INTERPOLATE(hair, pos.y, ratio, max_delta.y);
            INTERPOLATE(hair, pos.z, ratio, max_delta.z);
            INTERPOLATE_ROT(hair, rot.x, ratio, DEG_45);
            INTERPOLATE_ROT(hair, rot.y, ratio, DEG_45);
            INTERPOLATE_ROT(hair, rot.z, ratio, DEG_45);
        }
    }
}

void Interpolation_Remember(void)
{
    if (g_Camera.pos.room_num != NO_ROOM) {
        REMEMBER(&g_Camera, shift);
        REMEMBER(&g_Camera, pos.x);
        REMEMBER(&g_Camera, pos.y);
        REMEMBER(&g_Camera, pos.z);
        REMEMBER(&g_Camera, target.x);
        REMEMBER(&g_Camera, target.y);
        REMEMBER(&g_Camera, target.z);
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    REMEMBER(&lara->left_arm, rot.x);
    REMEMBER(&lara->left_arm, rot.y);
    REMEMBER(&lara->left_arm, rot.z);
    REMEMBER(&lara->right_arm, rot.x);
    REMEMBER(&lara->right_arm, rot.y);
    REMEMBER(&lara->right_arm, rot.z);
    REMEMBER(lara, torso_rot.x);
    REMEMBER(lara, torso_rot.y);
    REMEMBER(lara, torso_rot.z);
    REMEMBER(lara, head_rot.x);
    REMEMBER(lara, head_rot.y);
    REMEMBER(lara, head_rot.z);

    for (int i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        REMEMBER(item, pos.x);
        REMEMBER(item, pos.y);
        REMEMBER(item, pos.z);
        REMEMBER(item, rot.x);
        REMEMBER(item, rot.y);
        REMEMBER(item, rot.z);
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item != nullptr) {
        REMEMBER(lara_item, pos.x);
        REMEMBER(lara_item, pos.y);
        REMEMBER(lara_item, pos.z);
        REMEMBER(lara_item, rot.x);
        REMEMBER(lara_item, rot.y);
        REMEMBER(lara_item, rot.z);
    }

    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        REMEMBER(effect, pos.x);
        REMEMBER(effect, pos.y);
        REMEMBER(effect, pos.z);
        REMEMBER(effect, rot.x);
        REMEMBER(effect, rot.y);
        REMEMBER(effect, rot.z);
        effect_num = effect->next_active;
    }

    if (Lara_Hair_IsActive()) {
        for (int i = 0; i < Lara_Hair_GetSegmentCount(); i++) {
            HAIR_SEGMENT *const hair = Lara_Hair_GetSegment(i);
            REMEMBER(hair, pos.x);
            REMEMBER(hair, pos.y);
            REMEMBER(hair, pos.z);
            REMEMBER(hair, rot.x);
            REMEMBER(hair, rot.y);
            REMEMBER(hair, rot.z);
        }
    }
}

void Interpolation_RememberItem(ITEM *item)
{
    item->interp.prev.pos = item->pos;
    item->interp.prev.rot = item->rot;
}
