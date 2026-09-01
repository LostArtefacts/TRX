#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/items/anim.h>
#include <trx/game/lara.h>
#include <trx/game/lara/misc.h>
#include <trx/game/lara/util.h>
#include <trx/game/rooms.h>
#include <trx/game/rooms/geometry.h>
#include <trx/game/rooms/utils.h>

// clang-format off
#define M_CROUCH_RADIUS            200
#define M_CRAWL_BACK_RADIUS        250
#define M_CRAWL_BAD_POS            255
#define M_CRAWL_BAD_NEG           -255
#define M_CRAWL_BAD_CEILING        400
#define M_CROUCH_CEILING_THRESHOLD -362
#define M_CRAWL_TO_HANG_RADIUS     200
#define M_CRAWL_TO_HANG_HEIGHT     870
#define M_CRAWL_TO_HANG_XZ_OFFSET  100
#define M_CRAWL_TO_HANG_FALL_SPEED 512
#define M_CRAWL_TO_HANG_BAD_CEILING ((STEP_L * 3) / 4) // = 192
#define M_CRAWL_TO_HANG_FALL_FRAME 9
#define M_CRAWL_TILT_RADIUS        140
#define M_CRAWL_TILT_HEIGHT        238
#define M_CRAWL_TILT_RATE          (DEG_1 * 3)         // = 546
#define M_CRAWL_TILT_MAX           DEG_45
#define M_MAX_WATER_DEPTH          (LARA_HEIGHT_CROUCH - STEP_L / 2) // = 272
// clang-format on

static bool M_DeflectEdgeCrawl(ITEM *const item, COLL_INFO *const coll)
{
    switch (coll->coll_type) {
    case COLL_FRONT:
    case COLL_TOP_FRONT:
        Lara_Col_Shift(coll);
        item->speed = 0;
        item->gravity = false;
        return true;

    case COLL_LEFT:
        Lara_Col_Shift(coll);
        item->rot.y += LARA_TURN_UNDO;
        break;

    case COLL_RIGHT:
        Lara_Col_Shift(coll);
        item->rot.y -= LARA_TURN_UNDO;
        break;

    default:
        break;
    }

    return false;
}

static bool M_HasStaticBehind(const ITEM *const item, const int16_t angle)
{
    COLL_INFO test = {
        .radius = 50,
    };
    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, angle, STEP_L * 2);
    return Collide_CollideStaticObjects(&test, pos, item->room_num, 300);
}

static bool M_IsBadDestination(const ITEM *const item, const int16_t angle)
{
    if (!g_Config.gameplay.fix_underwater_crawl) {
        return false;
    }

    XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, angle, STEP_L);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    pos.y = Room_GetHeight(sector, pos) - M_MAX_WATER_DEPTH;
    Room_GetSector(pos, &room_num);
    const ROOM *const room = Room_Get(room_num);
    return room->flags.swamp || room->flags.underwater;
}

static void M_Crouch(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
    lara->move_angle = item->rot.y;

    coll->facing = lara->move_angle;
    coll->radius = M_CROUCH_RADIUS;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, -LARA_HEIGHT_CROUCH);

    if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
        return;
    }

    lara->keep_crouched = coll->side_mid.ceiling >= M_CROUCH_CEILING_THRESHOLD;
    Lara_Col_Shift(coll);
    item->pos.y += coll->side_mid.floor;

    const bool crouch_active = g_Config.gameplay.enable_toggle_crouch
        ? !(lara->crouching && g_InputDB.crouch)
        : g_Input.crouch;

    if ((!crouch_active || lara->water_status == LWS_WADE)
        && !lara->keep_crouched
        && Item_TestAnimEqual(item, LA(LA_CROUCH_IDLE))) {
        lara->crouching = false;
        item->goal_anim_state = LS(LS_STOP);
    } else if (g_Config.gameplay.enable_responsive_crawl) {
        if (g_Input.left) {
            item->goal_anim_state = LS(LS_CROUCH_TURN_LEFT);
        } else if (g_Input.right) {
            item->goal_anim_state = LS(LS_CROUCH_TURN_RIGHT);
        }
    }
}

static void M_CrouchRoll(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;

    coll->facing = lara->move_angle;
    coll->radius = M_CROUCH_RADIUS;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, -LARA_HEIGHT_CROUCH);

    if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
    } else if (!Lara_Col_TestSlide(item, coll)) {
        lara->keep_crouched =
            coll->side_mid.ceiling >= M_CROUCH_CEILING_THRESHOLD;

        if (coll->side_mid.floor < coll->bad_neg
            || coll->side_front.floor > coll->bad_pos
            || M_IsBadDestination(item, lara->move_angle)) {
            item->pos = coll->old_pos;
            return;
        }

        Lara_Col_Shift(coll);

        if (coll->coll_type == COLL_TOP || coll->coll_type == COLL_CLAMP) {
            item->pos = coll->old_pos;
            item->speed = 0;
        } else {
            item->pos.y += coll->side_mid.floor;
        }
    }
}

static void M_CrouchTurn(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
    lara->move_angle = item->rot.y;

    coll->facing = lara->move_angle;
    coll->radius = M_CROUCH_RADIUS;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, LARA_HEIGHT_CROUCH);

    if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
        return;
    }

    if (Lara_Col_TestSlide(item, coll)) {
        return;
    }

    lara->keep_crouched = coll->side_mid.ceiling >= M_CROUCH_CEILING_THRESHOLD;

    Lara_Col_Shift(coll);
    item->pos.y += coll->side_mid.floor;
}

static void M_CrawlIdle(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    if (item->goal_anim_state == LS(LS_CRAWL_TO_CLIMB)) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
    lara->move_angle = item->rot.y;

    coll->facing = lara->move_angle;
    coll->radius = M_CROUCH_RADIUS;
    coll->bad_pos = M_CRAWL_BAD_POS;
    coll->bad_neg = M_CRAWL_BAD_NEG;
    coll->bad_ceiling = M_CRAWL_BAD_CEILING;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, LARA_HEIGHT_CROUCH);
    Lara_Col_CrawlTilt(item);

    if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
        return;
    }

    lara->keep_crouched = coll->side_mid.ceiling >= M_CROUCH_CEILING_THRESHOLD;

    Lara_Col_Shift(coll);
    item->pos.y += coll->side_mid.floor;

    const bool crouch_active = g_Config.gameplay.enable_toggle_crouch
        ? lara->crouching
        : g_Input.crouch;
    if ((!crouch_active && !lara->keep_crouched) || g_Input.draw
        || (g_Config.gameplay.enable_toggle_crouch && !g_Input.forward
            && g_InputDB.crouch)) {
        item->goal_anim_state = LS(LS_CROUCH_IDLE);
        return;
    }

    bool allow_movement = Item_TestAnimEqual(item, LA(LA_CRAWL_IDLE))
        || Item_TestAnimEqual(item, LA(LA_CROUCH_TO_CRAWL_END));
    if (g_Config.gameplay.enable_responsive_crawl) {
        allow_movement |=
            Item_TestAnimEqual(item, LA(LA_CRAWL_FORWARD_TO_IDLE_END_LEFT))
            || Item_TestAnimEqual(item, LA(LA_CRAWL_FORWARD_TO_IDLE_END_RIGHT));
    }

    if (!allow_movement) {
        return;
    }

    if (g_Input.forward) {
        const int16_t h = Lara_FloorFront(item, item->rot.y, 256);
        if (h < 255 && h > -255 && Room_GetHeightType() != HT_BIG_SLOPE) {
            item->goal_anim_state = LS(LS_CRAWL_FORWARD);
        }
    } else if (g_Input.back) {
        int32_t h = Lara_CeilingFront(item, item->rot.y, -300, STEP_L / 2);
        if (h == NO_HEIGHT || h > 256) {
            return;
        }

        h = Lara_FloorFront(item, item->rot.y, -300);
        if (h < 255 && h > -255 && Room_GetHeightType() != HT_BIG_SLOPE) {
            item->goal_anim_state = LS(LS_CRAWL_BACK);
            return;
        }

        if (g_Input.action && h > 768
            && !M_HasStaticBehind(item, item->rot.y + DEG_180)) {
            const XYZ_32 old_pos = item->pos;
            const XYZ_16 old_rot = item->rot;

            const DIRECTION dir = Math_GetDirection(item->rot.y);
            switch (dir) {
            case DIR_NORTH:
                item->rot.y = 0;
                item->pos.z = ROUND_TO_SECTOR(item->pos.z) + 225;
                break;

            case DIR_EAST:
                item->rot.y = DEG_90;
                item->pos.x = ROUND_TO_SECTOR(item->pos.x) + 225;
                break;

            case DIR_SOUTH:
                item->rot.y = -DEG_180;
                item->pos.z = ROUND_TO_SECTOR_END(item->pos.z) - 225;
                break;

            case DIR_WEST:
                item->rot.y = -DEG_90;
                item->pos.x = ROUND_TO_SECTOR_END(item->pos.x) - 225;
                break;
            default:
                break;
            }

            h = Lara_FloorFront(item, item->rot.y, 0);
            if (h > 255 || h < -255 || Room_GetHeightType() == HT_BIG_SLOPE) {
                item->pos = old_pos;
                item->rot = old_rot;
            } else {
                item->goal_anim_state = LS(LS_CRAWL_TO_CLIMB);
                lara->crouching = false;
            }
        }
    } else if (g_Input.left) {
        Item_SwitchToAnim(item, LA(LA_CRAWL_TURN_LEFT), 0);
        item->goal_anim_state = LS(LS_CRAWL_TURN_LEFT);
        item->current_anim_state = LS(LS_CRAWL_TURN_LEFT);
    } else if (g_Input.right) {
        Item_SwitchToAnim(item, LA(LA_CRAWL_TURN_RIGHT), 0);
        item->goal_anim_state = LS(LS_CRAWL_TURN_RIGHT);
        item->current_anim_state = LS(LS_CRAWL_TURN_RIGHT);
    }
}

static void M_CrawlForward(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
    lara->move_angle = item->rot.y;

    coll->radius = M_CROUCH_RADIUS;
    coll->bad_pos = M_CRAWL_BAD_POS;
    coll->bad_neg = M_CRAWL_BAD_NEG;
    coll->bad_ceiling = M_CRAWL_BAD_CEILING;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    coll->facing = lara->move_angle;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, -LARA_HEIGHT_CROUCH);
    Lara_Col_CrawlTilt(item);

    if (M_DeflectEdgeCrawl(item, coll)
        || M_IsBadDestination(item, lara->move_angle)) {
        item->current_anim_state = LS(LS_CRAWL_IDLE);
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        if (!Item_TestAnimEqual(item, LA(LA_CRAWL_IDLE))) {
            Item_SwitchToAnim(item, LA(LA_CRAWL_IDLE), 0);
        }
    } else if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
    } else {
        Lara_Col_Shift(coll);
        item->pos.y += coll->side_mid.floor;
    }
}

static void M_CrawlTurn(ITEM *const item, COLL_INFO *const coll)
{
    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, LARA_HEIGHT_CROUCH);
    Lara_Col_CrawlTilt(item);
}

static void M_CrawlBack(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    item->fall_speed = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->is_crouched = true;
    lara->move_angle = item->rot.y + DEG_180;

    coll->radius = M_CRAWL_BACK_RADIUS;
    coll->bad_pos = M_CRAWL_BAD_POS;
    coll->bad_neg = M_CRAWL_BAD_NEG;
    coll->bad_ceiling = M_CRAWL_BAD_CEILING;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    coll->facing = lara->move_angle;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, -LARA_HEIGHT_CROUCH);
    Lara_Col_CrawlTilt(item);

    if (M_DeflectEdgeCrawl(item, coll)
        || M_IsBadDestination(item, lara->move_angle)) {
        item->current_anim_state = LS(LS_CRAWL_IDLE);
        item->goal_anim_state = LS(LS_CRAWL_IDLE);
        if (!Item_TestAnimEqual(item, LA(LA_CRAWL_IDLE))) {
            Item_SwitchToAnim(item, LA(LA_CRAWL_IDLE), 0);
        }
    } else if (Lara_Col_Fallen(item, coll)) {
        lara->gun_status = LGS_ARMLESS;
    } else {
        Lara_Col_Shift(coll);
        item->pos.y += coll->side_mid.floor;
        lara->move_angle = item->rot.y;
    }
}

static void M_CrawlToClimb(ITEM *const item, COLL_INFO *const coll)
{
    if (!Item_TestAnimEqual(item, LA(LA_CRAWL_TO_HANG_END))) {
        return;
    }

    item->fall_speed = M_CRAWL_TO_HANG_FALL_SPEED;
    item->pos.y |= 255;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y;

    coll->radius = M_CRAWL_TO_HANG_RADIUS;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = M_CRAWL_TO_HANG_BAD_CEILING;
    coll->facing = lara->move_angle;

    Collide_GetCollisionInfo(
        coll, item->pos, item->room_num, M_CRAWL_TO_HANG_HEIGHT);

    int32_t edge = 0;
    const EDGE_CATCH edge_catch = Lara_Col_TestEdgeCatch(item, coll, &edge);
    if (edge_catch == EDGE_CATCH_NONE
        || (edge_catch == EDGE_CATCH_NEG
            && !Lara_Col_TestLadderHang(item, coll))) {
        // LA_CRAWL_TO_HANG_END will loop indefinitely, so in cases where Lara
        // cannot grab the edge, make her fall and she will then either re-grab
        // it on a better position, or continue falling if the ledge is a slope.
        Item_SwitchToAnim(item, LA(LA_JUMP_UP), M_CRAWL_TO_HANG_FALL_FRAME);
        item->current_anim_state = LS(LS_JUMP_UP);
        item->goal_anim_state = LS(LS_JUMP_UP);
        item->gravity = true;
        item->speed = 2;
        item->fall_speed = 1;
        lara->gun_status = LGS_ARMLESS;
        return;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, LARA_HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return;
    }
    const int16_t angle = Math_DirectionToAngle(dir);

    const SWING_CATCH swing_catch = Lara_Col_TestHangSwingIn(item, angle);
    if (swing_catch == SWING_CATCH_SLOW) {
        lara->head_rot.x = 0;
        lara->head_rot.y = 0;
        lara->torso_rot.x = 0;
        lara->torso_rot.y = 0;
        Item_SwitchToAnim(item, LA(LA_SWING_IN_SLOW), 0);
    } else if (swing_catch == SWING_CATCH_FAST) {
        Item_SwitchToAnim(item, LA(LA_SWING_IN_FAST), 0);
    } else {
        Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), 0);
    }
    const ANIM *const anim = Item_GetAnim(item);
    item->current_anim_state = anim->current_anim_state;
    item->goal_anim_state = anim->current_anim_state;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    if (edge_catch == EDGE_CATCH_POS) {
        item->pos.y += coll->side_front.floor - bounds->min.y;

        switch (dir) {
        case DIR_NORTH:
            item->pos.z =
                ROUND_TO_SECTOR_END(item->pos.z) - M_CRAWL_TO_HANG_XZ_OFFSET;
            item->pos.x += coll->shift.x;
            break;

        case DIR_EAST:
            item->pos.x =
                ROUND_TO_SECTOR_END(item->pos.x) - M_CRAWL_TO_HANG_XZ_OFFSET;
            item->pos.z += coll->shift.z;
            break;

        case DIR_SOUTH:
            item->pos.z =
                ROUND_TO_SECTOR(item->pos.z) + M_CRAWL_TO_HANG_XZ_OFFSET;
            item->pos.x += coll->shift.x;
            break;

        case DIR_WEST:
            item->pos.x =
                ROUND_TO_SECTOR(item->pos.x) + M_CRAWL_TO_HANG_XZ_OFFSET;
            item->pos.z += coll->shift.z;
            break;

        default:
            break;
        }
    } else {
        item->pos.y = edge - bounds->min.y;
    }

    item->rot.y = angle;
    item->speed = 2;
    item->fall_speed = 1;
    item->gravity = true;
    lara->gun_status = LGS_HANDS_BUSY;
}

static XZ_32 M_GetWalkableTilt(const ITEM *const item, const int32_t y_pos)
{
    const int32_t base_x = ROUND_TO_SECTOR(item->pos.x);
    const int32_t base_z = ROUND_TO_SECTOR(item->pos.z);
    const XZ_32 offsets[3] = {
        { 1, 1 },
        { 3, 1 },
        { 1, 3 },
    };

    int32_t heights[3] = {};
    for (int32_t i = 0; i < 3; i++) {
        const XYZ_32 off_pos = {
            .x = base_x | (offsets[i].x * STEP_L - 1),
            .z = base_z | (offsets[i].z * STEP_L - 1),
            .y = y_pos,
        };
        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(off_pos, &room_num);
        heights[i] = Room_GetHeight(sector, off_pos);
    }

    return (XZ_32) { heights[1] - heights[0], heights[2] - heights[0] };
}

static int16_t M_GetTilt(const int32_t delta, const int32_t radius)
{
    int16_t rot = Math_Atan(2 * radius, delta);
    if ((delta > 0 && rot > 0) || (delta < 0 && rot < 0)) {
        rot = -rot;
    }
    return rot;
}

static void M_ApproachTilt(const int16_t target, int16_t *const current)
{
    if (ABS(target - *current) < M_CRAWL_TILT_RATE) {
        *current = target;
    } else if (target > *current) {
        *current += M_CRAWL_TILT_RATE;
    } else {
        *current -= M_CRAWL_TILT_RATE;
    }
    CLAMP(*current, -M_CRAWL_TILT_MAX, M_CRAWL_TILT_MAX);
}

void Lara_Col_CrawlTilt(ITEM *const item)
{
    if (!g_Config.gameplay.enable_crawl_tilt) {
        return;
    }

    const XYZ_32 pos = {
        .x = item->pos.x,
        .y = item->pos.y - M_CRAWL_TILT_HEIGHT,
        .z = item->pos.z,
    };
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);

    XYZ_F plane = {};
    if (Room_IsOnWalkable(sector, pos, height, NO_ITEM)) {
        const XZ_32 tilt = M_GetWalkableTilt(item, pos.y);
        plane.x = tilt.x * 2.0f / WALL_L;
        plane.z = tilt.z * 2.0f / WALL_L;
    } else {
        const XZ_16 tilt = Room_GetTiltType(sector, pos);
        plane.x = -tilt.x / 4.0f;
        plane.z = -tilt.z / 4.0f;
    }

    plane.y = item->pos.y - plane.x * item->pos.x - plane.z * item->pos.z;

    int32_t heights[4] = {};
    for (int32_t i = 0; i < 4; i++) {
        const XYZ_32 test_pos = XYZ_32_OffsetYaw(
            pos, item->rot.y + DEG_90 * i, M_CRAWL_TILT_RADIUS);
        room_num = item->room_num;
        const SECTOR *const test_sector = Room_GetSector(test_pos, &room_num);
        heights[i] = Room_GetHeight(test_sector, test_pos);

        if (ABS(height - heights[i]) > M_CRAWL_TILT_RADIUS / 2) {
            heights[i] = plane.x * test_pos.x + plane.z * test_pos.z + plane.y;
        }
    }

    const XZ_32 delta = {
        .x = heights[0] - heights[2],
        .z = heights[3] - heights[1],
    };
    const XZ_16 rot = {
        .x = M_GetTilt(delta.x, M_CRAWL_TILT_RADIUS),
        .z = M_GetTilt(delta.z, M_CRAWL_TILT_RADIUS),
    };
    M_ApproachTilt(rot.x, &item->rot.x);
    M_ApproachTilt(rot.z, &item->rot.z);
}

// clang-format off
REGISTER_LARA_COL(LS_CROUCH_IDLE,       M_Crouch)
REGISTER_LARA_COL(LS_CROUCH_ROLL,       M_CrouchRoll)
REGISTER_LARA_COL(LS_CROUCH_TURN_LEFT,  M_CrouchTurn)
REGISTER_LARA_COL(LS_CROUCH_TURN_RIGHT, M_CrouchTurn)
REGISTER_LARA_COL(LS_CRAWL_IDLE,        M_CrawlIdle)
REGISTER_LARA_COL(LS_CRAWL_FORWARD,     M_CrawlForward)
REGISTER_LARA_COL(LS_CRAWL_TURN_LEFT,   M_CrawlTurn)
REGISTER_LARA_COL(LS_CRAWL_TURN_RIGHT,  M_CrawlTurn)
REGISTER_LARA_COL(LS_CRAWL_BACK,        M_CrawlBack)
REGISTER_LARA_COL(LS_CRAWL_TO_CLIMB,    M_CrawlToClimb)
// clang-format on
