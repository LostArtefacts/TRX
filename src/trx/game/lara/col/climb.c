#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/util.h>
#include <trx/game/rooms.h>
#include <trx/game/rooms/geometry.h>
#include <trx/version.h>

// clang-format off
#define M_CLIMB_SHIFT            70
#define M_CLIMB_HANG             900
#define M_CLIMB_WIDTH_LEFT       (g_TRVersion >= 4 ? 120 : 80)
#define M_CLIMB_WIDTH_RIGHT      120
#define M_CLIMB_HEIGHT           (WALL_L / 2) // = 512
#define M_LF_HANG                21
#define M_LF_STOP_HANG           9
#define M_LF_CLIMB_L_SHIFT_START 28
#define M_LF_CLIMB_L_SHIFT_END   29
#define M_LF_CLIMB_R_SHIFT       57
#define M_LEDGE_JUMP_PUSH_HEIGHT (STEP_L - 16)                    // = 240
#define M_LEDGE_JUMP_HEIGHT_UP   (LARA_HEIGHT + (STEP_L * 3) / 8) // = 858
#define M_LEDGE_JUMP_HEIGHT_BACK (LARA_HEIGHT - (STEP_L * 5) / 4) // = 442
#define M_HANG_SHIFT             (g_TRVersion >= 3 ? 4 : 2)
#define M_CLIMB_WIDTH_CORNER     120
#define M_CORNER_SIDE_SHIFT      16
#define M_CORNER_FRONT_DIST      (LARA_RADIUS + M_CORNER_SIDE_SHIFT) // = 116
#define M_CORNER_MAX_DROP        (STEP_L * 3)                        // = 768
// clang-format on

typedef enum {
    // clang-format off
    CLIMB_RESULT_CRAWL = -2,
    CLIMB_RESULT_NEG  = -1,
    CLIMB_RESULT_NONE = 0,
    CLIMB_RESULT_POS  = 1,
    // clang-format on
} M_CLIMB_RESULT;

static int32_t M_GetDestinationClearance(const ITEM *const item)
{
    XYZ_32 pos = {
        .x = 0,
        .y = -M_CLIMB_HEIGHT,
        .z = 0,
    };
    Lara_GetJointAbsPosition(&pos, LM_HAND_R);
    int16_t room_num = item->room_num;
    Room_GetSector(pos, &room_num);

    pos = XYZ_32_OffsetYaw(pos, item->rot.y, LARA_RADIUS * 2);
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const int32_t ceiling = Room_GetCeiling(sector, pos);

    if (height == NO_HEIGHT || ceiling == NO_HEIGHT) {
        return NO_HEIGHT;
    }
    return ABS(height - ceiling);
}

static M_CLIMB_RESULT M_TestClimbPos(
    const ITEM *const item, const int32_t front, const int32_t right,
    const int32_t origin, const int32_t item_height, int32_t *const shift)
{
    const int32_t y = item->pos.y + origin;
    int32_t x;
    int32_t z;
    int32_t x_front = 0;
    int32_t z_front = 0;

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        x = item->pos.x + right;
        z = item->pos.z + front;
        z_front = M_HANG_SHIFT;
        break;

    case DIR_EAST:
        x = item->pos.x + front;
        z = item->pos.z - right;
        x_front = M_HANG_SHIFT;
        break;

    case DIR_SOUTH:
        x = item->pos.x - right;
        z = item->pos.z - front;
        z_front = -M_HANG_SHIFT;
        break;

    case DIR_WEST:
        x = item->pos.x - front;
        z = item->pos.z + right;
        x_front = -M_HANG_SHIFT;
        break;

    default:
        x = front;
        z = front;
        break;
    }

    *shift = 0;
    bool hang = true;
    if (!Lara_GetLaraInfo()->climb_status) {
        return CLIMB_RESULT_NONE;
    }

    int16_t room_num = item->room_num;
    XYZ_32 sample_pos = { x, y - 128, z };
    const SECTOR *sector = Room_GetSector(sample_pos, &room_num);
    sample_pos.y = y;
    int32_t height = Room_GetHeight(sector, sample_pos);
    if (height == NO_HEIGHT) {
        return CLIMB_RESULT_NONE;
    }

    height -= y + item_height + STEP_L / 2;
    if (height < -M_CLIMB_SHIFT) {
        return CLIMB_RESULT_NONE;
    }
    if (height < 0) {
        *shift = height;
    }

    int32_t ceiling = Room_GetCeiling(sector, sample_pos) - y;
    if (ceiling > M_CLIMB_SHIFT) {
        return CLIMB_RESULT_NONE;
    }
    if (ceiling > 0) {
        if (*shift) {
            return CLIMB_RESULT_NONE;
        }
        *shift = ceiling;
    }

    if (item_height + height < M_CLIMB_HANG) {
        hang = false;
    }

    const int32_t x2 = x + x_front;
    const int32_t z2 = z + z_front;
    sample_pos.x = x2;
    sample_pos.y = y;
    sample_pos.z = z2;
    sector = Room_GetSector(sample_pos, &room_num);
    height = Room_GetHeight(sector, sample_pos);
    if (height != NO_HEIGHT) {
        height -= y;
    }

    if (height > M_CLIMB_SHIFT) {
        ceiling = Room_GetCeiling(sector, sample_pos) - y;
        if (ceiling >= M_CLIMB_HEIGHT) {
            return CLIMB_RESULT_POS;
        }

        if (ceiling > M_CLIMB_HEIGHT - M_CLIMB_SHIFT) {
            if (*shift > 0) {
                return hang ? CLIMB_RESULT_NEG : CLIMB_RESULT_NONE;
            }
            *shift = ceiling - M_CLIMB_HEIGHT;
            return CLIMB_RESULT_POS;
        }

        if (ceiling > 0) {
            return hang ? CLIMB_RESULT_NEG : CLIMB_RESULT_NONE;
        }

        if (ceiling > -M_CLIMB_SHIFT && hang && *shift <= 0) {
            if (*shift > ceiling) {
                *shift = ceiling;
            }

            return CLIMB_RESULT_NEG;
        }

        return CLIMB_RESULT_NONE;
    }

    if (height > 0) {
        if (*shift < 0) {
            return CLIMB_RESULT_NONE;
        }
        if (height > *shift) {
            *shift = height;
        }
    }

    room_num = item->room_num;
    sample_pos = (XYZ_32) { x, item_height + y, z };
    Room_GetSector(sample_pos, &room_num);
    sample_pos.x = x2;
    sample_pos.z = z2;
    sector = Room_GetSector(sample_pos, &room_num);
    ceiling = Room_GetCeiling(sector, sample_pos);
    if (ceiling == NO_HEIGHT) {
        return CLIMB_RESULT_POS;
    }

    ceiling -= y;
    if (ceiling <= height) {
        return CLIMB_RESULT_POS;
    }

    if (ceiling >= M_CLIMB_HEIGHT) {
        return CLIMB_RESULT_POS;
    }

    if (ceiling > M_CLIMB_HEIGHT - M_CLIMB_SHIFT) {
        if (*shift > 0) {
            return hang ? CLIMB_RESULT_NEG : CLIMB_RESULT_NONE;
        }
        *shift = ceiling - M_CLIMB_HEIGHT;
        return CLIMB_RESULT_POS;
    }

    return hang ? CLIMB_RESULT_NEG : CLIMB_RESULT_NONE;
}

static bool M_TestHangStop(
    const ITEM *const item, const COLL_INFO *const coll, const bool front_floor,
    int32_t *const height_diff)
{
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    *height_diff = coll->side_front.floor - bounds->min.y;
    return ABS(coll->side_left2.floor - coll->side_right2.floor) >= SLOPE_DIF
        || coll->side_mid.ceiling >= 0 || coll->coll_type != COLL_FRONT
        || front_floor || coll->hit_static || *height_diff < -SLOPE_DIF
        || *height_diff > SLOPE_DIF;
}

static bool M_CanHangSideways(
    ITEM *const item, COLL_INFO *const coll, const int16_t angle)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const XYZ_32 old_pos = item->pos;
    const XYZ_32 old_coll_pos = coll->old_pos;
    lara->move_angle = item->rot.y + angle;

    int32_t x = item->pos.x;
    int32_t z = item->pos.z;
    switch (Math_GetDirection(lara->move_angle)) {
    case DIR_NORTH:
        z += M_CORNER_SIDE_SHIFT;
        break;
    case DIR_EAST:
        x += M_CORNER_SIDE_SHIFT;
        break;
    case DIR_SOUTH:
        z -= M_CORNER_SIDE_SHIFT;
        break;
    case DIR_WEST:
        x -= M_CORNER_SIDE_SHIFT;
        break;
    default:
        break;
    }

    item->pos.x = x;
    item->pos.z = z;
    coll->old_pos.y = item->pos.y;
    const bool blocked = Lara_Col_HangTest(item, coll);
    if (blocked) {
        coll->old_pos.y = old_coll_pos.y;
    }
    item->pos.x = old_pos.x;
    item->pos.z = old_pos.z;
    lara->move_angle = item->rot.y + angle;
    return !blocked;
}

static bool M_IsValidHangPos(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Lara_FloorFront(item, lara->move_angle, LARA_RADIUS) < 200) {
        return false;
    }

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        item->pos.z += M_HANG_SHIFT;
        break;
    case DIR_EAST:
        item->pos.x += M_HANG_SHIFT;
        break;
    case DIR_SOUTH:
        item->pos.z -= M_HANG_SHIFT;
        break;
    case DIR_WEST:
        item->pos.x -= M_HANG_SHIFT;
        break;
    default:
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEP_L * 2;
    coll->bad_ceiling = 0;
    lara->move_angle = item->rot.y;
    Lara_Col_GetInfo(item, coll);

    if (!(coll->side_mid.ceiling < 0 && coll->coll_type == COLL_FRONT
          && !coll->hit_static
          && ABS(coll->side_front.floor - coll->side_right2.floor)
              < SLOPE_DIF)) {
        return false;
    }

    // A laterally sloping ledge cannot be grabbed, matching an ordinary hang
    // grab (Lara_Col_HangTest rejects side_left2/side_right2 tilt). A side that
    // drops away - e.g. the open half of an inner corner - is fine, so only the
    // case where both ledge ends stay at grab height yet tilt across is
    // rejected.
    const bool left_level =
        ABS(coll->side_front.floor - coll->side_left2.floor) < SLOPE_DIF;
    if (left_level
        && ABS(coll->side_left2.floor - coll->side_right2.floor) >= SLOPE_DIF) {
        return false;
    }

    return true;
}

static LADDER_DIRECTION M_GetClimbFlags(
    const int32_t x, const int32_t z, const int16_t room_num)
{
    // The climb trigger lives in the floordata of the column's floor sector,
    // so it must be resolved at floor level (as Room_TestTriggers does), not
    // at Lara's airborne Y, which in vertically stacked rooms resolves to a
    // sector without the trigger.
    int16_t probe_room = room_num;
    const SECTOR *const sector =
        Room_GetSector((XYZ_32) { x, MAX_HEIGHT, z }, &probe_room);
    return sector->ladder;
}

// The corner destination position formulas below mirror Lara's position
// within her sector onto the perpendicular face she would hold after the
// turn. They come from the TR4 engine, which only spells out the
// right-hand variants; the left-hand variants are the same formulas with
// the direction rotated by a quadrant, which is what M_GetCornerDirection
// computes.
static DIRECTION M_GetCornerDirection(const DIRECTION dir, const bool right)
{
    return right ? dir : (DIRECTION)((dir + 3) % 4);
}

static XZ_32 M_GetInnerCornerPos(const XYZ_32 pos, const DIRECTION dir)
{
    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        return (XZ_32) {
            .x = (pos.x & ~(WALL_L - 1)) | (pos.z & (WALL_L - 1)),
            .z = (pos.z & ~(WALL_L - 1)) | (pos.x & (WALL_L - 1)),
        };

    default:
        return (XZ_32) {
            .x = (pos.x & ~(WALL_L - 1)) - (pos.z & (WALL_L - 1)) + WALL_L,
            .z = (pos.z & ~(WALL_L - 1)) - (pos.x & (WALL_L - 1)) + WALL_L,
        };
    }
}

static XZ_32 M_GetOuterCornerPos(const XYZ_32 pos, const DIRECTION dir)
{
    switch (dir) {
    case DIR_NORTH:
        return (XZ_32) {
            .x = ((pos.x + WALL_L) & ~(WALL_L - 1)) - (pos.z & (WALL_L - 1))
                + WALL_L,
            .z = ((pos.z + WALL_L) & ~(WALL_L - 1)) - (pos.x & (WALL_L - 1))
                + WALL_L,
        };

    case DIR_SOUTH:
        return (XZ_32) {
            .x = (pos.x & ~(WALL_L - 1)) - (pos.z & (WALL_L - 1)),
            .z = (pos.z & ~(WALL_L - 1)) - (pos.x & (WALL_L - 1)),
        };

    case DIR_WEST:
        return (XZ_32) {
            .x = ((pos.x & ~(WALL_L - 1)) | (pos.z & (WALL_L - 1))) - WALL_L,
            .z = ((pos.z + WALL_L) & ~(WALL_L - 1)) | (pos.x & (WALL_L - 1)),
        };

    default:
        return (XZ_32) {
            .x = ((pos.x + WALL_L) & ~(WALL_L - 1)) | (pos.z & (WALL_L - 1)),
            .z = ((pos.z & ~(WALL_L - 1)) | (pos.x & (WALL_L - 1))) - WALL_L,
        };
    }
}

static bool M_IsHangingOnLadder(const ITEM *const item, const COLL_INFO *coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->climb_status) {
        return true;
    }

    const LADDER_DIRECTION facing = 1 << Math_GetDirection(item->rot.y);
    for (int32_t side = -1; side <= 1; side += 2) {
        const XYZ_32 pos = XYZ_32_OffsetYaw(
            item->pos, item->rot.y + side * DEG_90, coll->radius);
        if (M_GetClimbFlags(pos.x, pos.z, item->room_num) & facing) {
            return true;
        }
    }
    return false;
}

// Returns +1 when Lara can traverse an outer corner in the given
// direction, -1 for an inner corner, and 0 when no turn is possible.
// On success, lara->corner_pos holds the destination.
static int32_t M_TestHangCorner(
    ITEM *const item, COLL_INFO *const coll, const bool right)
{
    if (!Item_TestAnimEqual(item, LA(LA_REACH_TO_HANG)) || coll->hit_static) {
        return 0;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const XYZ_32 old_pos = item->pos;
    const int16_t old_rot = item->rot.y;
    const int32_t front = coll->side_front.floor;
    const bool on_ladder = M_IsHangingOnLadder(item, coll);
    const DIRECTION dir = Math_GetDirection(old_rot);
    const DIRECTION corner_dir = M_GetCornerDirection(dir, right);
    const int16_t turn = right ? DEG_90 : -DEG_90;

    XZ_32 corner = M_GetInnerCornerPos(old_pos, corner_dir);
    item->pos.x = corner.x;
    item->pos.z = corner.z;
    item->rot.y += turn;
    lara->corner_pos = corner;
    int32_t result = M_IsValidHangPos(item, coll) ? -1 : 0;

    if (result != 0) {
        if (on_ladder) {
            const LADDER_DIRECTION flag = 1 << ((dir + (right ? 1 : 3)) % 4);
            if (!(M_GetClimbFlags(corner.x, corner.z, item->room_num) & flag)) {
                result = 0;
            }
        } else if (ABS(front - coll->side_front.floor) > SLOPE_DIF) {
            result = 0;
        }
    }

    if (result == 0) {
        item->pos = old_pos;
        item->rot.y = old_rot;
        lara->move_angle = old_rot;

        if (Lara_FloorFront(item, old_rot + turn, M_CORNER_FRONT_DIST) < 0) {
            return 0;
        }

        corner = M_GetOuterCornerPos(old_pos, corner_dir);
        item->pos.x = corner.x;
        item->pos.z = corner.z;
        item->rot.y -= turn;
        lara->corner_pos = corner;
        result = M_IsValidHangPos(item, coll) ? 1 : 0;

        if (result != 0) {
            item->pos = old_pos;
            item->rot.y = old_rot;
            lara->move_angle = old_rot;

            if (on_ladder) {
                const LADDER_DIRECTION flag = 1
                    << ((dir + (right ? 3 : 1)) % 4);
                if (!(M_GetClimbFlags(corner.x, corner.z, item->room_num)
                      & flag)) {
                    const int32_t new_front =
                        Lara_FloorFront(item, item->rot.y, M_CORNER_FRONT_DIST);
                    if (ABS(coll->side_front.floor - new_front) > SLOPE_DIF
                        || new_front < -M_CORNER_MAX_DROP) {
                        result = 0;
                    }
                }
            } else if (ABS(front - coll->side_front.floor) <= SLOPE_DIF) {
                // Only allow the outer turn when Lara hangs on the half of the
                // sector that meets the corner.
                const int32_t side_pos =
                    (dir == DIR_NORTH || dir == DIR_SOUTH ? old_pos.x
                                                          : old_pos.z)
                    & (WALL_L - 1);
                const bool towards_min = right
                    ? (dir == DIR_NORTH || dir == DIR_WEST)
                    : (dir == DIR_EAST || dir == DIR_SOUTH);
                if (towards_min ? side_pos < WALL_L / 2
                                : side_pos > WALL_L / 2) {
                    result = 0;
                }
            } else {
                result = 0;
            }

            return result;
        }
    }

    item->pos = old_pos;
    item->rot.y = old_rot;
    lara->move_angle = old_rot;
    return result;
}

// Ladder variant of M_TestHangCorner; same return convention.
static int32_t M_TestLadderCorner(
    ITEM *const item, COLL_INFO *const coll, const bool right)
{
    if (!Item_TestAnimEqual(
            item, LA(right ? LA_LADDER_RIGHT : LA_LADDER_LEFT))) {
        return 0;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const XYZ_32 old_pos = item->pos;
    const int16_t old_rot = item->rot.y;
    const DIRECTION dir = Math_GetDirection(old_rot);
    const DIRECTION corner_dir = M_GetCornerDirection(dir, right);
    const int16_t turn = right ? DEG_90 : -DEG_90;
    const int32_t side = right ? coll->radius + M_CLIMB_WIDTH_CORNER
                               : -(coll->radius + M_CLIMB_WIDTH_CORNER);
    int32_t result = 0;
    int32_t shift;

    XZ_32 corner = M_GetInnerCornerPos(old_pos, corner_dir);
    LADDER_DIRECTION flag = 1 << ((dir + (right ? 1 : 3)) % 4);
    if (M_GetClimbFlags(corner.x, corner.z, item->room_num) & flag) {
        item->pos.x = corner.x;
        item->pos.z = corner.z;
        item->rot.y += turn;
        lara->corner_pos = corner;
        lara->move_angle = item->rot.y;
        if (M_TestClimbPos(
                item, coll->radius, side, -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT,
                &shift)
            != CLIMB_RESULT_NONE) {
            result = -1;
        }
    }

    if (result == 0) {
        item->pos = old_pos;
        item->rot.y = old_rot;
        lara->move_angle = old_rot;

        corner = M_GetOuterCornerPos(old_pos, corner_dir);
        flag = 1 << ((dir + (right ? 3 : 1)) % 4);
        if (M_GetClimbFlags(corner.x, corner.z, item->room_num) & flag) {
            item->pos.x = corner.x;
            item->pos.z = corner.z;
            item->rot.y -= turn;
            lara->corner_pos = corner;
            lara->move_angle = item->rot.y;
            if (M_TestClimbPos(
                    item, coll->radius, side, -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT,
                    &shift)
                != CLIMB_RESULT_NONE) {
                result = 1;
            }
        }
    }

    item->pos = old_pos;
    item->rot.y = old_rot;
    lara->move_angle = old_rot;
    return result;
}

static bool M_TryCornerShimmy(ITEM *const item, COLL_INFO *const coll)
{
    if (!Lara_Col_IsCornerShimmyActive() || !g_Input.action
        || item->hit_points <= 0
        || !Item_TestAnimEqual(item, LA(LA_REACH_TO_HANG))
        || !Item_TestFrameEqual(item, M_LF_HANG)) {
        return false;
    }

    const bool left = g_Input.left || g_Input.step_left;
    const bool right = g_Input.right || g_Input.step_right;
    if (!left && !right) {
        return false;
    }

    if (M_CanHangSideways(item, coll, left ? -DEG_90 : DEG_90)) {
        item->goal_anim_state =
            Lara_Col_GetShimmyState(left ? LS_SHIMMY_LEFT : LS_SHIMMY_RIGHT);
        return true;
    }

    const int32_t result = M_TestHangCorner(item, coll, !left);
    if (result > 0) {
        item->goal_anim_state =
            LS(left ? LS_SHIMMY_OUTER_LEFT : LS_SHIMMY_OUTER_RIGHT);
        return true;
    }
    if (result < 0) {
        item->goal_anim_state =
            LS(left ? LS_SHIMMY_INNER_LEFT : LS_SHIMMY_INNER_RIGHT);
        return true;
    }
    return false;
}

static bool M_TryLadderCorner(ITEM *const item, COLL_INFO *const coll)
{
    if (!Lara_Col_IsCornerShimmyActive()) {
        return false;
    }

    const bool left = g_Input.left || g_Input.step_left;
    const bool right = g_Input.right || g_Input.step_right;
    if (!left && !right) {
        return false;
    }

    const int32_t result = M_TestLadderCorner(item, coll, !left);
    if (result > 0) {
        Item_SwitchToAnim(
            item,
            LA(left ? LA_LADDER_CORNER_LEFT_OUTER_START
                    : LA_LADDER_CORNER_RIGHT_OUTER_START),
            0);
        item->current_anim_state =
            LS(left ? LS_SHIMMY_OUTER_LEFT : LS_SHIMMY_OUTER_RIGHT);
        item->goal_anim_state = item->current_anim_state;
        return true;
    }
    if (result < 0) {
        Item_SwitchToAnim(
            item,
            LA(left ? LA_LADDER_CORNER_LEFT_INNER_START
                    : LA_LADDER_CORNER_RIGHT_INNER_START),
            0);
        item->current_anim_state =
            LS(left ? LS_SHIMMY_INNER_LEFT : LS_SHIMMY_INNER_RIGHT);
        item->goal_anim_state = item->current_anim_state;
        return true;
    }
    return false;
}

static bool M_TestLadderRelease(ITEM *const item)
{
    item->gravity = false;
    item->fall_speed = 0;

    if (g_Input.action && item->hit_points > 0) {
        return false;
    }

    item->goal_anim_state = LS(LS_JUMP_FORWARD);
    item->current_anim_state = LS(LS_JUMP_FORWARD);
    Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
    item->gravity = true;
    item->speed = 2;
    item->fall_speed = 1;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_ARMLESS;
    return true;
}

static M_CLIMB_RESULT M_TestClimbUpPos(
    const ITEM *const item, const int32_t front, const int32_t right,
    int32_t *const shift, int32_t *const ledge)
{
    const int32_t y = item->pos.y - M_CLIMB_HEIGHT - STEP_L;
    int32_t x;
    int32_t z;
    int32_t x_front = 0;
    int32_t z_front = 0;

    switch (Math_GetDirection(item->rot.y)) {
    case DIR_NORTH:
        x = item->pos.x + right;
        z = item->pos.z + front;
        z_front = M_HANG_SHIFT;
        break;

    case DIR_EAST:
        x = item->pos.x + front;
        z = item->pos.z - right;
        x_front = M_HANG_SHIFT;
        break;

    case DIR_SOUTH:
        x = item->pos.x - right;
        z = item->pos.z - front;
        z_front = -M_HANG_SHIFT;
        break;

    case DIR_WEST:
        z = item->pos.z + right;
        x = item->pos.x - front;
        x_front = -M_HANG_SHIFT;
        break;

    default:
        x = front;
        z = front;
        break;
    }

    *shift = 0;

    const SECTOR *sector;
    int32_t height;
    int32_t ceiling;

    int16_t room_num = item->room_num;
    XYZ_32 sample_pos = { x, y, z };
    sector = Room_GetSector(sample_pos, &room_num);
    ceiling = Room_GetCeiling(sector, sample_pos) + STEP_L - y;
    if (ceiling > M_CLIMB_SHIFT) {
        return CLIMB_RESULT_NONE;
    }

    if (ceiling > 0) {
        *shift = ceiling;
    }

    const int32_t x2 = x + x_front;
    const int32_t z2 = z + z_front;
    sample_pos.x = x2;
    sample_pos.z = z2;
    sector = Room_GetSector(sample_pos, &room_num);
    height = Room_GetHeightEx(sector, sample_pos, true, NO_ITEM);
    if (height == NO_HEIGHT) {
        *ledge = NO_HEIGHT;
        return CLIMB_RESULT_POS;
    }

    height -= y;
    *ledge = height;
    if (height > STEP_L / 2) {
        ceiling = Room_GetCeiling(sector, sample_pos) - y;
        if (ceiling >= M_CLIMB_HEIGHT) {
            return CLIMB_RESULT_POS;
        }

        if (M_GetDestinationClearance(item) > LARA_HEIGHT) {
            *shift = height;
            return CLIMB_RESULT_NEG;
        }

        if (g_Config.gameplay.enable_crawling
            && height - ceiling >= M_CLIMB_HEIGHT) {
            return CLIMB_RESULT_CRAWL;
        }

        return CLIMB_RESULT_NONE;
    }

    if (height > 0 && height > *shift) {
        *shift = height;
    }

    room_num = item->room_num;
    sample_pos = (XYZ_32) { x, y + M_CLIMB_HEIGHT, z };
    Room_GetSector(sample_pos, &room_num);
    sample_pos.x = x2;
    sample_pos.z = z2;
    sector = Room_GetSector(sample_pos, &room_num);
    ceiling = Room_GetCeiling(sector, sample_pos) - y;
    if (ceiling <= height) {
        return CLIMB_RESULT_POS;
    }

    if (ceiling >= M_CLIMB_HEIGHT) {
        return CLIMB_RESULT_POS;
    }
    return CLIMB_RESULT_NONE;
}

static bool M_TestLedgeJump(const ITEM *const item, const COLL_INFO *const coll)
{
    if (!g_Input.jump || !(g_Input.forward ^ g_Input.back)
        || (g_Input.forward && g_Input.slow)
        || !g_Config.gameplay.enable_ledge_jumps
        || !Lara_State_IsResponsive(LA_REACH_TO_HANG)) {
        return false;
    }

    // Lara needs sufficient space above to avoid the animation pushing her into
    // the ceiling.
    const int32_t jump_height =
        g_Input.forward ? M_LEDGE_JUMP_HEIGHT_UP : M_LEDGE_JUMP_HEIGHT_BACK;
    if (coll->side_mid.ceiling >= -jump_height) {
        return false;
    }

    // Test for a solid surface in front of Lara to push against.
    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, STEP_L);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const int32_t ceiling = Room_GetCeiling(sector, pos);
    return height == NO_HEIGHT || height < pos.y
        || (ceiling - pos.y) >= -M_LEDGE_JUMP_PUSH_HEIGHT;
}

// The ledge Lara hangs from puts her body below the things standing on it, so
// the collision info, which describes where she is now, says nothing about
// what she would pull up into.
static bool M_IsDestinationBlocked(
    const ITEM *const item, const COLL_INFO *const coll)
{
    XYZ_32 pos = XYZ_32_OffsetYaw(
        item->pos, Lara_GetLaraInfo()->move_angle, coll->radius);
    pos.y += coll->side_front.floor;
    return Room_IsPathBlocked(
        item->pos, pos, item->room_num, LARA_HEIGHT, LARA_RADIUS);
}

static void M_Hang(ITEM *const item, COLL_INFO *const coll)
{
    if (M_TryCornerShimmy(item, coll)) {
        return;
    }

    Lara_Col_HangTest(item, coll);
    if (item->goal_anim_state != LS(LS_HANG)) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->climb_status && M_TestLedgeJump(item, coll)) {
        item->goal_anim_state = LS(g_Input.forward ? LS_JUMP_UP : LS_JUMP_BACK);
        return;
    }

    if (g_Input.forward) {
        if (coll->side_front.floor > -850 && coll->side_front.floor < -650
            && M_GetDestinationClearance(item) > LARA_HEIGHT
            && !M_IsDestinationBlocked(item, coll)
            && coll->side_front.floor - coll->side_front.ceiling >= 0
            && coll->side_left2.floor - coll->side_left2.ceiling >= 0
            && coll->side_right2.floor - coll->side_right2.ceiling >= 0
            && !coll->hit_static) {
            if (g_Input.slow) {
                item->goal_anim_state = LS(LS_GYMNAST);
            } else {
                item->goal_anim_state =
                    LS(g_Config.gameplay.enable_fast_pull_up ? LS_FAST_PULL_UP
                                                             : LS_PULL_UP);
            }
            return;
        } else if (
            lara->climb_status && Item_TestAnimEqual(item, LA(LA_REACH_TO_HANG))
            && Item_TestFrameEqual(item, M_LF_HANG)
            && coll->side_mid.ceiling <= -256) {
            item->goal_anim_state = LS(LS_HANG);
            item->current_anim_state = LS(LS_HANG);
            Item_SwitchToAnim(item, LA(LA_LADDER_UP_HANGING), 0);
            return;
        }
    }

    if (g_Config.gameplay.enable_crawling && (g_Input.forward || g_Input.crouch)
        && coll->side_front.floor > -850 && coll->side_front.floor < -650
        && !M_IsDestinationBlocked(item, coll)
        && coll->side_front.floor - coll->side_front.ceiling >= -256
        && coll->side_left2.floor - coll->side_left2.ceiling >= -256
        && coll->side_right2.floor - coll->side_right2.ceiling >= -256
        && !coll->hit_static) {
        item->goal_anim_state = LS(LS_CLIMB_TO_CRAWL);
        item->required_anim_state = LS(LS_CROUCH_IDLE);
        lara->crouching = true;
    } else if (
        g_Input.back && lara->climb_status
        && Item_TestAnimEqual(item, LA(LA_REACH_TO_HANG))
        && Item_TestFrameEqual(item, M_LF_HANG)) {
        item->goal_anim_state = LS(LS_HANG);
        item->current_anim_state = LS(LS_HANG);
        Item_SwitchToAnim(item, LA(LA_LADDER_DOWN_HANGING), 0);
    }
}

static void M_Shimmy(ITEM *const item, COLL_INFO *const coll)
{
    const int32_t angle =
        item->current_anim_state == LS(LS_SHIMMY_LEFT) ? -DEG_90 : DEG_90;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + angle;
    Lara_Col_HangTest(item, coll);
    lara->move_angle = item->rot.y + angle;
}

static void M_StanceLadder(ITEM *const item, COLL_INFO *const coll)
{
    if (M_TestLadderRelease(item)
        || !Item_TestAnimEqual(item, LA(LA_LADDER_IDLE))) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.forward) {
        if (item->goal_anim_state == LS(LS_PULL_UP)) {
            return;
        }

        item->goal_anim_state = LS(LS_CLIMB_STANCE);

        int32_t shift_r = 0;
        int32_t ledge_r = 0;
        M_CLIMB_RESULT result_r = M_TestClimbUpPos(
            item, coll->radius, coll->radius + M_CLIMB_WIDTH_RIGHT, &shift_r,
            &ledge_r);

        int32_t shift_l = 0;
        int32_t ledge_l = 0;
        M_CLIMB_RESULT result_l = M_TestClimbUpPos(
            item, coll->radius, -(coll->radius + M_CLIMB_WIDTH_LEFT), &shift_l,
            &ledge_l);

        if (result_r == CLIMB_RESULT_NONE || result_l == CLIMB_RESULT_NONE) {
            return;
        }

        if (result_r == CLIMB_RESULT_NEG || result_l == CLIMB_RESULT_NEG
            || result_r == CLIMB_RESULT_CRAWL
            || result_l == CLIMB_RESULT_CRAWL) {
            if (ABS(ledge_l - ledge_r) > 120) {
                return;
            }
            if (result_r == CLIMB_RESULT_NEG && result_l == CLIMB_RESULT_NEG) {
                item->goal_anim_state = LS(LS_PULL_UP);
                item->pos.y += (ledge_l + ledge_r) / 2 - STEP_L;
            } else {
                item->goal_anim_state = LS(LS_CLIMB_TO_CRAWL);
                item->required_anim_state = LS(LS_CROUCH_IDLE);
                lara->crouching = true;
            }
            return;
        }

        int32_t shift = shift_l;
        if (shift_r) {
            if (shift_l) {
                if ((shift_r < 0) != (shift_l < 0)) {
                    return;
                }
                if (shift_r > 0 && shift_r > shift_l) {
                    shift = shift_r;
                } else if (shift_r < 0 && shift_r < shift_l) {
                    shift = shift_r;
                }
            } else {
                shift = shift_r;
            }
        }

        item->goal_anim_state = LS(LS_CLIMBING);
        item->pos.y += shift;
    } else if (g_Input.back) {
        if (item->goal_anim_state == LS(LS_HANG)) {
            return;
        }

        item->goal_anim_state = LS(LS_CLIMB_STANCE);
        item->pos.y += STEP_L;

        int32_t shift_r = 0;
        const M_CLIMB_RESULT result_r = M_TestClimbPos(
            item, coll->radius, coll->radius + M_CLIMB_WIDTH_RIGHT,
            -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT, &shift_r);

        int32_t shift_l = 0;
        const M_CLIMB_RESULT result_l = M_TestClimbPos(
            item, coll->radius, -(coll->radius + M_CLIMB_WIDTH_LEFT),
            -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT, &shift_l);

        item->pos.y -= STEP_L;
        if (result_r == CLIMB_RESULT_NONE || result_l == CLIMB_RESULT_NONE) {
            return;
        }

        int32_t shift = shift_l;
        if (shift_r && shift_l) {
            if ((shift_r < 0) != (shift_l < 0)) {
                return;
            }
            if (shift_r < 0 && shift_r < shift_l) {
                shift = shift_r;
            } else if (shift_r > 0 && shift_r > shift_l) {
                shift = shift_r;
            }
        }

        if (result_r == CLIMB_RESULT_POS && result_l == CLIMB_RESULT_POS) {
            item->goal_anim_state = LS(LS_CLIMB_DOWN);
            item->pos.y += shift;
        } else {
            item->goal_anim_state = LS(LS_HANG);
        }
    }
}

static void M_SideLadder(ITEM *const item, COLL_INFO *const coll)
{
    if (M_TestLadderRelease(item)) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    int32_t right;
    if (item->current_anim_state == LS(LS_CLIMB_LEFT)) {
        lara->move_angle = item->rot.y - DEG_90;
        right = -(coll->radius + M_CLIMB_WIDTH_LEFT);
    } else {
        lara->move_angle = item->rot.y + DEG_90;
        right = coll->radius + M_CLIMB_WIDTH_RIGHT;
    }

    int32_t shift;
    const M_CLIMB_RESULT result = M_TestClimbPos(
        item, coll->radius, right, -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT, &shift);

    if (result == CLIMB_RESULT_POS) {
        if (g_Input.left) {
            item->goal_anim_state = LS(LS_CLIMB_LEFT);
        } else if (g_Input.right) {
            item->goal_anim_state = LS(LS_CLIMB_RIGHT);
        } else {
            item->goal_anim_state = LS(LS_CLIMB_STANCE);
        }
        item->pos.y += shift;
        return;
    }

    if (result != CLIMB_RESULT_NONE) {
        item->goal_anim_state = LS(LS_HANG);
        do {
            Item_Animate(item);
        } while (item->current_anim_state != LS(LS_HANG));
        item->pos.x = coll->old_pos.x;
        item->pos.z = coll->old_pos.z;
        return;
    }

    item->pos.x = coll->old_pos.x;
    item->pos.z = coll->old_pos.z;
    item->goal_anim_state = LS(LS_CLIMB_STANCE);
    item->current_anim_state = LS(LS_CLIMB_STANCE);
    if (coll->old_anim_state == LS(LS_CLIMB_STANCE)) {
        if (M_TryLadderCorner(item, coll)) {
            return;
        }
        item->frame_num = coll->old_frame_num;
        item->anim_num = coll->old_anim_num;
        Lara_Animate(item);
    } else {
        Item_SwitchToAnim(item, LA(LA_LADDER_IDLE), 0);
    }
}

static void M_UpLadder(ITEM *const item, COLL_INFO *const coll)
{
    if (M_TestLadderRelease(item)
        || !Item_TestAnimEqual(item, LA(LA_LADDER_UP))) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (
        Item_TestFrameRange(
            item, M_LF_CLIMB_L_SHIFT_START, M_LF_CLIMB_L_SHIFT_END)) {
        yshift = -STEP_L;
    } else if (Item_TestFrameEqual(item, M_LF_CLIMB_R_SHIFT)) {
        yshift = -STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += yshift - STEP_L;

    int32_t shift_r = 0;
    int32_t ledge_r = 0;
    M_CLIMB_RESULT result_r = M_TestClimbUpPos(
        item, coll->radius, coll->radius + M_CLIMB_WIDTH_RIGHT, &shift_r,
        &ledge_r);

    int32_t shift_l = 0;
    int32_t ledge_l = 0;
    M_CLIMB_RESULT result_l = M_TestClimbUpPos(
        item, coll->radius, -(coll->radius + M_CLIMB_WIDTH_LEFT), &shift_l,
        &ledge_l);

    item->pos.y += STEP_L;

    if (result_r == CLIMB_RESULT_NONE || result_l == CLIMB_RESULT_NONE
        || !g_Input.forward) {
        item->goal_anim_state = LS(LS_CLIMB_STANCE);
        if (yshift) {
            Lara_Animate(item);
        }
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (result_r == CLIMB_RESULT_NEG || result_l == CLIMB_RESULT_NEG
        || result_r == CLIMB_RESULT_CRAWL || result_l == CLIMB_RESULT_CRAWL) {
        item->goal_anim_state = LS(LS_CLIMB_STANCE);
        Lara_Animate(item);
        if (ABS(ledge_l - ledge_r) <= 120) {
            if (result_r == CLIMB_RESULT_NEG || result_l == CLIMB_RESULT_NEG) {
                item->goal_anim_state = LS(LS_PULL_UP);
                item->pos.y += (ledge_r + ledge_l) / 2 - STEP_L;
            } else {
                item->goal_anim_state = LS(LS_CLIMB_TO_CRAWL);
                item->required_anim_state = LS(LS_CROUCH_IDLE);
                lara->crouching = true;
            }
        }
        return;
    }

    item->goal_anim_state = LS(LS_CLIMBING);
    item->pos.y -= yshift;
}

static void M_DownLadder(ITEM *const item, COLL_INFO *const coll)
{
    if (M_TestLadderRelease(item)
        || !Item_TestAnimEqual(item, LA(LA_LADDER_DOWN))) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (
        Item_TestFrameRange(
            item, M_LF_CLIMB_L_SHIFT_START, M_LF_CLIMB_L_SHIFT_END)) {
        yshift = STEP_L;
    } else if (Item_TestFrameEqual(item, M_LF_CLIMB_R_SHIFT)) {
        yshift = STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += yshift + STEP_L;

    int32_t shift_r = 0;
    const M_CLIMB_RESULT result_r = M_TestClimbPos(
        item, coll->radius, coll->radius + M_CLIMB_WIDTH_RIGHT, -M_CLIMB_HEIGHT,
        M_CLIMB_HEIGHT, &shift_r);

    int32_t shift_l = 0;
    const M_CLIMB_RESULT result_l = M_TestClimbPos(
        item, coll->radius, -(coll->radius + M_CLIMB_WIDTH_LEFT),
        -M_CLIMB_HEIGHT, M_CLIMB_HEIGHT, &shift_l);

    item->pos.y -= STEP_L;

    if (result_r == CLIMB_RESULT_NONE || result_l == CLIMB_RESULT_NONE
        || !g_Input.back) {
        item->goal_anim_state = LS(LS_CLIMB_STANCE);
        if (yshift != 0) {
            Lara_Animate(item);
        }
        return;
    }

#if 0
    int32_t shift = shift_l;
#endif
    if (shift_r && shift_l) {
        if ((shift_r < 0) != (shift_l < 0)) {
            item->goal_anim_state = LS(LS_CLIMB_STANCE);
            Lara_Animate(item);
            return;
        }
#if 0
        if (shift_r < 0 && shift_r < shift_l) {
            shift = shift_r;
        } else if (shift_r > 0 && shift_r > shift_l) {
            shift = shift_r;
        }
#endif
    }

    if (result_r == CLIMB_RESULT_NEG || result_l == CLIMB_RESULT_NEG) {
        Item_SwitchToAnim(item, LA(LA_LADDER_IDLE), 0);
        item->current_anim_state = LS(LS_CLIMB_STANCE);
        item->goal_anim_state = LS(LS_HANG);
        Lara_Animate(item);
        return;
    }

    item->goal_anim_state = LS(LS_CLIMB_DOWN);
    item->pos.y -= yshift;
}

static void M_ShimmyCorner(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_Col_GetInfo(item, coll);
}

// Returns true when the tested hang position is invalid and Lara was
// snapped back to where she was.
bool Lara_Col_HangTest(ITEM *const item, COLL_INFO *const coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    Lara_Col_GetInfo(item, coll);
    const bool flag = coll->side_front.floor < 200;

    item->gravity = false;
    item->fall_speed = 0;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;

    const DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
        item->pos.z += M_HANG_SHIFT;
        break;
    case DIR_EAST:
        item->pos.x += M_HANG_SHIFT;
        break;
    case DIR_SOUTH:
        item->pos.z -= M_HANG_SHIFT;
        break;
    case DIR_WEST:
        item->pos.x -= M_HANG_SHIFT;
        break;
    default:
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_Col_GetInfo(item, coll);

    if (lara->climb_status) {
        if (!g_Input.action || item->hit_points <= 0) {
            XYZ_32 pos = {
                .x = 0,
                .y = 0,
                .z = 0,
            };
            Collide_GetJointAbsPosition(item, &pos, 0);
            if (dir == DIR_NORTH || dir == DIR_SOUTH) {
                item->pos.x = pos.x;
            } else {
                item->pos.z = pos.z;
            }

            item->goal_anim_state = LS(LS_JUMP_FORWARD);
            item->current_anim_state = LS(LS_JUMP_FORWARD);
            Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
            item->pos.y += STEP_L;
            item->gravity = true;
            item->speed = 2;
            item->fall_speed = 1;
            lara->gun_status = LGS_ARMLESS;
            return false;
        }

        if (!Lara_Col_TestLadderHang(item, coll)) {
            int32_t height_diff = 0;
            if ((item->current_anim_state != LS(LS_SHIMMY_LEFT)
                 && item->current_anim_state != LS(LS_SHIMMY_RIGHT))
                || M_TestHangStop(item, coll, flag, &height_diff)) {
                item->pos = coll->old_pos;
                item->goal_anim_state = LS(LS_HANG);
                item->current_anim_state = LS(LS_HANG);
                Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), M_LF_HANG);
            }
            return true;
        }

        if (Item_TestAnimEqual(item, LA(LA_REACH_TO_HANG))
            && Item_TestFrameEqual(item, M_LF_HANG)
            && Lara_Col_TestClimbStance(item, coll)) {
            item->goal_anim_state = LS(LS_CLIMB_STANCE);
        }
        return false;
    }

    if (!g_Input.action || item->hit_points <= 0
        || coll->side_front.floor > 0) {
        item->goal_anim_state = LS(LS_JUMP_UP);
        item->current_anim_state = LS(LS_JUMP_UP);
        Item_SwitchToAnim(item, LA(LA_JUMP_UP), M_LF_STOP_HANG);
        const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
        if (g_Config.gameplay.enable_swing_cancel && item->hit_points > 0) {
            item->pos.y += bounds->max.y;
        } else {
            item->pos.y += coll->side_front.floor - bounds->min.y + 2;
        }
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
        item->gravity = true;
        item->speed = 2;
        item->fall_speed = 1;
        lara->gun_status = LGS_ARMLESS;
        return false;
    }

    int32_t height_diff = 0;
    if (M_TestHangStop(item, coll, flag, &height_diff)) {
        item->pos = coll->old_pos;
        if (item->current_anim_state == LS(LS_SHIMMY_LEFT)
            || item->current_anim_state == LS(LS_SHIMMY_RIGHT)) {
            item->goal_anim_state = LS(LS_HANG);
            item->current_anim_state = LS(LS_HANG);
            Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), M_LF_HANG);
        }
        return true;
    }

    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_EAST:
    case DIR_WEST:
        item->pos.x += coll->shift.x;
        break;

    default:
        break;
    }

    if (g_TRVersion >= 2 || (height_diff >= -STEP_L && height_diff <= STEP_L)) {
        item->pos.y += height_diff;
    }
    return false;
}

bool Lara_Col_IsCornerShimmyActive(void)
{
    return g_Config.gameplay.enable_corner_shimmying
        && LS(LS_SHIMMY_OUTER_LEFT) != LS_INVALID;
}

bool Lara_Col_TestLadderHang(ITEM *const item, const COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!lara->climb_status || item->fall_speed < 0) {
        return false;
    }

    const DIRECTION dir = Math_GetDirection(item->rot.y);
    switch (dir) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_EAST:
    case DIR_WEST:
        item->pos.x += coll->shift.x;
        break;

    default:
        break;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    const int32_t y = bounds->min.y;
    const int32_t h = bounds->max.y - y;

    int32_t shift;
    if (M_TestClimbPos(item, coll->radius, coll->radius, y, h, &shift)
        == CLIMB_RESULT_NONE) {
        return false;
    }

    if (M_TestClimbPos(item, coll->radius, -coll->radius, y, h, &shift)
        == CLIMB_RESULT_NONE) {
        return false;
    }

    const M_CLIMB_RESULT result =
        M_TestClimbPos(item, coll->radius, 0, y, h, &shift);
    if (result == CLIMB_RESULT_NEG) {
        item->pos.y += shift;
    }
    return result != CLIMB_RESULT_NONE;
}

bool Lara_Col_TestClimbStance(ITEM *const item, const COLL_INFO *const coll)
{
    int32_t shift_r;
    const M_CLIMB_RESULT result_r = M_TestClimbPos(
        item, coll->radius, coll->radius + M_CLIMB_WIDTH_RIGHT, -700,
        STEP_L * 2, &shift_r);
    if (result_r != CLIMB_RESULT_POS) {
        return false;
    }

    int32_t shift_l;
    const M_CLIMB_RESULT result_l = M_TestClimbPos(
        item, coll->radius, -(coll->radius + M_CLIMB_WIDTH_LEFT), -700,
        STEP_L * 2, &shift_l);
    if (result_l != CLIMB_RESULT_POS) {
        return false;
    }

    int32_t shift = 0;
    if (shift_r != 0) {
        if (shift_l) {
            if ((shift_r < 0) != (shift_l < 0)) {
                return false;
            }
            if (shift_r < 0 && shift_l < shift_r) {
                shift = shift_l;
            } else if (shift_r > 0 && shift_l > shift_r) {
                shift = shift_l;
            } else {
                shift = shift_r;
            }
        } else {
            shift = shift_r;
        }
    } else if (shift_l != 0) {
        shift = shift_l;
    }

    item->pos.y += shift;
    return true;
}

int16_t Lara_Col_GetShimmyState(const LARA_STATE_ID state)
{
    if (!g_Config.gameplay.enable_fast_shimmying) {
        return LS(state);
    }
    return LS(
        state == LS_SHIMMY_LEFT ? LS_FAST_SHIMMY_LEFT : LS_FAST_SHIMMY_RIGHT);
}

// clang-format off
REGISTER_LARA_COL(LS_HANG,               M_Hang)
REGISTER_LARA_COL(LS_SHIMMY_LEFT,        M_Shimmy)
REGISTER_LARA_COL(LS_SHIMMY_RIGHT,       M_Shimmy)
REGISTER_LARA_COL(LS_SHIMMY_OUTER_LEFT,  M_ShimmyCorner)
REGISTER_LARA_COL(LS_SHIMMY_OUTER_RIGHT, M_ShimmyCorner)
REGISTER_LARA_COL(LS_SHIMMY_INNER_LEFT,  M_ShimmyCorner)
REGISTER_LARA_COL(LS_SHIMMY_INNER_RIGHT, M_ShimmyCorner)
REGISTER_LARA_COL(LS_CLIMB_STANCE,       M_StanceLadder)
REGISTER_LARA_COL(LS_CLIMB_LEFT,         M_SideLadder)
REGISTER_LARA_COL(LS_CLIMB_RIGHT,        M_SideLadder)
REGISTER_LARA_COL(LS_CLIMBING,           M_UpLadder)
REGISTER_LARA_COL(LS_CLIMB_DOWN,         M_DownLadder)
// clang-format on
