#include <trx/game/lara/rope.h>

#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/rope.h>
#include <trx/game/sound.h>

#define M_MAX_SWING_ARC 9000
#define M_JUMP_FALL_SPEED (-112)
#define M_HOP_FALL_SPEED (-20)
#define M_FALL_OFF_Y_SHIFT 320

static uint8_t m_LegsSwinging = 0;

const ANIM *Lara_Rope_GetSwingAnim(void)
{
    return Anim_GetAnim(Object_Get(O_LARA)->anim_idx + LA(LA_ROPE_SWING));
}

void Lara_Rope_ApplyVelocity(const int16_t angle, const uint16_t vel)
{
    // A rope velocity carries twelve bits more than a position does.
    const XYZ_32 pendulum =
        XYZ_32_RotateYaw((XYZ_32) { .z = vel << 12 }, angle);
    Rope_SetPendulumVelocity(pendulum.x, 0, pendulum.z);
}

void Lara_Rope_UpdateSwing(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const int32_t frame_base = Lara_Rope_GetSwingAnim()->frame_base;

    CLAMPG(lara->rope.max_x_forward, M_MAX_SWING_ARC);
    CLAMPG(lara->rope.max_x_backward, M_MAX_SWING_ARC);

    if (lara->rope.direction != 0) {
        if (item->rot.x > 0 && item->rot.x - lara->rope.last_x_rot < -100) {
            lara->rope.arc_front = lara->rope.last_x_rot;
            lara->rope.direction = 0;
            lara->rope.max_x_backward = 0;
            const int32_t frame =
                (15 * lara->rope.max_x_forward / 18000 + frame_base + 47) << 8;
            if (frame > lara->rope.d_frame) {
                lara->rope.d_frame = frame;
                m_LegsSwinging = 1;
            } else {
                m_LegsSwinging = 0;
            }
            Sound_Effect(SFX_LARA_ROPE_CREAK, &item->pos, SPM_NORMAL);
        } else if (
            lara->rope.last_x_rot < 0
            && lara->rope.frame == lara->rope.d_frame) {
            m_LegsSwinging = 0;
            lara->rope.d_frame =
                (15 * lara->rope.max_x_backward / 18000 + frame_base + 47) << 8;
            lara->rope.frame_rate =
                15 * lara->rope.max_x_backward / M_MAX_SWING_ARC + 1;
        } else if (lara->rope.frame_rate < 512) {
            lara->rope.frame_rate += (m_LegsSwinging ? 31 : 7)
                    * lara->rope.max_x_backward / M_MAX_SWING_ARC
                + 1;
        }
    } else if (item->rot.x < 0 && item->rot.x - lara->rope.last_x_rot > 100) {
        lara->rope.arc_back = lara->rope.last_x_rot;
        lara->rope.direction = 1;
        lara->rope.max_x_forward = 0;
        const int32_t frame =
            (frame_base - 15 * lara->rope.max_x_backward / 18000 + 17) << 8;
        if (frame < lara->rope.d_frame) {
            lara->rope.d_frame = frame;
            m_LegsSwinging = 1;
        } else {
            m_LegsSwinging = 0;
        }
        Sound_Effect(SFX_LARA_ROPE_CREAK, &item->pos, SPM_NORMAL);
    } else if (
        lara->rope.last_x_rot > 0 && lara->rope.frame == lara->rope.d_frame) {
        m_LegsSwinging = 0;
        lara->rope.d_frame =
            (frame_base - 15 * lara->rope.max_x_forward / 18000 + 17) << 8;
        lara->rope.frame_rate =
            15 * lara->rope.max_x_forward / M_MAX_SWING_ARC + 1;
    } else if (lara->rope.frame_rate < 512) {
        lara->rope.frame_rate += (m_LegsSwinging ? 31 : 7)
                * lara->rope.max_x_forward / M_MAX_SWING_ARC
            + 1;
    }

    lara->rope.last_x_rot = item->rot.x;
    if (lara->rope.direction != 0) {
        if (item->rot.x > lara->rope.max_x_forward) {
            lara->rope.max_x_forward = item->rot.x;
        }
    } else if (item->rot.x < -lara->rope.max_x_backward) {
        lara->rope.max_x_backward = ABS(item->rot.x);
    }
}

void Lara_Rope_JumpOff(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->rope.index == NO_ROPE) {
        return;
    }

    if (item->rot.x >= 0) {
        item->fall_speed = M_JUMP_FALL_SPEED;
        item->speed = item->rot.x / 128;
    } else {
        item->speed = 0;
        item->fall_speed = M_HOP_FALL_SPEED;
    }
    item->rot.x = 0;
    item->gravity = true;
    lara->gun_status = LGS_ARMLESS;

    const int32_t rel_frame =
        item->frame_num - Lara_Rope_GetSwingAnim()->frame_base;
    LARA_ANIMATION_ID anim;
    if (rel_frame <= 21) {
        anim = LA_ROPE_SWING_TO_REACH;
    } else if (rel_frame <= 42) {
        anim = LA_ROPE_SWING_TO_REACH_MIDDLE;
    } else {
        anim = LA_ROPE_SWING_TO_REACH_FRONT;
    }
    Item_SwitchToAnim(item, LA(anim), 0);
    item->current_anim_state = LS(LS_REACH);
    item->goal_anim_state = LS(LS_REACH);
    lara->rope.index = NO_ROPE;
}

void Lara_Rope_FallOff(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ROPE_PENDULUM *const pendulum = Rope_GetPendulum();

    const int32_t vel = ABS(pendulum->vel.x >> 16) + ABS(pendulum->vel.z >> 16);
    item->speed = (int16_t)(vel >> 1);
    item->rot.x = 0;
    item->pos.y += M_FALL_OFF_Y_SHIFT;
    Item_SwitchToAnim(item, LA(LA_FALL_START), 0);
    item->current_anim_state = LS(LS_JUMP_FORWARD);
    item->goal_anim_state = LS(LS_JUMP_FORWARD);
    item->fall_speed = 0;
    item->gravity = true;
    lara->gun_status = LGS_ARMLESS;
    lara->rope.index = NO_ROPE;
}
