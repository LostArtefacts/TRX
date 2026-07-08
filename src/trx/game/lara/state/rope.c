#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/rope.h>
#include <trx/game/lara/util.h>
#include <trx/game/rope.h>

#define M_TURN_RATE 256
#define M_CAM_CLIMB_ANGLE 5460
#define M_TOP_SEGMENT 4
#define M_BOTTOM_SEGMENT 21

static void M_RopeIdle(ITEM *const item, COLL_INFO *const coll)
{
    if (!g_Input.action) {
        Lara_Rope_FallOff(item);
    }

    if (g_Input.look) {
        Lara_Look_UpDown();
    }
}

static void M_RopeTurnLeft(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.action) {
        if (g_Input.left) {
            lara->rope.y_rot += M_TURN_RATE;
        } else {
            item->goal_anim_state = LS(LS_ROPE_IDLE);
        }
    } else {
        Lara_Rope_FallOff(item);
    }
}

static void M_RopeTurnRight(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.action) {
        if (g_Input.right) {
            lara->rope.y_rot -= M_TURN_RATE;
        } else {
            item->goal_anim_state = LS(LS_ROPE_IDLE);
        }
    } else {
        Lara_Rope_FallOff(item);
    }
}

static void M_RopeClimb(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.roll) {
        Lara_Rope_FallOff(item);
        return;
    }

    g_Camera.target_angle = M_CAM_CLIMB_ANGLE;

    const ANIM *const anim = Item_GetAnim(item);
    if (item->frame_num == anim->frame_end) {
        item->frame_num = anim->frame_base;
        lara->rope.segment -= 2;
    }

    if (!g_Input.forward || lara->rope.segment <= M_TOP_SEGMENT) {
        item->goal_anim_state = LS(LS_ROPE_IDLE);
    }
}

static void M_RopeSlide(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action) {
        Lara_Rope_FallOff(item);
        return;
    }

    g_Camera.target_angle = M_CAM_CLIMB_ANGLE;

    if (lara->rope.count != 0) {
        if (lara->rope.flag == 0) {
            lara->rope.count++;
            lara->rope.offset += lara->rope.down_vel;
            if (lara->rope.count == 0) {
                lara->rope.flag = 1;
            }
            return;
        }
    } else if (lara->rope.flag == 0) {
        const ROPE *const rope = Rope_Get(lara->rope.index);
        lara->rope.offset = 0;
        lara->rope.down_vel =
            (uint32_t)(rope->mesh_segments[lara->rope.segment + 1].y
                       - rope->mesh_segments[lara->rope.segment].y)
            >> 17;
        lara->rope.count = 0;
        lara->rope.offset += lara->rope.down_vel;
        lara->rope.flag = 1;
        return;
    }

    const ANIM *const anim = Item_GetAnim(item);
    if (Item_TestAnimEqual(item, LA(LA_ROPE_DOWN))
        && item->frame_num == anim->frame_end) {
        item->frame_num = anim->frame_base;
        lara->rope.flag = 0;
        lara->rope.segment++;
        lara->rope.offset = 0;
    }

    if (!g_Input.back || lara->rope.segment >= M_BOTTOM_SEGMENT) {
        item->goal_anim_state = LS(LS_ROPE_IDLE);
    }
}

REGISTER_LARA_STATE(LS_ROPE_IDLE, M_RopeIdle)
REGISTER_LARA_STATE(LS_ROPE_FORWARD, M_RopeIdle)
REGISTER_LARA_STATE(LS_ROPE_BACK, M_RopeIdle)
REGISTER_LARA_STATE(LS_ROPE_LEFT, M_RopeTurnLeft)
REGISTER_LARA_STATE(LS_ROPE_RIGHT, M_RopeTurnRight)
REGISTER_LARA_STATE(LS_ROPE_CLIMB, M_RopeClimb)
REGISTER_LARA_STATE(LS_ROPE_SLIDE, M_RopeSlide)
