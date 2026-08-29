#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/lara/util.h>
#include <trx/version.h>

// clang-format off
#define M_CAM_HANG_ANGLE             0
#define M_CAM_HANG_ELEVATION         (-60 * DEG_1)              // = -10920
#define M_CAM_CLIMB_LEFT_ANGLE       (-30 * DEG_1)              // = -5460
#define M_CAM_CLIMB_LEFT_ELEVATION   (-15 * DEG_1)              // = -2730
#define M_CAM_CLIMB_RIGHT_ANGLE      (-M_CAM_CLIMB_LEFT_ANGLE)  // = 5460
#define M_CAM_CLIMB_RIGHT_ELEVATION  M_CAM_CLIMB_LEFT_ELEVATION // = -2730
#define M_CAM_CLIMB_STANCE_ELEVATION (-20 * DEG_1)              // = -3640
#define M_CAM_CLIMBING_ELEVATION     (30 * DEG_1)               // = 5460
#define M_CAM_CLIMB_END_ELEVATION    (-45 * DEG_1)              // = -8190
#define M_CAM_CLIMB_DOWN_ELEVATION   M_CAM_CLIMB_END_ELEVATION  // = -8190
#define M_CAM_CORNER_ELEVATION       (-6144)                    // = -33.75 deg
#define M_LF_HANG                    21
// clang-format on

static void M_Hang(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_STOP);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->torso_rot.x = 0;
    lara->torso_rot.y = 0;

    if (g_Config.gameplay.look_mode != LOOK_MODE_RESTRICTED && g_Input.look) {
        Lara_Look_UpDown();
    }

    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = M_CAM_HANG_ANGLE;
    g_Camera.target_elevation = M_CAM_HANG_ELEVATION;
    if (Lara_Col_IsCornerShimmyActive()) {
        // The collision routine decides between shimmying and corner
        // traversal (TR4 behavior).
        return;
    }
    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = Lara_Col_GetShimmyState(LS_SHIMMY_LEFT);
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = Lara_Col_GetShimmyState(LS_SHIMMY_RIGHT);
    }
}

static void M_SetCornerAnim(
    ITEM *const item, COLL_INFO *const coll, const int16_t rot,
    const LARA_ANIMATION_ID hang_end_anim,
    const LARA_ANIMATION_ID ladder_end_anim)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS(LS_JUMP_FORWARD);
        item->current_anim_state = LS(LS_JUMP_FORWARD);
        Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
        item->pos.y += STEP_L;
        item->gravity = true;
        item->speed = 2;
        item->fall_speed = 1;
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->gun_status = LGS_ARMLESS;
        item->rot.y += rot / 2;
        return;
    }

    // Once the turn animation has advanced to its ending, teleport Lara
    // to the position computed by the corner test and snap her back to
    // the idle hold on the other face.
    const bool on_ladder = Item_TestAnimEqual(item, LA(ladder_end_anim));
    if (!on_ladder && !Item_TestAnimEqual(item, LA(hang_end_anim))) {
        return;
    }

    if (on_ladder) {
        Item_SwitchToAnim(item, LA(LA_LADDER_IDLE), 0);
        item->goal_anim_state = LS(LS_CLIMB_STANCE);
        item->current_anim_state = LS(LS_CLIMB_STANCE);
    } else {
        Item_SwitchToAnim(item, LA(LA_REACH_TO_HANG), M_LF_HANG);
        item->goal_anim_state = LS(LS_HANG);
        item->current_anim_state = LS(LS_HANG);
    }

    if (g_Camera.type == CAM_CHASE && on_ladder
        && (ladder_end_anim == LA_LADDER_CORNER_RIGHT_OUTER_END
            || ladder_end_anim == LA_LADDER_CORNER_LEFT_OUTER_END)) {
        // Some camera strategies will be unable to LOS through the corner from
        // Lara's old position to her new, so will become stuck. Force a
        // transitional target update with Lara placed at the corner.
        // TODO: investigate alternatives to this approach
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y - rot, STEP_L);
        g_Camera.speed = 1;
        Camera_Update();
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->old_pos.x = lara->corner_pos.x;
    coll->old_pos.z = lara->corner_pos.z;
    item->pos.x = lara->corner_pos.x;
    item->pos.z = lara->corner_pos.z;
    item->rot.y += rot;
    Interpolation_RememberItem(item);
}

static void M_ShimmyCornerOuterLeft(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = DEG_90;
    g_Camera.target_elevation = M_CAM_CORNER_ELEVATION;
    M_SetCornerAnim(
        item, coll, DEG_90, LA_HANG_CORNER_LEFT_OUTER_END,
        LA_LADDER_CORNER_LEFT_OUTER_END);
}

static void M_ShimmyCornerOuterRight(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = -DEG_90;
    g_Camera.target_elevation = M_CAM_CORNER_ELEVATION;
    M_SetCornerAnim(
        item, coll, -DEG_90, LA_HANG_CORNER_RIGHT_OUTER_END,
        LA_LADDER_CORNER_RIGHT_OUTER_END);
}

static void M_ShimmyCornerInnerLeft(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = -DEG_90;
    g_Camera.target_elevation = M_CAM_CORNER_ELEVATION;
    M_SetCornerAnim(
        item, coll, -DEG_90, LA_HANG_CORNER_LEFT_INNER_END,
        LA_LADDER_CORNER_LEFT_INNER_END);
}

static void M_ShimmyCornerInnerRight(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = DEG_90;
    g_Camera.target_elevation = M_CAM_CORNER_ELEVATION;
    M_SetCornerAnim(
        item, coll, DEG_90, LA_HANG_CORNER_RIGHT_INNER_END,
        LA_LADDER_CORNER_RIGHT_INNER_END);
}

static void M_Shimmy(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = M_CAM_HANG_ANGLE;
    g_Camera.target_elevation = M_CAM_HANG_ELEVATION;

    const bool stop = item->current_anim_state == LS(LS_SHIMMY_LEFT)
        ? (!g_Input.left && !g_Input.step_left)
        : (!g_Input.right && !g_Input.step_right);
    if (stop) {
        item->goal_anim_state = LS(LS_HANG);
    }
}

static void M_StanceLadder(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = M_CAM_CLIMB_STANCE_ELEVATION;

    if (g_Input.look) {
        Lara_Look_UpDown();
    }

    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS(LS_CLIMB_LEFT);
        lara->move_angle = item->rot.y - DEG_90;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS(LS_CLIMB_RIGHT);
        lara->move_angle = item->rot.y + DEG_90;
    } else if (g_Input.jump) {
        item->goal_anim_state = LS(LS_JUMP_BACK);
        lara->gun_status = LGS_ARMLESS;
        lara->move_angle = item->rot.y + DEG_180;
    }
}

static void M_SideLadder(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    if (item->current_anim_state == LS(LS_CLIMB_LEFT)) {
        g_Camera.target_angle = M_CAM_CLIMB_LEFT_ANGLE;
        g_Camera.target_elevation = M_CAM_CLIMB_LEFT_ELEVATION;
        if (!g_Input.left && !g_Input.step_left) {
            item->goal_anim_state = LS(LS_CLIMB_STANCE);
        }
    } else {
        g_Camera.target_angle = M_CAM_CLIMB_RIGHT_ANGLE;
        g_Camera.target_elevation = M_CAM_CLIMB_RIGHT_ELEVATION;
        if (!g_Input.right && !g_Input.step_right) {
            item->goal_anim_state = LS(LS_CLIMB_STANCE);
        }
    }
}

static void M_UpDownLadder(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    switch (LS_U(item->current_anim_state)) {
    case LS_CLIMBING:
        g_Camera.target_elevation = M_CAM_CLIMBING_ELEVATION;
        break;
    case LS_CLIMB_DOWN:
        g_Camera.target_elevation = M_CAM_CLIMB_DOWN_ELEVATION;
        break;
    case LS_CLIMB_END:
        g_Camera.flags = CF_FOLLOW_CENTRE;
        g_Camera.target_angle = M_CAM_CLIMB_END_ELEVATION;
        break;
    default:
        break;
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_HANG,               M_Hang)
REGISTER_LARA_STATE(LS_SHIMMY_LEFT,        M_Shimmy)
REGISTER_LARA_STATE(LS_SHIMMY_RIGHT,       M_Shimmy)
REGISTER_LARA_STATE(LS_SHIMMY_OUTER_LEFT,  M_ShimmyCornerOuterLeft)
REGISTER_LARA_STATE(LS_SHIMMY_OUTER_RIGHT, M_ShimmyCornerOuterRight)
REGISTER_LARA_STATE(LS_SHIMMY_INNER_LEFT,  M_ShimmyCornerInnerLeft)
REGISTER_LARA_STATE(LS_SHIMMY_INNER_RIGHT, M_ShimmyCornerInnerRight)
REGISTER_LARA_STATE(LS_CLIMB_STANCE,       M_StanceLadder)
REGISTER_LARA_STATE(LS_CLIMB_LEFT,         M_SideLadder)
REGISTER_LARA_STATE(LS_CLIMB_RIGHT,        M_SideLadder)
REGISTER_LARA_STATE(LS_CLIMBING,           M_UpDownLadder)
REGISTER_LARA_STATE(LS_CLIMB_DOWN,         M_UpDownLadder)
REGISTER_LARA_STATE(LS_CLIMB_END,          M_UpDownLadder)
// clang-format on
