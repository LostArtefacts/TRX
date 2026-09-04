#include <trx/game/interpolation.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/game/state.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/photo_mode.h>
#include <trx/game/rooms.h>
#include <trx/game/shell.h>
#include <trx/game/viewport.h>

#include <stdint.h>

#define CAM_MAX_DELTA (STEP_L * 3 / 2)
#define FOV_MAX_DELTA (DEG_1 * 5)

#define REMEMBER(target, member) (target)->interp.prev.member = (target)->member

#define COMMIT(target, member) (target)->interp.result.member = (target)->member

#define DIFF(target, member)                                                   \
    (ABS(((target)->member) - ((target)->interp.prev.member)))

#define INTERPOLATE_F(target, member, ratio)                                   \
    (target)->interp.result.member = ((target)->interp.prev.member)            \
        + (((target)->member - ((target)->interp.prev.member)) * (ratio));

#define INTERPOLATE(target, member, ratio, max_diff)                           \
    if (DIFF((target), member) >= (max_diff)) {                                \
        COMMIT((target), member);                                              \
    } else {                                                                   \
        INTERPOLATE_F(target, member, ratio);                                  \
    }

#define INTERPOLATE_ROT_F(target, member, ratio)                               \
    (target)->interp.result.member = Math_AngleMean(                           \
        (target)->interp.prev.member, (target)->member, (ratio))

#define INTERPOLATE_ROT(target, member, ratio, max_diff)                       \
    if (!Math_AngleInCone(                                                     \
            (target)->member, (target)->interp.prev.member, (max_diff))) {     \
        COMMIT((target), member);                                              \
    } else {                                                                   \
        INTERPOLATE_ROT_F(target, member, ratio);                              \
    }

static bool m_IsEnabled = true;
static CAMERA_TYPE m_PrevCameraType = CAM_CHASE;
static double m_Rate = 0.0;
static double m_WorldRate = 0.0;
static double m_CameraRate = 0.0;

static int32_t M_GetFPS(void)
{
    return g_Config.rendering.fps;
}

static XYZ_32 M_GetItemMaxDelta(const ITEM *const item)
{
    int32_t max_xz = 128;
    int32_t max_y = MAX(128, ABS(item->fall_speed) * 2);
    switch (item->object_id) {
    case O_BAT:
        max_xz = 0;
        max_y = 0;
        break;

    case O_DART:
    case O_DISC:
    case O_BOAT:
    case O_RIB:
    case O_SKIDOO_ARMED:
    case O_SKIDOO_TRACK:
    case O_SKIDOO_FAST:
    case O_SKIDOO_DRIVER:
    case O_GRENADE:
        max_xz = 200;
        break;

    case O_QUAD_BIKE:
    case O_KAYAK:
    case O_UPV:
    case O_TRAIN:
        max_xz = 300;
        break;

    case O_MINE_CART:
        max_xz = 512;
        max_y = 200;
        break;

    case O_POISON_DART:
    case O_ROCKET:
    case O_HEAVY_ROCKET:
        max_xz = 1000;
        max_y = 200;
        break;

    case O_SOPHIA_LASER_BOLT:
        max_xz = 1000;
        max_y = 1000;
        break;

    case O_HARPOON_BOLT:
        max_xz = 300;
        max_y = 200;
        break;

    case O_LARA:
    case O_LARA_EXTRA: {
        const ITEM *const vehicle = Lara_Vehicle_GetItem();
        if (vehicle == nullptr || vehicle == item
            || vehicle->object_id == O_LARA
            || vehicle->object_id == O_LARA_EXTRA) {
            break;
        }
        return M_GetItemMaxDelta(vehicle);
    }

    default:
        break;
    }
    return (XYZ_32) { .x = max_xz, .y = max_y, .z = max_xz };
}

static XYZ_32 M_GetEffectMaxDelta(const EFFECT *const effect)
{
    int32_t max_xz = 128;
    int32_t max_y = MAX(128, effect->fall_speed * 2);
    switch (effect->object_id) {
    case O_NATLA_GUN:
    case O_MISSILE_ATLANTEAN_BOMB:
        max_xz = 220;
        break;
    case O_MISSILE_ATLANTEAN_SHARD:
        max_xz = 250;
        break;
    case O_MISSILE_FLAME:
        max_xz = 200;
        break;
    case O_MISSILE_KNIFE:
    case O_MISSILE_HARPOON:
        max_xz = 150;
        break;

    default:
        break;
    }

    return (XYZ_32) { .x = max_xz, .y = max_y, .z = max_xz };
}

static void M_RememberCamera(void)
{
    m_PrevCameraType = g_Camera.type;
    if (g_Camera.type == CAM_PHOTO_MODE) {
        g_Camera.interp.prev.rot = Camera_PhotoMode_GetRot();
    }
    REMEMBER(&g_Camera, fov);
    REMEMBER(&g_Camera, shift);
    REMEMBER(&g_Camera, pos.x);
    REMEMBER(&g_Camera, pos.y);
    REMEMBER(&g_Camera, pos.z);
    REMEMBER(&g_Camera, target.x);
    REMEMBER(&g_Camera, target.y);
    REMEMBER(&g_Camera, target.z);
}

static void M_CommitCamera(void)
{
    COMMIT(&g_Camera, fov);
    COMMIT(&g_Camera, shift);
    COMMIT(&g_Camera, pos.x);
    COMMIT(&g_Camera, pos.y);
    COMMIT(&g_Camera, pos.z);
    COMMIT(&g_Camera, target.x);
    COMMIT(&g_Camera, target.y);
    COMMIT(&g_Camera, target.z);
}

static void M_InterpolateCamera(const double ratio)
{
    INTERPOLATE_F(&g_Camera, shift, ratio);
    INTERPOLATE_F(&g_Camera, pos.x, ratio);
    INTERPOLATE_F(&g_Camera, pos.y, ratio);
    INTERPOLATE_F(&g_Camera, pos.z, ratio);
    INTERPOLATE_F(&g_Camera, target.x, ratio);
    INTERPOLATE_F(&g_Camera, target.y, ratio);
    INTERPOLATE_F(&g_Camera, target.z, ratio);
}

static void M_RememberLara(LARA_INFO *const lara)
{
    ASSERT(lara != nullptr);
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
}

static void M_InterpolateLara(const double ratio, LARA_INFO *const lara)
{
    ASSERT(lara != nullptr);
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
}

static void M_CommitLara(LARA_INFO *const lara)
{
    ASSERT(lara != nullptr);
    COMMIT(&lara->left_arm, rot.x);
    COMMIT(&lara->left_arm, rot.y);
    COMMIT(&lara->left_arm, rot.z);
    COMMIT(&lara->right_arm, rot.x);
    COMMIT(&lara->right_arm, rot.y);
    COMMIT(&lara->right_arm, rot.z);
    COMMIT(lara, torso_rot.x);
    COMMIT(lara, torso_rot.y);
    COMMIT(lara, torso_rot.z);
    COMMIT(lara, head_rot.x);
    COMMIT(lara, head_rot.y);
    COMMIT(lara, head_rot.z);
}

static void M_RememberBraidSegment(HAIR_SEGMENT *const segment)
{
    ASSERT(segment != nullptr);
    REMEMBER(segment, pos.x);
    REMEMBER(segment, pos.y);
    REMEMBER(segment, pos.z);
    REMEMBER(segment, rot.x);
    REMEMBER(segment, rot.y);
    REMEMBER(segment, rot.z);
}

static void M_InterpolateBraidSegment(
    HAIR_SEGMENT *const segment, const double ratio, const XYZ_32 max_delta)
{
    ASSERT(segment != nullptr);
    INTERPOLATE(segment, pos.x, ratio, max_delta.x);
    INTERPOLATE(segment, pos.y, ratio, max_delta.y);
    INTERPOLATE(segment, pos.z, ratio, max_delta.z);
    INTERPOLATE_ROT(segment, rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(segment, rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(segment, rot.z, ratio, DEG_45);
}

static void M_CommitBraidSegment(HAIR_SEGMENT *const segment)
{
    ASSERT(segment != nullptr);
    COMMIT(segment, pos.x);
    COMMIT(segment, pos.y);
    COMMIT(segment, pos.z);
    COMMIT(segment, rot.x);
    COMMIT(segment, rot.y);
    COMMIT(segment, rot.z);
}

static void M_RememberBraid(void)
{
    for (int32_t i = 0; i < Lara_Hair_GetBraidCount(); i++) {
        for (int32_t j = 0; j < Lara_Hair_GetSegmentCount(); j++) {
            M_RememberBraidSegment(Lara_Hair_GetSegment(i, j));
        }
    }
}

static void M_CommitBraid(void)
{
    for (int32_t i = 0; i < Lara_Hair_GetBraidCount(); i++) {
        for (int32_t j = 0; j < Lara_Hair_GetSegmentCount(); j++) {
            M_CommitBraidSegment(Lara_Hair_GetSegment(i, j));
        }
    }
}

static void M_InterpolateBraid(const double ratio, ITEM *const lara_item)
{
    ASSERT(lara_item != nullptr);
    const XYZ_32 max_delta = M_GetItemMaxDelta(lara_item);
    for (int32_t i = 0; i < Lara_Hair_GetBraidCount(); i++) {
        for (int32_t j = 0; j < Lara_Hair_GetSegmentCount(); j++) {
            M_InterpolateBraidSegment(
                Lara_Hair_GetSegment(i, j), ratio, max_delta);
        }
    }
}

static void M_CommitItem(ITEM *const item)
{
    COMMIT(item, floor);
    COMMIT(item, pos.x);
    COMMIT(item, pos.y);
    COMMIT(item, pos.z);
    COMMIT(item, rot.x);
    COMMIT(item, rot.y);
    COMMIT(item, rot.z);
}

static void M_InterpolateItem(ITEM *const item, const double ratio)
{
    const XYZ_32 max_delta = M_GetItemMaxDelta(item);
    INTERPOLATE(item, floor, ratio, max_delta.y);
    INTERPOLATE(item, pos.x, ratio, max_delta.x);
    INTERPOLATE(item, pos.y, ratio, max_delta.y);
    INTERPOLATE(item, pos.z, ratio, max_delta.z);
    INTERPOLATE_ROT(item, rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(item, rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(item, rot.z, ratio, DEG_45);
}

static void M_RememberItems(void)
{
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        Interpolation_RememberItem(Item_Get(i));
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item != nullptr) {
        Interpolation_RememberItem(lara_item);
    }
}

static void M_InterpolateItems(const double ratio)
{
    const int16_t lara_vehicle_num = Lara_Vehicle_GetIndex();
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if ((item->is_destroyed || Item_IsInactive(item))
            && i != lara_vehicle_num
            && XYZ_32_AreEquivalent(item->pos, item->interp.prev.pos)) {
            M_CommitItem(item);
        } else {
            M_InterpolateItem(item, ratio);
        }
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item != nullptr) {
        M_InterpolateItem(lara_item, ratio);
    }
}

static void M_RememberEffect(EFFECT *const effect)
{
    ASSERT(effect != nullptr);
    REMEMBER(effect, pos.x);
    REMEMBER(effect, pos.y);
    REMEMBER(effect, pos.z);
    REMEMBER(effect, rot.x);
    REMEMBER(effect, rot.y);
    REMEMBER(effect, rot.z);
    effect->interp.is_new = false;
}

static void M_InterpolateEffect(const double ratio, EFFECT *const effect)
{
    ASSERT(effect != nullptr);
    if (effect->interp.is_new) {
        COMMIT(effect, pos.x);
        COMMIT(effect, pos.y);
        COMMIT(effect, pos.z);
        COMMIT(effect, rot.x);
        COMMIT(effect, rot.y);
        COMMIT(effect, rot.z);
        return;
    }

    const XYZ_32 max_delta = M_GetEffectMaxDelta(effect);
    INTERPOLATE(effect, pos.x, ratio, max_delta.x);
    INTERPOLATE(effect, pos.y, ratio, max_delta.y);
    INTERPOLATE(effect, pos.z, ratio, max_delta.z);
    INTERPOLATE_ROT(effect, rot.x, ratio, DEG_45);
    INTERPOLATE_ROT(effect, rot.y, ratio, DEG_45);
    INTERPOLATE_ROT(effect, rot.z, ratio, DEG_45);
}

static void M_RememberEffects(void)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        M_RememberEffect(effect);
        effect_num = effect->next_active;
    }
}

static void M_InterpolateEffects(const double ratio)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        M_InterpolateEffect(ratio, effect);
        effect_num = effect->next_active;
    }
}

void Interpolation_Enable(void)
{
    m_IsEnabled = true;
}

void Interpolation_Disable(void)
{
    m_IsEnabled = false;
}

bool Interpolation_IsEnabled(void)
{
    return m_IsEnabled;
}

bool Interpolation_IsActive(void)
{
    return m_IsEnabled && M_GetFPS() == 60 && !Shell_IsExiting();
}

double Interpolation_GetWorldRate(void)
{
    // Photo mode steps the world itself and keeps its own rate, so the frozen
    // gameplay state must not force the rate back to 1.0.
    if (!Interpolation_IsActive()
        || (!Game_IsPlaying() && !PhotoMode_IsActive())) {
        return 1.0;
    }
    return m_WorldRate;
}

double Interpolation_GetCameraRate(void)
{
    if (!Interpolation_IsActive()) {
        return 1.0;
    }
    return m_CameraRate;
}

double Interpolation_GetRate(void)
{
    return m_Rate;
}

void Interpolation_SetRate(double rate)
{
    m_Rate = rate;
}

void Interpolation_Remember(void)
{
    if (g_Camera.pos.room_num != NO_ROOM) {
        M_RememberCamera();
    }

    if (g_Camera.type == CAM_PHOTO_MODE) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara != nullptr) {
        M_RememberLara(lara);
    }

    if (Lara_Hair_IsActive()) {
        M_RememberBraid();
    }
    M_RememberItems();
    M_RememberEffects();
}

void Interpolation_RememberItem(ITEM *const item)
{
    REMEMBER(item, floor);
    REMEMBER(item, pos.x);
    REMEMBER(item, pos.y);
    REMEMBER(item, pos.z);
    REMEMBER(item, rot.x);
    REMEMBER(item, rot.y);
    REMEMBER(item, rot.z);
    item->prev_frame_num = item->frame_num;
}

void Interpolation_Interpolate(void)
{
    if (g_Camera.type != CAM_PHOTO_MODE) {
        m_WorldRate = m_Rate;
    }
    m_CameraRate = m_Rate;

    if (g_Camera.pos.room_num != NO_ROOM) {
        // While the binocular camera stays active, the target orbits Lara at a
        // large radius, so per-tick deltas routinely exceed CAM_MAX_DELTA even
        // though the motion is a smooth rotation — keep interpolating.
        const bool sustained_binoculars = g_Camera.type == CAM_BINOCULARS
            && m_PrevCameraType == CAM_BINOCULARS;
        g_Camera.fov = Viewport_GetEffectiveFOV();
        INTERPOLATE(&g_Camera, fov, m_CameraRate, FOV_MAX_DELTA);

        const bool snap = !sustained_binoculars
            && (DIFF(&g_Camera, shift) >= 128
                || DIFF(&g_Camera, pos.x) >= CAM_MAX_DELTA
                || DIFF(&g_Camera, pos.y) >= CAM_MAX_DELTA
                || DIFF(&g_Camera, pos.z) >= CAM_MAX_DELTA
                || DIFF(&g_Camera, target.x) >= CAM_MAX_DELTA
                || DIFF(&g_Camera, target.y) >= CAM_MAX_DELTA
                || DIFF(&g_Camera, target.z) >= CAM_MAX_DELTA);
        if (snap) {
            M_CommitCamera();
        } else {
            M_InterpolateCamera(m_CameraRate);
        }

        if (g_Camera.type == CAM_PHOTO_MODE) {
            const XYZ_16 rot = Camera_PhotoMode_GetRot();
            g_Camera.interp.result.rot =
                snap || m_PrevCameraType != CAM_PHOTO_MODE
                ? rot
                : Matrix_SlerpRot16(
                      g_Camera.interp.prev.rot, rot, m_CameraRate);
        }

        g_Camera.interp.room_num = g_Camera.pos.room_num;
        Camera_ClampInterpResult();
    }

    if (g_Camera.type == CAM_PHOTO_MODE) {
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    if (lara != nullptr) {
        M_InterpolateLara(m_WorldRate, lara);
    }
    M_InterpolateItems(m_WorldRate);
    M_InterpolateEffects(m_WorldRate);
    if (lara_item != nullptr && Lara_Hair_IsActive()) {
        M_InterpolateBraid(m_WorldRate, lara_item);
    }
}

void Interpolation_CommitLara(void)
{
    ITEM *const lara_item = Lara_GetItem();
    if (lara_item != nullptr) {
        M_CommitItem(lara_item);
    }
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara != nullptr) {
        M_CommitLara(lara);
    }
    M_CommitBraid();
}

void Interpolation_CommitBraid(void)
{
    M_CommitBraid();
}
