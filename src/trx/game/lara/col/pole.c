#include <trx/core/math.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/util.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_RADIUS               100
#define M_BAD_NEG              (-384)
#define M_BAD_CEILING          192
#define M_CLIMB_UP_CLEARANCE   WALL_L
#define M_TURN_RATE            256
#define M_LET_GO_SHIFT         64
#define M_FLOOR_SNAP_PROBE     762
#define M_SLIDE_ACCELERATION   256
#define M_SLIDE_DECELERATION   1024
#define M_SLIDE_MAX_VELOCITY   16384
#define M_LF_JUMP_RELEASE      (-2)
// clang-format on

static int32_t m_SlideVelocity = 0;

static int32_t M_GetCeilingClearance(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    return item->pos.y - Room_GetCeiling(sector, item->pos);
}

static void M_PoleIdle(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_FAST_FALL);
        return;
    }

    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (Item_TestAnimEqual(item, LA(LA_POLE_JUMP))
        && Item_TestFrameEqual(item, M_LF_JUMP_RELEASE)) {
        lara->gun_status = LGS_ARMLESS;
    }

    if (!Item_TestAnimEqual(item, LA(LA_POLE_IDLE))) {
        return;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = M_BAD_NEG;
    coll->bad_ceiling = M_BAD_CEILING;
    coll->radius = M_RADIUS;
    coll->slopes_are_walls = 1;
    lara->move_angle = item->rot.y;
    Lara_Col_GetInfo(item, coll);

    if (g_Input.action) {
        item->goal_anim_state = LS(LS_POLE_IDLE);

        if (g_Input.left) {
            item->goal_anim_state = LS(LS_POLE_LEFT);
        } else if (g_Input.right) {
            item->goal_anim_state = LS(LS_POLE_RIGHT);
        }

        if (g_Input.look) {
            Lara_Look_UpDown();
        }

        if (g_Input.forward) {
            if (M_GetCeilingClearance(item) > M_CLIMB_UP_CLEARANCE) {
                item->goal_anim_state = LS(LS_POLE_UP);
            }
        } else if (g_Input.back && coll->side_mid.floor > 0) {
            item->goal_anim_state = LS(LS_POLE_DOWN);
            m_SlideVelocity = 0;
        }

        if (g_Input.jump) {
            item->goal_anim_state = LS(LS_JUMP_BACK);
        }
    } else if (coll->side_mid.floor <= 0) {
        item->goal_anim_state = LS(LS_STOP);
    } else {
        item->goal_anim_state = LS(LS_FAST_FALL);
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, -M_LET_GO_SHIFT);
    }
}

static void M_PoleUp(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    if (!g_Input.forward || !g_Input.action || item->hit_points <= 0
        || M_GetCeilingClearance(item) < M_CLIMB_UP_CLEARANCE) {
        item->goal_anim_state = LS(LS_POLE_IDLE);
    }
}

static void M_PoleDown(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    if (!g_Input.back || !g_Input.action || item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_POLE_IDLE);
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = M_BAD_NEG;
    coll->bad_ceiling = 0;
    coll->radius = M_RADIUS;
    coll->slopes_are_walls = 1;
    lara->move_angle = item->rot.y;
    Lara_Col_GetInfo(item, coll);

    if (coll->side_mid.floor < 0) {
        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
        item->floor = Room_GetHeight(
            sector,
            (XYZ_32) {
                .x = item->pos.x,
                .y = item->pos.y - M_FLOOR_SNAP_PROBE,
                .z = item->pos.z,
            });
        item->goal_anim_state = LS(LS_POLE_IDLE);
        m_SlideVelocity = 0;
    }

    if (g_Input.left) {
        item->rot.y += M_TURN_RATE;
    } else if (g_Input.right) {
        item->rot.y -= M_TURN_RATE;
    }

    if (Item_TestAnimEqual(item, LA(LA_POLE_CLIMB_DOWN_TO_IDLE))) {
        m_SlideVelocity -= M_SLIDE_DECELERATION;
    } else {
        m_SlideVelocity += M_SLIDE_ACCELERATION;
    }
    CLAMP(m_SlideVelocity, 0, M_SLIDE_MAX_VELOCITY);

    Sound_Effect(SFX_LARA_POLE_LOOP, &item->pos, SPM_NORMAL);
    item->pos.y += m_SlideVelocity >> 8;
}

REGISTER_LARA_COL(LS_POLE_IDLE, M_PoleIdle)
REGISTER_LARA_COL(LS_POLE_UP, M_PoleUp)
REGISTER_LARA_COL(LS_POLE_DOWN, M_PoleDown)
