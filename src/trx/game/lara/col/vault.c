#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/rooms.h>
#include <trx/game/rooms/geometry.h>

// clang-format off
#define M_ANGLE           (30 * DEG_1)                // = 5460
#define M_MIN_CLEARANCE   (-LARA_HEIGHT + STEP_L / 8) // = -730
#define M_HALF_CLICK      (STEP_L / 2)                // = 128
#define M_LOW_WATER_DIST  (-STEP_L * 3)               // = -768
#define M_LADDER_SPEED    (-116)
#define M_MIN_LOW_CLICKS  2
#define M_MAX_LOW_CLICKS  (M_MIN_LOW_CLICKS + 1)      // = 3
#define M_MIN_HIGH_CLICKS (M_MAX_LOW_CLICKS + 1)      // = 4
#define M_MAX_HIGH_CLICKS (M_MIN_HIGH_CLICKS + 3)     // = 7
// clang-format on

typedef struct {
    COLL_SIDE mid;
    COLL_SIDE front;
    COLL_SIDE left;
    COLL_SIDE right;
    int32_t radius;
    bool is_slope;
} M_COLL;

typedef enum {
    M_VAULT_NONE,
    M_VAULT_LOW,
    M_VAULT_HIGH,
    M_VAULT_LADDER,
    M_VAULT_CLIMB_ON,
} M_VAULT_TYPE;

typedef struct {
    M_VAULT_TYPE type;
    int32_t clicks;
    int32_t floor;
} M_CANDIDATE;

static void M_Sample(
    const ITEM *const item, const XYZ_32 pos, int16_t *const room_num,
    COLL_SIDE *const side)
{
    const SECTOR *const sector = Room_GetSector(pos, room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    side->type = Room_GetHeightType();

    if (side->type == HT_BIG_SLOPE
        || Room_IsPathBlocked(
            item->pos, pos, item->room_num, LARA_HEIGHT, LARA_RADIUS)) {
        side->floor = NO_HEIGHT;
        side->ceiling = NO_HEIGHT;
        return;
    }

    side->floor = height;
    if (side->floor != NO_HEIGHT) {
        side->floor -= item->pos.y;
    }

    side->ceiling = Room_GetCeiling(sector, pos);
    if (side->ceiling != NO_HEIGHT) {
        side->ceiling -= item->pos.y - LARA_HEIGHT;
    }
}

static void M_GetCollision(
    const ITEM *const item, const int16_t angle, M_COLL *const coll,
    const int32_t clicks)
{
    XYZ_32 pos = item->pos;
    pos.y -= STEP_L * clicks + M_HALF_CLICK;
    int16_t room_num = item->room_num;
    Room_GetSector(pos, &room_num);

    pos = XYZ_32_OffsetYaw(pos, angle, coll->radius);
    M_Sample(item, pos, &room_num, &coll->front);

    const XYZ_32 left_pos = XYZ_32_OffsetYaw(pos, angle - DEG_90, coll->radius);
    M_Sample(item, left_pos, &room_num, &coll->left);

    const XYZ_32 right_pos =
        XYZ_32_OffsetYaw(pos, angle + DEG_90, coll->radius);
    M_Sample(item, right_pos, &room_num, &coll->right);

    coll->is_slope = ABS(coll->left.floor - coll->right.floor) >= SLOPE_DIF;
}

static bool M_IsLowVault(
    const M_COLL *const coll, const int32_t clicks, const ROOM *const room)
{
    if (clicks < M_MIN_LOW_CLICKS || clicks > M_MAX_LOW_CLICKS) {
        return false;
    }

    if (coll->is_slope) {
        return false;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (room->flags.swamp && lara->water_surface_dist < M_LOW_WATER_DIST) {
        return false;
    }

    if (coll->front.floor - coll->front.ceiling < 0
        || coll->left.floor - coll->left.ceiling < 0
        || coll->right.floor - coll->right.ceiling < 0) {
        return false;
    }

    const int32_t height = STEP_L * clicks;
    return coll->front.floor >= -height - M_HALF_CLICK
        && coll->front.floor <= -height + M_HALF_CLICK;
}

static bool M_HasHeadClearance(const M_COLL *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->climb_status
        || coll->front.floor - coll->mid.ceiling >= M_MIN_CLEARANCE;
}

static bool M_IsHighVault(
    const M_COLL *const coll, const int32_t clicks, const ROOM *const room)
{
    if (clicks < M_MIN_HIGH_CLICKS || clicks > M_MAX_HIGH_CLICKS) {
        return false;
    }

    if (coll->is_slope) {
        return false;
    }

    if (room->flags.swamp) {
        return false;
    }

    return coll->front.floor >= -STEP_L * M_MAX_HIGH_CLICKS - M_HALF_CLICK
        && coll->front.floor <= -STEP_L * M_MIN_HIGH_CLICKS + M_HALF_CLICK;
}

static bool M_IsLadderVault(const M_COLL *const coll, const int32_t clicks)
{
    if (clicks < M_MIN_HIGH_CLICKS || clicks > M_MAX_HIGH_CLICKS) {
        return false;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->climb_status || lara->water_status == LWS_WADE) {
        return false;
    }

    // TODO: If the ceiling in front of Lara is below her and the top floor is
    // above M_MAX_HIGH_CLICKS clicks, Lara does not vault. coll->front.floor in
    // that case will always represent the base floor.
    const int32_t side_max = STEP_L * (M_MAX_HIGH_CLICKS + 1);
    return coll->front.floor <= -STEP_L * M_MAX_HIGH_CLICKS + M_HALF_CLICK
        && coll->left.floor <= -side_max + M_HALF_CLICK
        && coll->right.floor <= -side_max
        && coll->mid.ceiling <= -side_max + M_HALF_CLICK + LARA_HEIGHT;
}

static bool M_IsLadderClimbOn(const M_COLL *const coll, const int32_t clicks)
{
    if (clicks < M_MIN_HIGH_CLICKS || clicks > M_MAX_HIGH_CLICKS) {
        return false;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->climb_status) {
        return false;
    }

    return (coll->front.floor < -STEP_L * M_MIN_HIGH_CLICKS
            || coll->front.ceiling >= LARA_HEIGHT - STEP_L)
        && coll->mid.ceiling <= -STEP_L * (M_MIN_HIGH_CLICKS + 1) + LARA_HEIGHT;
}

static void M_DoLowVault(ITEM *const item, const M_CANDIDATE candidate)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const LARA_ANIMATION_ID anim =
        candidate.clicks == 2 ? LA_CLIMB_2CLICK : LA_CLIMB_3CLICK;
    item->goal_anim_state = LS(LS_STOP);
    item->current_anim_state = LS(LS_PULL_UP);
    Item_SwitchToAnim(item, LA(anim), 0);
    item->pos.y += candidate.floor + STEP_L * candidate.clicks;
    lara->gun_status = LGS_HANDS_BUSY;
}

static void M_DoJumpUpVault(ITEM *const item, const int16_t calc_fall_speed)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    item->goal_anim_state = LS(LS_JUMP_UP);
    item->current_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
    lara->calc_fall_speed = calc_fall_speed;
    Lara_Animate(item);
}

static void M_DoHighVault(ITEM *const item, const M_CANDIDATE candidate)
{
    const int16_t calc_fall_speed =
        -(Math_Sqrt(-2 * GRAVITY * (candidate.floor + 800)) + 3);
    M_DoJumpUpVault(item, calc_fall_speed);
}

static void M_DoLadderVault(ITEM *const item)
{
    M_DoJumpUpVault(item, M_LADDER_SPEED);
}

static void M_DoLadderClimbOn(ITEM *const item, const int16_t angle)
{
    item->goal_anim_state = LS(LS_CLIMB_STANCE);
    item->current_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
    Lara_Animate(item);
    item->rot.y = angle;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_HANDS_BUSY;
    lara->sprinting = false;
}

static M_CANDIDATE M_GetCandidate(
    const ITEM *const item, const COLL_INFO *const coll, const int16_t angle,
    const int32_t clicks)
{
    M_COLL vault_coll = {
        .radius = coll->radius,
        .mid.ceiling = coll->side_mid.ceiling,
        .mid.floor = coll->side_mid.floor,
    };
    M_GetCollision(item, angle, &vault_coll, clicks);
    const ROOM *const room = Room_Get(item->room_num);

    M_VAULT_TYPE type = M_VAULT_NONE;
    if (M_IsLowVault(&vault_coll, clicks, room)) {
        type = M_VAULT_LOW;
    } else if (!M_HasHeadClearance(&vault_coll)) {
        goto finish;
    } else if (M_IsHighVault(&vault_coll, clicks, room)) {
        type = M_VAULT_HIGH;
    } else if (M_IsLadderVault(&vault_coll, clicks)) {
        type = M_VAULT_LADDER;
    } else if (M_IsLadderClimbOn(&vault_coll, clicks)) {
        type = M_VAULT_CLIMB_ON;
    }

finish:
    return (M_CANDIDATE) {
        .type = type,
        .clicks = clicks,
        .floor = vault_coll.front.floor,
    };
}

static bool M_ExecuteCandidate(
    ITEM *const item, COLL_INFO *const coll, const int16_t angle,
    const M_CANDIDATE candidate)
{
    switch (candidate.type) {
    case M_VAULT_LOW:
        M_DoLowVault(item, candidate);
        break;
    case M_VAULT_HIGH:
        M_DoHighVault(item, candidate);
        break;
    case M_VAULT_LADDER:
        M_DoLadderVault(item);
        break;
    case M_VAULT_CLIMB_ON:
        Lara_Col_Shift(coll);
        if (Lara_Col_TestClimbStance(item, coll)) {
            M_DoLadderClimbOn(item, angle);
            return true;
        }
        return false;
    default:
        return false;
    }

    item->rot.y = angle;
    Lara_Col_Shift(coll);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->sprinting = false;
    lara->crouching = false;
    return true;
}

bool Lara_Col_TestVault(ITEM *const item, COLL_INFO *const coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || Lara_GetLaraInfo()->gun_status != LGS_ARMLESS) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, M_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return false;
    }
    const int16_t angle = Math_DirectionToAngle(dir);

    for (int32_t i = M_MIN_LOW_CLICKS; i <= M_MAX_HIGH_CLICKS; i++) {
        const M_CANDIDATE candidate = M_GetCandidate(item, coll, angle, i);
        if (M_ExecuteCandidate(item, coll, angle, candidate)) {
            return true;
        }
    }

    return false;
}
