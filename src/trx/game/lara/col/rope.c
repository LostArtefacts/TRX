#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/rope.h>
#include <trx/game/lara/util.h>
#include <trx/game/objects.h>
#include <trx/game/rope.h>

#define M_CAM_SWING_DISTANCE 2048
#define M_MAX_SWING_ARC 9000
#define M_HANG_SWING_ARC 6750
#define M_TOP_SEGMENT 4
#define M_BOTTOM_SEGMENT 21
#define M_LF_SWING_NEUTRAL 32
#define M_LF_KICK_PUSH 15

static const ANIM *M_GetLaraAnim(const LARA_ANIMATION_ID anim)
{
    return Anim_GetAnim(Object_Get(O_LARA)->anim_idx + LA(anim));
}

static void M_RopeIdle(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action) {
        Lara_Rope_FallOff(item);
        return;
    }

    Lara_Rope_UpdateSwing(item);

    if (g_Input.sprint) {
        lara->rope.d_frame =
            (Lara_Rope_GetSwingAnim()->frame_base + M_LF_SWING_NEUTRAL) << 8;
        lara->rope.frame = lara->rope.d_frame;
        item->goal_anim_state = LS(LS_ROPE_FORWARD);
    } else if (g_Input.forward && lara->rope.segment > M_TOP_SEGMENT) {
        item->goal_anim_state = LS(LS_ROPE_CLIMB);
    } else if (g_Input.back && lara->rope.segment < M_BOTTOM_SEGMENT) {
        item->goal_anim_state = LS(LS_ROPE_SLIDE);
        lara->rope.flag = 0;
        lara->rope.count = 0;
    } else if (g_Input.left) {
        item->goal_anim_state = LS(LS_ROPE_LEFT);
    } else if (g_Input.right) {
        item->goal_anim_state = LS(LS_ROPE_RIGHT);
    }
}

static void M_RopeSwing(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    g_Camera.target_distance = M_CAM_SWING_DISTANCE;
    Lara_Rope_UpdateSwing(item);

    if (Item_TestAnimEqual(item, LA(LA_ROPE_SWING))) {
        const int32_t frame_base = Lara_Rope_GetSwingAnim()->frame_base;

        if (g_Input.sprint) {
            const int32_t vel = ABS(lara->rope.last_x_rot) < M_MAX_SWING_ARC
                ? 192 * (M_MAX_SWING_ARC - ABS(lara->rope.last_x_rot))
                    / M_MAX_SWING_ARC
                : 0;
            Lara_Rope_ApplyVelocity(
                item->rot.y + (lara->rope.direction == 0 ? 32760 : 0),
                (uint16_t)(vel >> 5));
        }

        if (lara->rope.frame < lara->rope.d_frame) {
            lara->rope.frame += lara->rope.frame_rate;
            CLAMPG(lara->rope.frame, lara->rope.d_frame);
        } else if (lara->rope.frame > lara->rope.d_frame) {
            lara->rope.frame -= lara->rope.frame_rate;
            CLAMPL(lara->rope.frame, lara->rope.d_frame);
        }

        item->frame_num = (int16_t)(lara->rope.frame >> 8);

        if (!g_Input.sprint
            && (lara->rope.frame >> 8) == frame_base + M_LF_SWING_NEUTRAL
            && lara->rope.max_x_backward < M_HANG_SWING_ARC
            && lara->rope.max_x_forward < M_HANG_SWING_ARC) {
            Item_SwitchToAnim(item, LA(LA_JUMP_UP_TO_ROPE_END), 0);
            item->current_anim_state = LS(LS_ROPE_IDLE);
            item->goal_anim_state = LS(LS_ROPE_IDLE);
        }

        if (g_Input.jump) {
            Lara_Rope_JumpOff(item);
        }
    } else if (
        item->frame_num
        == M_GetLaraAnim(LA_ROPE_IDLE_TO_SWING)->frame_base + M_LF_KICK_PUSH) {
        Lara_Rope_ApplyVelocity(item->rot.y, 128);
    }
}

REGISTER_LARA_COL(LS_ROPE_IDLE, M_RopeIdle)
REGISTER_LARA_COL(LS_ROPE_FORWARD, M_RopeSwing)
REGISTER_LARA_COL(LS_ROPE_BACK, M_RopeSwing)
